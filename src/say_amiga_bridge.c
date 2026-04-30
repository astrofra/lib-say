/*
 * say_amiga_bridge.c — P5 bridge from lib-say frame_t[] to the Amiga substrate.
 *
 * lib-say's upstream pipeline (text → segments → frames) is sample-rate-agnostic
 * up to the synthesizer. The biquad path consumes frames at any rate; the
 * Amiga substrate consumes a packed 8-byte coefficient buffer tied to the
 * 22 050 Hz the impulse-response LUTs were measured at.
 *
 * For each lib-say frame this builds one Amiga coef record:
 *   F1_idx, F2_idx, F3_idx — nearest-neighbour into the substrate's LUT
 *                            frequency tables (from f1.asm / f2.asm / f3.asm)
 *   AMP1/2/3              — quantized to 5 bits from amplitude * gain[k]
 *   FLAGS                 — V/F/A/N steering inferred from voiced + noise_mix
 *   F0                    — pitch period in samples (sample_rate / pitch_hz)
 *
 * The buffer is then run through say_synth_amiga_run_ex() with samperframe
 * pinned to the lib-say frame cadence (so each lib-say frame holds for one
 * microframe). The first sample of the next pitch period after samperframe
 * expires reloads the next coef.
 *
 * Output rate is SAY_AMIGA_SAMPLE_RATE — the caller is responsible for
 * writing at that rate.
 */

#define _CRT_SECURE_NO_WARNINGS

#include "say_internal.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Bit layout in the coef[FLAGS] byte — must match say_synth_amiga.c. */
#define FLAG_NASAL  (1u << 4)
#define FLAG_ASPIR  (1u << 5)
#define FLAG_FRIC   (1u << 6)
#define FLAG_VOICED (1u << 7)

/* LUT frequency tables, copied from the comments in f1.asm / f2.asm / f3.asm.
 * The substrate has F1table at 31 entries, F2table at 31 entries, F3table at
 * 21 entries. Indexing is by nearest-neighbour. */
static const int F1_FREQS[31] = {
    200, 209, 220, 231, 243, 255, 268, 281, 295, 310,
    325, 342, 359, 377, 395, 415, 436, 458, 481, 505,
    530, 557, 585, 614, 645, 677, 711, 746, 784, 823, 864
};

static const int F2_FREQS[31] = {
    600,  630,  661,  694,  729,  765,  804,  844,  886,  930,
    977,  1026, 1077, 1131, 1187, 1247, 1309, 1375, 1443, 1516,
    1591, 1671, 1755, 1842, 1935, 2031, 2133, 2240, 2352, 2469, 2593
};

static const int F3_FREQS[21] = {
    1300, 1364, 1433, 1504, 1580, 1659, 1742, 1829, 1920, 2016,
    2117, 2223, 2334, 2451, 2573, 2702, 2837, 2979, 3128, 3285, 3449
};

static int nearest_idx(double hz, const int *table, int count)
{
    int best = 0;
    double best_d = 1e30;
    int i;
    for (i = 0; i < count; ++i) {
        double d = hz - (double) table[i];
        if (d < 0.0) d = -d;
        if (d < best_d) { best_d = d; best = i; }
    }
    return best;
}

static int clamp_amp_5bit(double v)
{
    int a = (int) (v + 0.5);
    if (a < 0)  return 0;
    if (a > 31) return 31;
    return a;
}

/* Pick a fricative LUT record (FRICtable, 17 records) by where the noise
 * energy sits. Approximate mapping inferred from phonet.h comments — sibilants
 * (S/Z) live at index 0, palatal sibilants (SH/ZH) around 2, lip/dental
 * fricatives (F/V/TH/DH) around 4. */
static int pick_fric_record(const frame_t *fr)
{
    /* lib-say's fricative formant_freq[1] (F2 region) is the primary cue. */
    double f2 = fr->formant_freq[1];
    if (f2 >= 2400.0) return 0;        /* S/Z high-frequency sibilants */
    if (f2 >= 1700.0) return 2;        /* SH/ZH palatal sibilants */
    return 4;                          /* F/V/TH/DH lower-frequency fricatives */
}

/* Aspirant LUT record (AXP-like). The original Amiga uses index 5 of FRICtable
 * for /h/ and aspiration. */
#define ASPIR_RECORD 5

/* Build one 8-byte coef from a lib-say frame. */
static void frame_to_coef(const frame_t *fr, uint8_t out[8])
{
    int f1_idx = nearest_idx(fr->formant_freq[0], F1_FREQS, 31);
    int f2_idx = nearest_idx(fr->formant_freq[1], F2_FREQS, 31);
    int f3_idx = nearest_idx(fr->formant_freq[2], F3_FREQS, 21);

    unsigned flags = 0;
    int silent = fr->is_pause || fr->amplitude < 0.02;
    int is_nasal_consonant = say_phone_has_features(fr->source_phoneme, F_NASAL);
    /* Aspirants: /h/ and the paragogic-schwa allophone (post-voiceless-plosive
     * release). lib-say has no F_ASPIRANT feature so we name them directly.
     * The Amiga substrate's AH branch (synth.asm:393) plays the F3 aspiration
     * record (FRICtable index 5) attenuated 18 dB; with VOICED set the
     * substrate switches to F1AH (F1 + aspiration), giving voiced /ɦ/. */
    int is_aspirant_phone = (fr->source_phoneme == PH_H || fr->source_phoneme == PH_AXP);

    if (!silent) {
        if (fr->voiced)               flags |= FLAG_VOICED;
        if (is_aspirant_phone) {
            flags |= FLAG_ASPIR;
        } else if (fr->noise_mix > 0.45) {
            flags |= FLAG_FRIC;
        } else if (!fr->voiced && fr->noise_mix > 0.20) {
            flags |= FLAG_ASPIR;
        }
        /* Nasal consonants (M N NY NG) — V+N steers the substrate to the
         * AN routine (synth.asm:393, F1 only with voicebar character). The
         * acoustic class can't be inferred from voiced + noise_mix alone, so
         * we read it off the source phoneme. F_NASAL_VOWEL phonemes (French
         * AN/ON/IN) deliberately stay in the vowel branch — they're voiced
         * vowels with nasal coupling, not full nasal stops. */
        if (is_nasal_consonant && fr->voiced) flags |= FLAG_NASAL;
    }

    /* Override F2/F3 indices when the substrate is steered to FRIC/ASPIR — in
     * those branches the coef bytes index FRICtable instead of F2table/F3table. */
    int f2_byte = (flags & FLAG_FRIC)  ? pick_fric_record(fr) : f2_idx;
    int f3_byte = (flags & FLAG_ASPIR) ? ASPIR_RECORD          : f3_idx;

    /* Amplitudes. The phoneme table's amplitude field tops out near 1.0 and
     * gain[] tops out near 1.0; the smoke test already established that 28..30
     * is full volume on the substrate's MULT table. We aim a bit lower (26)
     * because hitting amp=31 every loud-vowel frame parks samples on the
     * MULT-table corner where the int8 product flips to -128 (= -32768 after
     * the int16 promotion in synth_emit_sample) and audibly clips. */
    const double AMP_SCALE = 26.0;
    int a1 = clamp_amp_5bit(fr->amplitude * fr->gain[0] * AMP_SCALE);
    int a2 = clamp_amp_5bit(fr->amplitude * fr->gain[1] * AMP_SCALE);
    int a3 = clamp_amp_5bit(fr->amplitude * fr->gain[2] * AMP_SCALE);

    if (silent) {
        a1 = a2 = a3 = 0;
    }

    /* For unvoiced fricatives the substrate uses AMP2 to drive FRICtable. If
     * lib-say's gain[1] is small (most fricatives have low F2 gain in the
     * formant table) the result would be inaudible. Lift it from amplitude
     * directly when fricative-class. */
    if (flags & FLAG_FRIC) {
        a2 = clamp_amp_5bit(fr->amplitude * AMP_SCALE);
        if (!fr->voiced) a1 = a3 = 0;
    }
    if (flags & FLAG_ASPIR) {
        a3 = clamp_amp_5bit(fr->amplitude * AMP_SCALE);
        if (!fr->voiced) a1 = a2 = 0;
    }

    /* F0 byte = pitch period in samples = sample_rate / pitch_hz. Clamp to
     * the byte range; pitch_hz ~= 100 Hz → F0 byte ~= 220, fine. */
    int f0_byte = 200;
    if (fr->pitch_hz > 1.0) {
        double period = (double) SAY_AMIGA_SAMPLE_RATE / fr->pitch_hz;
        if (period < 1.0)   period = 1.0;
        if (period > 255.0) period = 255.0;
        f0_byte = (int) (period + 0.5);
    }

    out[0] = (uint8_t) f1_idx;
    out[1] = (uint8_t) f2_byte;
    out[2] = (uint8_t) f3_byte;
    out[3] = (uint8_t) a1;
    out[4] = (uint8_t) a2;
    out[5] = (uint8_t) a3;
    out[6] = (uint8_t) flags;
    out[7] = (uint8_t) f0_byte;
}

/* Linear 4x upsample int16 buffer (allocates a new buffer). The substrate
 * synthesizes at SAY_AMIGA_SAMPLE_RATE = 11025 Hz; we deliver 44100 Hz so the
 * file rate matches what most host audio paths force on mono PCM. Linear
 * interpolation is good enough for cascade-formant output where everything
 * above ~5 kHz is already attenuated by the LUTs. */
static int upsample_4x_linear(
    int16_t **buf, size_t *count, char *error, size_t error_size)
{
    size_t in_n = *count;
    if (in_n < 2) return 1;
    size_t out_n = in_n * 4;
    int16_t *out = (int16_t *) malloc(out_n * sizeof(int16_t));
    if (out == NULL) {
        say_set_error(error, error_size, "amiga bridge: out of memory upsampling output");
        return 0;
    }
    int16_t *in = *buf;
    size_t i;
    for (i = 0; i + 1 < in_n; ++i) {
        int a = in[i];
        int b = in[i + 1];
        out[4 * i]     = (int16_t) a;
        out[4 * i + 1] = (int16_t) (a + (b - a) / 4);
        out[4 * i + 2] = (int16_t) (a + (b - a) / 2);
        out[4 * i + 3] = (int16_t) (a + 3 * (b - a) / 4);
    }
    /* Last input sample: pad with itself (no successor to interpolate with). */
    int16_t last = in[in_n - 1];
    out[4 * (in_n - 1)]     = last;
    out[4 * (in_n - 1) + 1] = last;
    out[4 * (in_n - 1) + 2] = last;
    out[4 * (in_n - 1) + 3] = last;

    free(in);
    *buf = out;
    *count = out_n;
    return 1;
}

int say_synth_amiga_from_frames(
    const frame_t *frames,
    size_t frame_count,
    int frame_ms,
    int16_t **out_samples,
    size_t *out_sample_count,
    char *error,
    size_t error_size)
{
    size_t i;
    uint8_t *coef;
    int samperframe;
    int rc;

    if (frames == NULL || frame_count == 0) {
        say_set_error(error, error_size, "amiga bridge: no frames to synthesize");
        return 0;
    }
    if (frame_ms < 1) {
        say_set_error(error, error_size, "amiga bridge: frame_ms must be >= 1");
        return 0;
    }
    if (out_samples == NULL || out_sample_count == NULL) {
        say_set_error(error, error_size, "amiga bridge: output pointers must not be null");
        return 0;
    }

    coef = (uint8_t *) malloc((frame_count + 1) * 8u);
    if (coef == NULL) {
        say_set_error(error, error_size, "amiga bridge: out of memory for coef buffer");
        return 0;
    }

    for (i = 0; i < frame_count; ++i) {
        frame_to_coef(&frames[i], coef + i * 8);
    }

    if (getenv("SAY_AMIGA_DEBUG")) {
        size_t k;
        size_t lim = frame_count < 30 ? frame_count : 30;
        fprintf(stderr, "[amiga-bridge] frames=%zu frame_ms=%d samperframe=%d\n",
                frame_count, frame_ms, (SAY_AMIGA_SAMPLE_RATE * frame_ms) / 1000);
        for (k = 0; k < lim; ++k) {
            const uint8_t *c = coef + k * 8;
            const frame_t *fr = &frames[k];
            fprintf(stderr, "  [%3zu] f1=%2u f2=%2u f3=%2u  amp=%2u/%2u/%2u  flags=0x%02x  F0=%3u  pitch_hz=%6.1f voiced=%d nm=%4.2f\n",
                    k, c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7],
                    fr->pitch_hz, fr->voiced, fr->noise_mix);
        }
    }
    /* Terminator: F1 byte with the high bit set (0xFF). */
    memset(coef + frame_count * 8, 0, 8);
    coef[frame_count * 8] = 0xFF;

    samperframe = (SAY_AMIGA_SAMPLE_RATE * frame_ms) / 1000;
    if (samperframe < 1) samperframe = 1;

    rc = say_synth_amiga_run_ex(coef, SAY_AMIGA_SAMPLE_RATE, /*rate_wpm*/ 0,
                                samperframe, out_samples, out_sample_count);
    free(coef);

    if (!rc) {
        say_set_error(error, error_size, "amiga bridge: substrate synth returned failure");
        return 0;
    }

    /* Deliver 44100 Hz output (substrate is at 11025 Hz natively). */
    if (!upsample_4x_linear(out_samples, out_sample_count, error, error_size)) {
        free(*out_samples);
        *out_samples = NULL;
        *out_sample_count = 0;
        return 0;
    }
    return 1;
}
