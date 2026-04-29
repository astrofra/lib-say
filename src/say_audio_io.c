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
