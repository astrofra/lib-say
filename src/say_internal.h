#ifndef SAY_INTERNAL_H
#define SAY_INTERNAL_H

#include "say.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define SAY_MAX_FORMANTS 5
#define SAY_ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

/* ---------------------------------------------------------------------------
 * Phoneme inventory and feature matrix (C1)
 *
 * Splits in v0.0.4 :
 * single say.c file is broken into stage-aligned modules. The contracts below
 * are the only types and functions shared between those modules.
 * ------------------------------------------------------------------------- */

typedef enum phoneme_id_t {
    PH_PAUSE = 0,
    PH_A,
    PH_AE,
    PH_AH,
    PH_E,
    PH_EH,
    PH_I,
    PH_IH,
    PH_O,
    PH_OH,
    PH_U,
    PH_Y,
    PH_EU,
    PH_SCHWA,
    PH_AN,
    PH_ON,
    PH_IN,
    PH_W,
    PH_J,
    PH_R,
    PH_L,
    PH_M,
    PH_N,
    PH_NY,
    PH_NG,
    PH_P,
    PH_B,
    PH_T,
    PH_D,
    PH_K,
    PH_G,
    PH_F,
    PH_V,
    PH_S,
    PH_Z,
    PH_SH,
    PH_ZH,
    PH_H,
    PH_TH,
    PH_DH,
    PH_CH,
    PH_JH,
    PH_TS,
    PH_DZ,
    PH_COUNT
} phoneme_id_t;

typedef unsigned int say_features_t;

#define F_VOWEL        (1u <<  0)  /* any vowel (oral or nasal) */
#define F_VOICED       (1u <<  1)  /* voicing present during the steady state */
#define F_PLOS         (1u <<  2)  /* plosive: P B T D K G */
#define F_FRIC         (1u <<  3)  /* fricative: F V S Z SH ZH H TH DH */
#define F_AFFRIC       (1u <<  4)  /* affricate: CH JH TS DZ */
#define F_SONOR        (1u <<  5)  /* sonorant consonant: W J R L M N NY NG */
#define F_NASAL        (1u <<  6)  /* nasal consonant: M N NY NG */
#define F_GLIDE        (1u <<  7)  /* semi-vowel glide: J W */
#define F_LIQUID       (1u <<  8)  /* liquid: L R */
#define F_SIBILANT     (1u <<  9)  /* sibilant fricative: S Z SH ZH */
#define F_DENTAL       (1u << 10)  /* dental fricative: TH DH */
#define F_NASAL_VOWEL  (1u << 11)  /* French nasal vowels: AN ON IN */
#define F_PAUSE        (1u << 12)  /* the dedicated PAUSE phoneme */

typedef struct phoneme_def_t {
    phoneme_id_t id;
    const char *symbol;
    int is_vowel;
    int voiced;
    say_features_t features;
    double min_ms;
    double base_ms;
    double amplitude;
    double noise_mix;
    double formant_freq[SAY_MAX_FORMANTS];
    double bandwidth[SAY_MAX_FORMANTS];
    double gain[SAY_MAX_FORMANTS];
} phoneme_def_t;

typedef struct phoneme_noise_path_t {
    double noise_path_mix;
    double f_low;
    double f_high;
    double gain;
    double voicing_bar_amp;
} phoneme_noise_path_t;

/* ---------------------------------------------------------------------------
 * Pipeline data structures
 * ------------------------------------------------------------------------- */

typedef struct segment_t {
    phoneme_id_t phoneme;
    double duration_scale;
    int word_start;
    int word_end;
    int boundary_type;
    int weak_word;
    int stress;
    int diphthong_target;
    /* E1 — graded accent level (0 = unstressed, 1..9 = rule-emitted stress digit
     * with 4 = default primary). NRL fills this from the rule's stress digit;
     * lexicon-path words use 4 on the primary vowel. The binary `stress` field
     * stays in lock-step (stress=2 ⇔ accent_n>=1) so the rest of the code
     * keeps working unchanged. The F0 declination model in say_prosody.c reads
     * accent_n to vary peak emphasis per accented syllable. */
    int accent_n;
} segment_t;

typedef struct segment_buffer_t {
    segment_t *data;
    size_t count;
    size_t capacity;
} segment_buffer_t;

typedef struct frame_t {
    double pitch_hz;
    double amplitude;
    double noise_mix;
    int voiced;
    int is_pause;
    double formant_freq[SAY_MAX_FORMANTS];
    double bandwidth[SAY_MAX_FORMANTS];
    double gain[SAY_MAX_FORMANTS];
    double noise_path_mix;
    double noise_path_f_low;
    double noise_path_f_high;
    double noise_path_gain;
    double voicing_bar_amp;
} frame_t;

typedef struct frame_buffer_t {
    frame_t *data;
    size_t count;
    size_t capacity;
} frame_buffer_t;

typedef struct text_buffer_t {
    char *data;
    size_t count;
    size_t capacity;
} text_buffer_t;

/* ---------------------------------------------------------------------------
 * say_phonemes.c — phoneme table accessor + feature predicates
 * ------------------------------------------------------------------------- */

const phoneme_def_t *say_get_phoneme(phoneme_id_t id);
int   say_phone_has_features(phoneme_id_t id, say_features_t mask);
int   say_is_vowel_phone(phoneme_id_t id);
int   say_is_glide_phone(phoneme_id_t id);
int   say_is_plosive_phone(phoneme_id_t id);
int   say_is_fricative_phone(phoneme_id_t id);
int   say_is_affricate_phone(phoneme_id_t id);
int   say_is_sonorant_phone(phoneme_id_t id);
int   say_is_nasal_vowel_phone(phoneme_id_t id);
int   say_is_sibilant_phone(phoneme_id_t id);
int   say_is_dental_fricative_phone(phoneme_id_t id);
int   say_is_voiced_fricative_phone(phoneme_id_t id);
int   say_is_voiced_plosive_phone(phoneme_id_t id);
int   say_is_voiceless_plosive_phone(phoneme_id_t id);
const phoneme_def_t *say_lookup_symbol(const char *symbol);
phoneme_noise_path_t say_get_noise_path_en(phoneme_id_t id);

/* ---------------------------------------------------------------------------
 * say_text.c — buffer utilities + phonemizer
 * ------------------------------------------------------------------------- */

int   say_segment_buffer_reserve(segment_buffer_t *buffer, size_t extra);
int   say_segment_buffer_push(segment_buffer_t *buffer, phoneme_id_t phoneme,
                              double duration_scale, int boundary_type);
int   say_segment_buffer_append(segment_buffer_t *buffer, const segment_buffer_t *extra);

int   say_frame_buffer_reserve(frame_buffer_t *buffer, size_t extra);
int   say_frame_buffer_push(frame_buffer_t *buffer, const frame_t *frame);

int   say_text_buffer_reserve(text_buffer_t *buffer, size_t extra);
int   say_text_buffer_append(text_buffer_t *buffer, const char *text);
int   say_text_buffer_appendf(text_buffer_t *buffer, const char *fmt, ...);

int   say_is_boundary_char(char c);
int   say_is_word_char(char c);
int   say_is_token_char(char c);
int   say_equals_icase(const char *a, const char *b);
size_t say_utf8_decode(const unsigned char *text, unsigned int *codepoint);
char *say_normalize_text(const char *text, char *error, size_t error_size);

int   say_phonemize_text(const char *normalized_text, say_language_t language,
                         segment_buffer_t *segments, char *error, size_t error_size);
int   say_parse_phoneme_input(const char *input, segment_buffer_t *segments,
                              char *error, size_t error_size);

int   say_append_text_word(const char *word, say_language_t language,
                           segment_buffer_t *segments);
const char *say_digit_word(char digit, say_language_t language);

/* D1/D2 — NRL letter-to-sound rule engine (English). Tries the bundled reference
 * rule set against `word`, appending the resulting PH_* segments. Returns 1
 * on success and a non-empty emission, 0 otherwise. The caller is responsible
 * for word_start / word_end metadata after this returns. */
int   say_phonemize_english_nrl(const char *word, segment_buffer_t *segments);

/* ---------------------------------------------------------------------------
 * say_prosody.c — clause analysis, F0, duration cascade, frame generation
 * ------------------------------------------------------------------------- */

int   say_find_next_non_pause(const segment_t *segments, size_t count, size_t start_index);
int   say_find_prev_non_pause(const segment_t *segments, size_t end_index);
double say_pause_duration_ms(int boundary_type);

void  say_fuse_english_diphthongs(segment_buffer_t *segments);
int   say_generate_frames(const segment_t *segments, size_t segment_count,
                          const say_options_t *options, frame_buffer_t *frames,
                          char *error, size_t error_size);

/* ---------------------------------------------------------------------------
 * say_synth.c — biquad bank + glottal pulse + sample-level synthesis
 * ------------------------------------------------------------------------- */

int   say_synthesize_frames(const frame_t *frames, size_t frame_count,
                            int sample_rate, int frame_ms,
                            int16_t **out_samples, size_t *out_sample_count,
                            char *error, size_t error_size);

/* say_audio_io.c implements the public say_write_audio_file / say_encode_audio
 * declared in say.h. Private writers / IEEE 754 helpers are static to that
 * translation unit. */

/* ---------------------------------------------------------------------------
 * Tiny math helpers used by both prosody and synth
 * ------------------------------------------------------------------------- */

static inline double say_clamp01(double value)
{
    if (value < 0.0) {
        return 0.0;
    }
    if (value > 1.0) {
        return 1.0;
    }
    return value;
}

static inline double say_smoothstep01(double value)
{
    value = say_clamp01(value);
    return value * value * (3.0 - 2.0 * value);
}

static inline double say_lerp(double a, double b, double t)
{
    return a + (b - a) * t;
}

/* ---------------------------------------------------------------------------
 * say.c — error reporting helper used by every module
 * ------------------------------------------------------------------------- */

void  say_set_error(char *error, size_t error_size, const char *fmt, ...);

#endif /* SAY_INTERNAL_H */
