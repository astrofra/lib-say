#ifndef SAY_H
#define SAY_H

#include <stddef.h>
#include <stdint.h>

typedef enum say_language_t {
    SAY_LANG_EN = 0,
    SAY_LANG_FR = 1
} say_language_t;

typedef enum say_audio_format_t {
    SAY_FORMAT_RAW = 0,
    SAY_FORMAT_AIFF = 1,
    SAY_FORMAT_WAV = 2
} say_audio_format_t;

typedef struct say_options_t {
    int sample_rate;
    int frame_ms;
    say_language_t language;
    int phoneme_input;
    /* F1 — centralization (0..100). Blends every vowel's formant frequencies
     * toward the schwa target. 0 = neutral, 100 = fully reduced (everything
     * sounds like schwa). Useful for fast-speech / fatigued voice effects. */
    int centralization_pct;
    /* F2 — articulation (50..200, default 100). Scales the steady-state ratio
     * inside each segment. <100 = lazier (longer transitions, blurrier),
     * >100 = clipped/staccato (shorter transitions, more abrupt). */
    int articulation_pct;
    /* F3 — voice tilt. Multiplies all formant frequencies by voice_formants_pct
     * (80..130, default 100; ~117 for a higher / smaller-vocal-tract voice).
     * Multiplies pitch by voice_pitch_pct (50..200, default 100). Both default
     * to 100, leaving the synthesized voice unchanged. */
    int voice_formants_pct;
    int voice_pitch_pct;
} say_options_t;

void say_default_options(say_options_t *options);

const char *say_language_name(say_language_t language);
int say_parse_language(const char *name, say_language_t *out_language);

const char *say_audio_format_name(say_audio_format_t format);
int say_parse_audio_format(const char *name, say_audio_format_t *out_format);

say_audio_format_t say_guess_audio_format(const char *path);

int say_synthesize(
    const char *input,
    const say_options_t *options,
    int16_t **out_samples,
    size_t *out_sample_count,
    char *error,
    size_t error_size
);

/* P5 — Amiga substrate path. Same upstream pipeline as say_synthesize() but
 * the synthesizer is the cascade-formant LUT bank ported from the Amiga
 * narrator (clean-room C port of synth.asm). The output sample rate is
 * fixed at 22 050 Hz — the rate the impulse-response LUTs were measured at
 * — and is returned via out_sample_rate. options->sample_rate is ignored
 * for this path. */
int say_synthesize_amiga(
    const char *input,
    const say_options_t *options,
    int16_t **out_samples,
    size_t *out_sample_count,
    int *out_sample_rate,
    char *error,
    size_t error_size
);

int say_build_debug_report(
    const char *input,
    const say_options_t *options,
    char **out_report,
    char *error,
    size_t error_size
);

int say_write_audio_file(
    const char *path,
    say_audio_format_t format,
    int sample_rate,
    const int16_t *samples,
    size_t sample_count,
    char *error,
    size_t error_size
);

int say_encode_audio(
    say_audio_format_t format,
    int sample_rate,
    const int16_t *samples,
    size_t sample_count,
    uint8_t **out_data,
    size_t *out_size,
    char *error,
    size_t error_size
);

void say_free(void *ptr);

#endif
