#define _CRT_SECURE_NO_WARNINGS

#include "say_internal.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * Stream writers — emit a few primitive integer types in big- and little-endian
 * to a FILE pointer. Used by AIFF (BE) and WAV (LE) container synthesis.
 * ------------------------------------------------------------------------- */

static void say_write_u16_be(FILE *file, unsigned int value)
{
    unsigned char bytes[2];

    bytes[0] = (unsigned char) ((value >> 8) & 0xFFu);
    bytes[1] = (unsigned char) (value & 0xFFu);
    fwrite(bytes, sizeof(bytes), 1, file);
}

static void say_store_u16_be(unsigned char *bytes, unsigned int value)
{
    bytes[0] = (unsigned char) ((value >> 8) & 0xFFu);
    bytes[1] = (unsigned char) (value & 0xFFu);
}

static void say_write_u32_be(FILE *file, unsigned int value)
{
    unsigned char bytes[4];

    bytes[0] = (unsigned char) ((value >> 24) & 0xFFu);
    bytes[1] = (unsigned char) ((value >> 16) & 0xFFu);
    bytes[2] = (unsigned char) ((value >> 8) & 0xFFu);
    bytes[3] = (unsigned char) (value & 0xFFu);
    fwrite(bytes, sizeof(bytes), 1, file);
}

static void say_store_u32_be(unsigned char *bytes, unsigned int value)
{
    bytes[0] = (unsigned char) ((value >> 24) & 0xFFu);
    bytes[1] = (unsigned char) ((value >> 16) & 0xFFu);
    bytes[2] = (unsigned char) ((value >> 8) & 0xFFu);
    bytes[3] = (unsigned char) (value & 0xFFu);
}

static void say_write_u16_le(FILE *file, unsigned int value)
{
    unsigned char bytes[2];

    bytes[0] = (unsigned char) (value & 0xFFu);
    bytes[1] = (unsigned char) ((value >> 8) & 0xFFu);
    fwrite(bytes, sizeof(bytes), 1, file);
}

static void say_store_u16_le(unsigned char *bytes, unsigned int value)
{
    bytes[0] = (unsigned char) (value & 0xFFu);
    bytes[1] = (unsigned char) ((value >> 8) & 0xFFu);
}

static void say_write_u32_le(FILE *file, unsigned int value)
{
    unsigned char bytes[4];

    bytes[0] = (unsigned char) (value & 0xFFu);
    bytes[1] = (unsigned char) ((value >> 8) & 0xFFu);
    bytes[2] = (unsigned char) ((value >> 16) & 0xFFu);
    bytes[3] = (unsigned char) ((value >> 24) & 0xFFu);
    fwrite(bytes, sizeof(bytes), 1, file);
}

static void say_store_u32_le(unsigned char *bytes, unsigned int value)
{
    bytes[0] = (unsigned char) (value & 0xFFu);
    bytes[1] = (unsigned char) ((value >> 8) & 0xFFu);
    bytes[2] = (unsigned char) ((value >> 16) & 0xFFu);
    bytes[3] = (unsigned char) ((value >> 24) & 0xFFu);
}

/* IEEE 754 80-bit extended precision encoding for the AIFF COMM chunk. */
static void say_write_ieee_extended(FILE *file, double value)
{
    unsigned char bytes[10];
    unsigned int exponent;
    unsigned long long significand;
    int exp2;
    double fraction;
    double mantissa;
    long double scaled;
    size_t i;

    memset(bytes, 0, sizeof(bytes));

    if (value > 0.0) {
        fraction = frexp(value, &exp2);
        exponent = (unsigned int) (exp2 - 1 + 16383);
        mantissa = ldexp(fraction, 1);
        scaled = (long double) mantissa * (long double) (1ull << 63);
        significand = (unsigned long long) (scaled + 0.5L);

        if (significand == 0ull) {
            exponent = 0;
        }

        bytes[0] = (unsigned char) ((exponent >> 8) & 0x7F);
        bytes[1] = (unsigned char) (exponent & 0xFFu);
        for (i = 0; i < 8; ++i) {
            bytes[2 + i] = (unsigned char) ((significand >> (56 - 8 * i)) & 0xFFu);
        }
    }

    fwrite(bytes, sizeof(bytes), 1, file);
}

static void say_store_ieee_extended(unsigned char *bytes, double value)
{
    unsigned int exponent;
    unsigned long long significand;
    int exp2;
    double fraction;
    double mantissa;
    long double scaled;
    size_t i;

    memset(bytes, 0, 10);

    if (value > 0.0) {
        fraction = frexp(value, &exp2);
        exponent = (unsigned int) (exp2 - 1 + 16383);
        mantissa = ldexp(fraction, 1);
        scaled = (long double) mantissa * (long double) (1ull << 63);
        significand = (unsigned long long) (scaled + 0.5L);

        if (significand == 0ull) {
            exponent = 0;
        }

        bytes[0] = (unsigned char) ((exponent >> 8) & 0x7F);
        bytes[1] = (unsigned char) (exponent & 0xFFu);
        for (i = 0; i < 8; ++i) {
            bytes[2 + i] = (unsigned char) ((significand >> (56 - 8 * i)) & 0xFFu);
        }
    }
}

/* ---------------------------------------------------------------------------
 * Per-format file writers
 * ------------------------------------------------------------------------- */

static int say_write_raw(
    const char *path,
    const int16_t *samples,
    size_t sample_count,
    char *error,
    size_t error_size
)
{
    FILE *file;

    file = fopen(path, "wb");
    if (file == NULL) {
        say_set_error(error, error_size, "unable to open %s: %s", path, strerror(errno));
        return 0;
    }

    if (sample_count > 0 && fwrite(samples, sizeof(*samples), sample_count, file) != sample_count) {
        fclose(file);
        say_set_error(error, error_size, "failed to write raw output to %s", path);
        return 0;
    }

    fclose(file);
    return 1;
}

static int say_write_aiff(
    const char *path,
    int sample_rate,
    const int16_t *samples,
    size_t sample_count,
    char *error,
    size_t error_size
)
{
    FILE *file;
    unsigned int sound_bytes;
    size_t i;

    if (sample_count > 0x7FFFFFFFu / 2u) {
        say_set_error(error, error_size, "sample buffer is too large for AIFF");
        return 0;
    }

    sound_bytes = (unsigned int) (sample_count * 2u);
    file = fopen(path, "wb");
    if (file == NULL) {
        say_set_error(error, error_size, "unable to open %s: %s", path, strerror(errno));
        return 0;
    }

    fwrite("FORM", 4, 1, file);
    say_write_u32_be(file, 46u + sound_bytes);
    fwrite("AIFF", 4, 1, file);

    fwrite("COMM", 4, 1, file);
    say_write_u32_be(file, 18u);
    say_write_u16_be(file, 1u);
    say_write_u32_be(file, (unsigned int) sample_count);
    say_write_u16_be(file, 16u);
    say_write_ieee_extended(file, (double) sample_rate);

    fwrite("SSND", 4, 1, file);
    say_write_u32_be(file, 8u + sound_bytes);
    say_write_u32_be(file, 0u);
    say_write_u32_be(file, 0u);

    for (i = 0; i < sample_count; ++i) {
        unsigned short value = (unsigned short) samples[i];
        unsigned char bytes[2];
        bytes[0] = (unsigned char) ((value >> 8) & 0xFFu);
        bytes[1] = (unsigned char) (value & 0xFFu);
        fwrite(bytes, sizeof(bytes), 1, file);
    }

    fclose(file);
    return 1;
}

static int say_write_wav(
    const char *path,
    int sample_rate,
    const int16_t *samples,
    size_t sample_count,
    char *error,
    size_t error_size
)
{
    FILE *file;
    unsigned int data_bytes;
    size_t i;

    if (sample_count > 0xFFFFFFFFu / 2u) {
        say_set_error(error, error_size, "sample buffer is too large for WAV");
        return 0;
    }

    data_bytes = (unsigned int) (sample_count * 2u);
    if (data_bytes > 0xFFFFFFFFu - 36u) {
        say_set_error(error, error_size, "sample buffer is too large for WAV");
        return 0;
    }

    file = fopen(path, "wb");
    if (file == NULL) {
        say_set_error(error, error_size, "unable to open %s: %s", path, strerror(errno));
        return 0;
    }

    fwrite("RIFF", 4, 1, file);
    say_write_u32_le(file, 36u + data_bytes);
    fwrite("WAVE", 4, 1, file);

    fwrite("fmt ", 4, 1, file);
    say_write_u32_le(file, 16u);
    say_write_u16_le(file, 1u);
    say_write_u16_le(file, 1u);
    say_write_u32_le(file, (unsigned int) sample_rate);
    say_write_u32_le(file, (unsigned int) sample_rate * 2u);
    say_write_u16_le(file, 2u);
    say_write_u16_le(file, 16u);

    fwrite("data", 4, 1, file);
    say_write_u32_le(file, data_bytes);

    for (i = 0; i < sample_count; ++i) {
        unsigned short value = (unsigned short) samples[i];
        unsigned char bytes[2];
        bytes[0] = (unsigned char) (value & 0xFFu);
        bytes[1] = (unsigned char) ((value >> 8) & 0xFFu);
        fwrite(bytes, sizeof(bytes), 1, file);
    }

    fclose(file);
    return 1;
}

/* ---------------------------------------------------------------------------
 * Public API: write a synthesized buffer to a file or to a fresh in-memory blob
 * ------------------------------------------------------------------------- */

int say_write_audio_file(
    const char *path,
    say_audio_format_t format,
    int sample_rate,
    const int16_t *samples,
    size_t sample_count,
    char *error,
    size_t error_size
)
{
    if (path == NULL || path[0] == '\0') {
        say_set_error(error, error_size, "output path is required");
        return 0;
    }
    if (samples == NULL && sample_count != 0) {
        say_set_error(error, error_size, "sample buffer is null");
        return 0;
    }

    if (format == SAY_FORMAT_AIFF) {
        return say_write_aiff(path, sample_rate, samples, sample_count, error, error_size);
    }
    if (format == SAY_FORMAT_WAV) {
        return say_write_wav(path, sample_rate, samples, sample_count, error, error_size);
    }
    return say_write_raw(path, samples, sample_count, error, error_size);
}

int say_encode_audio(
    say_audio_format_t format,
    int sample_rate,
    const int16_t *samples,
    size_t sample_count,
    uint8_t **out_data,
    size_t *out_size,
    char *error,
    size_t error_size
)
{
    uint8_t *data;
    size_t byte_count;
    size_t i;

    if (out_data == NULL || out_size == NULL) {
        say_set_error(error, error_size, "encoded audio output pointers must not be null");
        return 0;
    }
    if (samples == NULL && sample_count != 0) {
        say_set_error(error, error_size, "sample buffer is null");
        return 0;
    }

    *out_data = NULL;
    *out_size = 0;

    if (format == SAY_FORMAT_AIFF) {
        unsigned int sound_bytes;

        if (sample_count > 0x7FFFFFFFu / 2u) {
            say_set_error(error, error_size, "sample buffer is too large for AIFF");
            return 0;
        }
        if (sample_count > (SIZE_MAX - 54u) / 2u) {
            say_set_error(error, error_size, "sample buffer is too large to encode");
            return 0;
        }

        sound_bytes = (unsigned int) (sample_count * 2u);
        byte_count = 54u + (size_t) sound_bytes;
        data = (uint8_t *) malloc(byte_count);
        if (data == NULL) {
            say_set_error(error, error_size, "out of memory while encoding AIFF blob");
            return 0;
        }

        memcpy(data + 0, "FORM", 4);
        say_store_u32_be(data + 4, 46u + sound_bytes);
        memcpy(data + 8, "AIFF", 4);

        memcpy(data + 12, "COMM", 4);
        say_store_u32_be(data + 16, 18u);
        say_store_u16_be(data + 20, 1u);
        say_store_u32_be(data + 22, (unsigned int) sample_count);
        say_store_u16_be(data + 26, 16u);
        say_store_ieee_extended(data + 28, (double) sample_rate);

        memcpy(data + 38, "SSND", 4);
        say_store_u32_be(data + 42, 8u + sound_bytes);
        say_store_u32_be(data + 46, 0u);
        say_store_u32_be(data + 50, 0u);

        for (i = 0; i < sample_count; ++i) {
            unsigned short value = (unsigned short) samples[i];
            data[54u + i * 2u + 0u] = (uint8_t) ((value >> 8) & 0xFFu);
            data[54u + i * 2u + 1u] = (uint8_t) (value & 0xFFu);
        }
    }
    else if (format == SAY_FORMAT_WAV) {
        unsigned int data_bytes;

        if (sample_count > 0xFFFFFFFFu / 2u) {
            say_set_error(error, error_size, "sample buffer is too large for WAV");
            return 0;
        }
        if (sample_count > (SIZE_MAX - 44u) / 2u) {
            say_set_error(error, error_size, "sample buffer is too large to encode");
            return 0;
        }

        data_bytes = (unsigned int) (sample_count * 2u);
        if (data_bytes > 0xFFFFFFFFu - 36u) {
            say_set_error(error, error_size, "sample buffer is too large for WAV");
            return 0;
        }

        byte_count = 44u + (size_t) data_bytes;
        data = (uint8_t *) malloc(byte_count);
        if (data == NULL) {
            say_set_error(error, error_size, "out of memory while encoding WAV blob");
            return 0;
        }

        memcpy(data + 0, "RIFF", 4);
        say_store_u32_le(data + 4, 36u + data_bytes);
        memcpy(data + 8, "WAVE", 4);

        memcpy(data + 12, "fmt ", 4);
        say_store_u32_le(data + 16, 16u);
        say_store_u16_le(data + 20, 1u);
        say_store_u16_le(data + 22, 1u);
        say_store_u32_le(data + 24, (unsigned int) sample_rate);
        say_store_u32_le(data + 28, (unsigned int) sample_rate * 2u);
        say_store_u16_le(data + 32, 2u);
        say_store_u16_le(data + 34, 16u);

        memcpy(data + 36, "data", 4);
        say_store_u32_le(data + 40, data_bytes);

        for (i = 0; i < sample_count; ++i) {
            unsigned short value = (unsigned short) samples[i];
            data[44u + i * 2u + 0u] = (uint8_t) (value & 0xFFu);
            data[44u + i * 2u + 1u] = (uint8_t) ((value >> 8) & 0xFFu);
        }
    }
    else {
        if (sample_count > SIZE_MAX / 2u) {
            say_set_error(error, error_size, "sample buffer is too large to encode");
            return 0;
        }

        byte_count = sample_count * 2u;
        data = (uint8_t *) malloc(byte_count > 0 ? byte_count : 1u);
        if (data == NULL) {
            say_set_error(error, error_size, "out of memory while encoding raw blob");
            return 0;
        }

        for (i = 0; i < sample_count; ++i) {
            unsigned short value = (unsigned short) samples[i];
            data[i * 2u + 0u] = (uint8_t) (value & 0xFFu);
            data[i * 2u + 1u] = (uint8_t) ((value >> 8) & 0xFFu);
        }
    }

    *out_data = data;
    *out_size = byte_count;
    return 1;
}

/* Linear gain + soft-knee saturator. Samples are normalised to [-1, 1],
 * scaled by gain, then any magnitude above KNEE is bent toward the ±1
 * ceiling with a tanh curve. tanh'(0) = 1, so the slope matches the linear
 * branch at the knee — no kink, no audible discontinuity. tanh < 1 strictly
 * for finite input, so the result is always inside the int16 range without
 * needing a saturating clamp; the clamp at the end only catches rounding. */
void say_apply_gain(int16_t *samples, size_t sample_count, double gain)
{
    const double KNEE = 0.7;
    const double knee_span = 1.0 - KNEE;
    size_t i;

    if (samples == NULL || sample_count == 0 || gain == 1.0) {
        return;
    }

    for (i = 0; i < sample_count; ++i) {
        double x = (double) samples[i] * (1.0 / 32768.0);
        double abs_x;
        double y;
        int v;

        x *= gain;
        abs_x = x < 0.0 ? -x : x;

        if (abs_x <= KNEE) {
            y = x;
        }
        else {
            double sign = x < 0.0 ? -1.0 : 1.0;
            double over = (abs_x - KNEE) / knee_span;
            y = sign * (KNEE + knee_span * tanh(over));
        }

        v = (int) (y * 32767.0 + (y >= 0.0 ? 0.5 : -0.5));
        if (v >  32767) v =  32767;
        if (v < -32768) v = -32768;
        samples[i] = (int16_t) v;
    }
}

/* Telephone-band emulator — "remote / bad-connection" flavour.
 *
 * Signal chain: HPF(500 Hz)x2 -> LPF(2800 Hz)x2 -> peak EQ(+8 dB at 1700 Hz)
 * -> tanh saturator. The bandpass is tighter than the canonical 300-3400 Hz
 * channel and each side is a 2-biquad cascade (24 dB/oct), so the voice is
 * audibly stripped of low body and high air. The peak EQ adds the small
 * resonance you'd expect from a tiny earpiece capsule — it's what makes a
 * phone voice sound "tinny" rather than just bandlimited. The tanh is driven
 * hard enough to compress consonants the way a low-bitrate codec would, with
 * a matching makeup gain so the perceived loudness lands near the input.
 *
 * All biquad coefficients come from the RBJ audio-EQ cookbook. Stable for
 * any sample rate where 2800 Hz < Nyquist (i.e. rate >= ~6 kHz). */
void say_apply_phone_filter(int16_t *samples, size_t sample_count, int sample_rate)
{
    const double PI = 3.14159265358979323846;
    const double HPF_FREQ = 500.0;
    const double LPF_FREQ = 2800.0;
    const double BUTTER_Q = 0.7071067811865476;
    const double PEAK_FREQ = 1700.0;
    const double PEAK_Q = 2.0;
    const double PEAK_GAIN_DB = 8.0;
    const double DRIVE = 2.4;
    const double MAKEUP = 2.6;

    double hp1_b0, hp1_b1, hp1_b2, hp1_a1, hp1_a2;
    double hp2_b0, hp2_b1, hp2_b2, hp2_a1, hp2_a2;
    double lp1_b0, lp1_b1, lp1_b2, lp1_a1, lp1_a2;
    double lp2_b0, lp2_b1, lp2_b2, lp2_a1, lp2_a2;
    double pk_b0,  pk_b1,  pk_b2,  pk_a1,  pk_a2;
    double hp1_x1, hp1_x2, hp1_y1, hp1_y2;
    double hp2_x1, hp2_x2, hp2_y1, hp2_y2;
    double lp1_x1, lp1_x2, lp1_y1, lp1_y2;
    double lp2_x1, lp2_x2, lp2_y1, lp2_y2;
    double pk_x1,  pk_x2,  pk_y1,  pk_y2;
    size_t i;

    if (samples == NULL || sample_count == 0 || sample_rate <= 0) {
        return;
    }
    if ((double) sample_rate <= 2.0 * LPF_FREQ) {
        return;
    }

    /* Two cascaded HPFs at 500 Hz (24 dB/oct). */
    {
        double w0 = 2.0 * PI * HPF_FREQ / (double) sample_rate;
        double cos_w0 = cos(w0);
        double alpha = sin(w0) / (2.0 * BUTTER_Q);
        double a0 = 1.0 + alpha;
        double b0 = ((1.0 + cos_w0) * 0.5) / a0;
        double b1 = (-(1.0 + cos_w0))      / a0;
        double b2 = ((1.0 + cos_w0) * 0.5) / a0;
        double a1 = (-2.0 * cos_w0)        / a0;
        double a2 = (1.0 - alpha)          / a0;
        hp1_b0 = hp2_b0 = b0;
        hp1_b1 = hp2_b1 = b1;
        hp1_b2 = hp2_b2 = b2;
        hp1_a1 = hp2_a1 = a1;
        hp1_a2 = hp2_a2 = a2;
    }
    /* Two cascaded LPFs at 2800 Hz (24 dB/oct). */
    {
        double w0 = 2.0 * PI * LPF_FREQ / (double) sample_rate;
        double cos_w0 = cos(w0);
        double alpha = sin(w0) / (2.0 * BUTTER_Q);
        double a0 = 1.0 + alpha;
        double b0 = ((1.0 - cos_w0) * 0.5) / a0;
        double b1 = (1.0 - cos_w0)         / a0;
        double b2 = ((1.0 - cos_w0) * 0.5) / a0;
        double a1 = (-2.0 * cos_w0)        / a0;
        double a2 = (1.0 - alpha)          / a0;
        lp1_b0 = lp2_b0 = b0;
        lp1_b1 = lp2_b1 = b1;
        lp1_b2 = lp2_b2 = b2;
        lp1_a1 = lp2_a1 = a1;
        lp1_a2 = lp2_a2 = a2;
    }
    /* Peak EQ at 1700 Hz, +PEAK_GAIN_DB, Q=2 — earpiece resonance. */
    {
        double A = pow(10.0, PEAK_GAIN_DB / 40.0);
        double w0 = 2.0 * PI * PEAK_FREQ / (double) sample_rate;
        double cos_w0 = cos(w0);
        double alpha = sin(w0) / (2.0 * PEAK_Q);
        double a0 = 1.0 + alpha / A;
        pk_b0 = (1.0 + alpha * A) / a0;
        pk_b1 = (-2.0 * cos_w0)   / a0;
        pk_b2 = (1.0 - alpha * A) / a0;
        pk_a1 = (-2.0 * cos_w0)   / a0;
        pk_a2 = (1.0 - alpha / A) / a0;
    }

    hp1_x1 = hp1_x2 = hp1_y1 = hp1_y2 = 0.0;
    hp2_x1 = hp2_x2 = hp2_y1 = hp2_y2 = 0.0;
    lp1_x1 = lp1_x2 = lp1_y1 = lp1_y2 = 0.0;
    lp2_x1 = lp2_x2 = lp2_y1 = lp2_y2 = 0.0;
    pk_x1  = pk_x2  = pk_y1  = pk_y2  = 0.0;

    for (i = 0; i < sample_count; ++i) {
        double x = (double) samples[i] * (1.0 / 32768.0);
        double y, sat;
        int v;

        y = hp1_b0 * x + hp1_b1 * hp1_x1 + hp1_b2 * hp1_x2 - hp1_a1 * hp1_y1 - hp1_a2 * hp1_y2;
        hp1_x2 = hp1_x1; hp1_x1 = x;
        hp1_y2 = hp1_y1; hp1_y1 = y;

        {
            double in = y;
            y = hp2_b0 * in + hp2_b1 * hp2_x1 + hp2_b2 * hp2_x2 - hp2_a1 * hp2_y1 - hp2_a2 * hp2_y2;
            hp2_x2 = hp2_x1; hp2_x1 = in;
            hp2_y2 = hp2_y1; hp2_y1 = y;
        }
        {
            double in = y;
            y = lp1_b0 * in + lp1_b1 * lp1_x1 + lp1_b2 * lp1_x2 - lp1_a1 * lp1_y1 - lp1_a2 * lp1_y2;
            lp1_x2 = lp1_x1; lp1_x1 = in;
            lp1_y2 = lp1_y1; lp1_y1 = y;
        }
        {
            double in = y;
            y = lp2_b0 * in + lp2_b1 * lp2_x1 + lp2_b2 * lp2_x2 - lp2_a1 * lp2_y1 - lp2_a2 * lp2_y2;
            lp2_x2 = lp2_x1; lp2_x1 = in;
            lp2_y2 = lp2_y1; lp2_y1 = y;
        }
        {
            double in = y;
            y = pk_b0 * in + pk_b1 * pk_x1 + pk_b2 * pk_x2 - pk_a1 * pk_y1 - pk_a2 * pk_y2;
            pk_x2 = pk_x1; pk_x1 = in;
            pk_y2 = pk_y1; pk_y1 = y;
        }

        sat = tanh(y * DRIVE * MAKEUP);

        v = (int) (sat * 32767.0 + (sat >= 0.0 ? 0.5 : -0.5));
        if (v >  32767) v =  32767;
        if (v < -32768) v = -32768;
        samples[i] = (int16_t) v;
    }
}
