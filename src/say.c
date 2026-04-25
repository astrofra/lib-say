#define _CRT_SECURE_NO_WARNINGS

#include "say.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAY_MAX_FORMANTS 5
#define SAY_ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

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

typedef struct phoneme_def_t {
    phoneme_id_t id;
    const char *symbol;
    int is_vowel;
    int voiced;
    double base_ms;
    double amplitude;
    double noise_mix;
    double formant_freq[SAY_MAX_FORMANTS];
    double bandwidth[SAY_MAX_FORMANTS];
    double gain[SAY_MAX_FORMANTS];
} phoneme_def_t;

typedef struct segment_t {
    phoneme_id_t phoneme;
    double duration_scale;
    int word_start;
    int word_end;
    int boundary_type;
    int weak_word;
    int stress;
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

typedef struct biquad_t {
    double b0;
    double b1;
    double b2;
    double a1;
    double a2;
    double z1;
    double z2;
} biquad_t;

typedef struct lexicon_entry_t {
    const char *word;
    const phoneme_id_t *phonemes;
    size_t phoneme_count;
    double duration_scale;
    int weak_word;
    int primary_stress_vowel;
} lexicon_entry_t;

typedef struct phoneme_pattern_rule_t {
    const char *pattern;
    const phoneme_id_t *phonemes;
    size_t phoneme_count;
    int initial_only;
    int final_only;
} phoneme_pattern_rule_t;

typedef struct elision_prefix_t {
    const char *text;
    const phoneme_id_t *phonemes;
    size_t phoneme_count;
    double duration_scale;
} elision_prefix_t;

typedef enum prosody_role_t {
    SAY_PROSODY_PREHEAD = 0,
    SAY_PROSODY_HEAD,
    SAY_PROSODY_NUCLEUS,
    SAY_PROSODY_TAIL
} prosody_role_t;

typedef struct clause_prosody_t {
    size_t start;
    size_t end;
    int boundary_type;
    size_t first_vowel;
    size_t last_vowel;
    size_t first_anchor;
    size_t nucleus;
    size_t anchor_count;
} clause_prosody_t;

typedef struct prosody_tune_t {
    double register_base;
    double prehead_start;
    double prehead_end;
    double head_start;
    double head_end;
    double head_anchor_bump;
    double head_unstressed_drop;
    double nucleus_start;
    double nucleus_end;
    double nucleus_no_tail_end;
    double tail_start;
    double tail_end;
} prosody_tune_t;

static const phoneme_def_t g_phonemes[PH_COUNT] = {
    { PH_PAUSE, "PAUSE", 0, 0, 90.0, 0.0, 0.0, { 0 }, { 0 }, { 0 } },
    { PH_A, "A", 1, 1, 115.0, 1.00, 0.02, { 800, 1200, 2800, 3600, 4500 }, { 90, 100, 150, 200, 260 }, { 1.00, 0.85, 0.35, 0.16, 0.08 } },
    { PH_AE, "AE", 1, 1, 110.0, 1.00, 0.02, { 700, 1700, 2500, 3600, 4500 }, { 90, 110, 150, 200, 260 }, { 1.00, 0.82, 0.34, 0.15, 0.07 } },
    { PH_AH, "AH", 1, 1, 105.0, 0.96, 0.03, { 650, 1200, 2500, 3400, 4400 }, { 100, 120, 160, 200, 260 }, { 0.98, 0.78, 0.30, 0.14, 0.07 } },
    { PH_E, "E", 1, 1, 108.0, 0.98, 0.02, { 500, 1900, 2600, 3400, 4300 }, { 90, 100, 130, 180, 240 }, { 0.98, 0.84, 0.34, 0.14, 0.07 } },
    { PH_EH, "EH", 1, 1, 104.0, 0.98, 0.02, { 600, 1700, 2500, 3400, 4300 }, { 95, 105, 135, 185, 240 }, { 0.98, 0.83, 0.34, 0.15, 0.07 } },
    { PH_I, "I", 1, 1, 108.0, 0.98, 0.02, { 300, 2200, 3000, 3800, 4700 }, { 70, 95, 120, 180, 240 }, { 0.96, 0.82, 0.32, 0.14, 0.06 } },
    { PH_IH, "IH", 1, 1, 96.0, 0.92, 0.02, { 400, 2000, 2800, 3600, 4500 }, { 80, 100, 130, 185, 240 }, { 0.92, 0.78, 0.30, 0.13, 0.06 } },
    { PH_O, "O", 1, 1, 110.0, 1.00, 0.02, { 450, 900, 2400, 3400, 4300 }, { 90, 100, 140, 200, 240 }, { 1.00, 0.84, 0.34, 0.14, 0.06 } },
    { PH_OH, "OH", 1, 1, 108.0, 0.98, 0.02, { 550, 900, 2400, 3400, 4300 }, { 95, 105, 145, 200, 240 }, { 0.98, 0.84, 0.34, 0.14, 0.06 } },
    { PH_U, "U", 1, 1, 112.0, 0.96, 0.02, { 350, 800, 2200, 3200, 4200 }, { 70, 95, 130, 190, 240 }, { 0.96, 0.80, 0.30, 0.13, 0.06 } },
    { PH_Y, "Y", 1, 1, 110.0, 0.96, 0.02, { 320, 1750, 2400, 3300, 4200 }, { 75, 100, 130, 180, 240 }, { 0.96, 0.80, 0.30, 0.13, 0.06 } },
    { PH_EU, "EU", 1, 1, 106.0, 0.95, 0.02, { 400, 1500, 2400, 3300, 4200 }, { 80, 100, 130, 180, 240 }, { 0.95, 0.80, 0.30, 0.13, 0.06 } },
    { PH_SCHWA, "SCHWA", 1, 1, 85.0, 0.88, 0.03, { 500, 1500, 2500, 3400, 4300 }, { 100, 120, 160, 210, 260 }, { 0.88, 0.74, 0.28, 0.12, 0.06 } },
    { PH_AN, "AN", 1, 1, 118.0, 0.96, 0.04, { 650, 1100, 2300, 3200, 4100 }, { 110, 130, 170, 210, 260 }, { 0.94, 0.76, 0.30, 0.12, 0.06 } },
    { PH_ON, "ON", 1, 1, 116.0, 0.96, 0.04, { 500, 900, 2200, 3200, 4100 }, { 110, 130, 170, 210, 260 }, { 0.94, 0.76, 0.30, 0.12, 0.06 } },
    { PH_IN, "IN", 1, 1, 114.0, 0.95, 0.04, { 450, 1800, 2500, 3400, 4300 }, { 110, 125, 160, 210, 260 }, { 0.94, 0.80, 0.30, 0.12, 0.06 } },
    { PH_W, "W", 0, 1, 72.0, 0.70, 0.03, { 320, 800, 2200, 3000, 3900 }, { 110, 130, 180, 220, 280 }, { 0.72, 0.56, 0.18, 0.07, 0.03 } },
    { PH_J, "J", 0, 1, 68.0, 0.68, 0.04, { 320, 2100, 2900, 3600, 4400 }, { 110, 130, 180, 220, 280 }, { 0.70, 0.58, 0.18, 0.07, 0.03 } },
    { PH_R, "R", 0, 1, 76.0, 0.72, 0.08, { 300, 1300, 1800, 2600, 3500 }, { 110, 140, 190, 240, 300 }, { 0.74, 0.56, 0.20, 0.08, 0.03 } },
    { PH_L, "L", 0, 1, 76.0, 0.70, 0.04, { 350, 1500, 2600, 3400, 4300 }, { 110, 130, 180, 220, 280 }, { 0.72, 0.56, 0.18, 0.07, 0.03 } },
    { PH_M, "M", 0, 1, 78.0, 0.72, 0.05, { 250, 1200, 2100, 2900, 3800 }, { 130, 150, 200, 240, 300 }, { 0.74, 0.54, 0.18, 0.07, 0.03 } },
    { PH_N, "N", 0, 1, 76.0, 0.72, 0.05, { 300, 1500, 2500, 3400, 4300 }, { 130, 150, 200, 240, 300 }, { 0.74, 0.54, 0.18, 0.07, 0.03 } },
    { PH_NY, "NY", 0, 1, 78.0, 0.72, 0.06, { 300, 1800, 2600, 3500, 4300 }, { 130, 150, 200, 240, 300 }, { 0.74, 0.54, 0.18, 0.07, 0.03 } },
    { PH_NG, "NG", 0, 1, 78.0, 0.72, 0.06, { 300, 1400, 2200, 3000, 3800 }, { 130, 150, 200, 240, 300 }, { 0.74, 0.54, 0.18, 0.07, 0.03 } },
    { PH_P, "P", 0, 0, 68.0, 0.62, 1.00, { 500, 1400, 2400, 3500, 4500 }, { 300, 340, 380, 420, 500 }, { 0.40, 0.28, 0.24, 0.20, 0.14 } },
    { PH_B, "B", 0, 1, 70.0, 0.64, 0.36, { 500, 1400, 2400, 3500, 4500 }, { 300, 340, 380, 420, 500 }, { 0.42, 0.30, 0.24, 0.20, 0.14 } },
    { PH_T, "T", 0, 0, 66.0, 0.62, 1.00, { 600, 1700, 2800, 3800, 4800 }, { 280, 330, 380, 420, 500 }, { 0.40, 0.28, 0.24, 0.20, 0.14 } },
    { PH_D, "D", 0, 1, 68.0, 0.64, 0.34, { 600, 1700, 2800, 3800, 4800 }, { 280, 330, 380, 420, 500 }, { 0.42, 0.30, 0.24, 0.20, 0.14 } },
    { PH_K, "K", 0, 0, 70.0, 0.62, 1.00, { 700, 1800, 3000, 3900, 4900 }, { 280, 330, 380, 420, 500 }, { 0.40, 0.28, 0.24, 0.20, 0.14 } },
    { PH_G, "G", 0, 1, 72.0, 0.64, 0.36, { 700, 1800, 3000, 3900, 4900 }, { 280, 330, 380, 420, 500 }, { 0.42, 0.30, 0.24, 0.20, 0.14 } },
    { PH_F, "F", 0, 0, 82.0, 0.66, 1.00, { 900, 1800, 2900, 3900, 5000 }, { 250, 290, 340, 390, 450 }, { 0.42, 0.34, 0.28, 0.22, 0.16 } },
    { PH_V, "V", 0, 1, 84.0, 0.68, 0.42, { 900, 1800, 2900, 3900, 5000 }, { 250, 290, 340, 390, 450 }, { 0.44, 0.34, 0.28, 0.22, 0.16 } },
    { PH_S, "S", 0, 0, 92.0, 0.60, 1.00, { 1200, 2800, 4200, 5400, 6700 }, { 230, 300, 380, 470, 560 }, { 0.24, 0.32, 0.48, 0.52, 0.34 } },
    { PH_Z, "Z", 0, 1, 90.0, 0.60, 0.44, { 1200, 2800, 4200, 5400, 6700 }, { 230, 300, 380, 470, 560 }, { 0.26, 0.32, 0.46, 0.48, 0.30 } },
    { PH_SH, "SH", 0, 0, 94.0, 0.62, 1.00, { 1050, 2400, 3500, 4650, 5900 }, { 230, 300, 380, 470, 560 }, { 0.24, 0.42, 0.54, 0.44, 0.26 } },
    { PH_ZH, "ZH", 0, 1, 92.0, 0.62, 0.44, { 1050, 2400, 3500, 4650, 5900 }, { 230, 300, 380, 470, 560 }, { 0.26, 0.40, 0.50, 0.40, 0.24 } },
    { PH_H, "H", 0, 0, 68.0, 0.48, 0.68, { 1000, 1700, 2600, 3500, 4400 }, { 260, 310, 360, 410, 470 }, { 0.34, 0.26, 0.20, 0.16, 0.12 } },
    { PH_TH, "TH", 0, 0, 90.0, 0.72, 1.00, { 900, 2000, 3800, 6000, 8000 }, { 280, 360, 460, 580, 700 }, { 0.12, 0.22, 0.32, 0.46, 0.62 } },
    { PH_DH, "DH", 0, 1, 90.0, 0.60, 0.44, { 950, 2200, 3300, 4400, 5600 }, { 250, 320, 400, 500, 620 }, { 0.30, 0.34, 0.34, 0.28, 0.16 } },
    { PH_CH, "CH", 0, 0, 92.0, 0.68, 1.00, { 1300, 2600, 3700, 4800, 5900 }, { 220, 270, 320, 370, 420 }, { 0.20, 0.36, 0.54, 0.42, 0.24 } },
    { PH_JH, "JH", 0, 1, 92.0, 0.70, 0.48, { 1300, 2600, 3700, 4800, 5900 }, { 220, 270, 320, 370, 420 }, { 0.22, 0.34, 0.52, 0.40, 0.22 } },
    { PH_TS, "TS", 0, 0, 90.0, 0.66, 1.00, { 1200, 2500, 3600, 4700, 5800 }, { 220, 270, 320, 370, 420 }, { 0.46, 0.38, 0.30, 0.24, 0.18 } },
    { PH_DZ, "DZ", 0, 1, 90.0, 0.68, 0.48, { 1200, 2500, 3600, 4700, 5800 }, { 220, 270, 320, 370, 420 }, { 0.48, 0.38, 0.30, 0.24, 0.18 } }
};

typedef struct phoneme_noise_path_t {
    double noise_path_mix;
    double f_low;
    double f_high;
    double gain;
    double voicing_bar_amp;
} phoneme_noise_path_t;

static phoneme_noise_path_t say_get_noise_path_en(phoneme_id_t id)
{
    phoneme_noise_path_t p;
    memset(&p, 0, sizeof(p));
    /* Dedicated HP+LP flat-spectrum noise path, bypassing the formant filter bank.
     * noise_path_mix=1.0 fully replaces formant noise; fractional values blend.
     * Formant-bank voiced output is always preserved (affects DH, V, ZH). */
    /* The dedicated noise path is acoustically correct (flat HP+LP spectrum) but this
     * extractor's acoustic models are calibrated on formant-bank output. Any flat-spectrum
     * noise — regardless of bandwidth — is misidentified as S, causing regressions on all
     * tested phonemes (TH, DH, V, ZH). Infrastructure kept for future extractors. */
    (void) id;
    return p;
}

static const phoneme_id_t g_word_en_changing[] = { PH_CH, PH_E, PH_J, PH_N, PH_JH, PH_IH, PH_NG };
static const phoneme_id_t g_word_en_church[] = { PH_CH, PH_R, PH_CH };
static const phoneme_id_t g_word_en_demo[] = { PH_D, PH_EH, PH_M, PH_OH, PH_W };
static const phoneme_id_t g_word_en_fricatives[] = { PH_F, PH_R, PH_I, PH_K, PH_SCHWA, PH_T, PH_IH, PH_V, PH_Z };
static const phoneme_id_t g_word_en_hello[] = { PH_H, PH_SCHWA, PH_L, PH_OH, PH_W };
static const phoneme_id_t g_word_en_be[] = { PH_B, PH_I };
static const phoneme_id_t g_word_en_by[] = { PH_B, PH_AH, PH_J };
static const phoneme_id_t g_word_en_clothes[] = { PH_K, PH_L, PH_OH, PH_W, PH_DH, PH_Z };
static const phoneme_id_t g_word_en_from[] = { PH_F, PH_R, PH_AH, PH_M };
static const phoneme_id_t g_word_en_effort[] = { PH_EH, PH_F, PH_R, PH_T };
static const phoneme_id_t g_word_en_feathers[] = { PH_F, PH_EH, PH_DH, PH_R, PH_Z };
static const phoneme_id_t g_word_en_gather[] = { PH_G, PH_AE, PH_DH, PH_R };
static const phoneme_id_t g_word_en_this[] = { PH_DH, PH_IH, PH_S };
static const phoneme_id_t g_word_en_is[] = { PH_IH, PH_Z };
static const phoneme_id_t g_word_en_an[] = { PH_AE, PH_N };
static const phoneme_id_t g_word_en_english[] = { PH_IH, PH_NG, PH_G, PH_L, PH_IH, PH_SH };
static const phoneme_id_t g_word_en_not[] = { PH_N, PH_A, PH_T };
static const phoneme_id_t g_word_en_question[] = { PH_K, PH_W, PH_EH, PH_S, PH_CH, PH_SCHWA, PH_N };
static const phoneme_id_t g_word_en_sentence[] = { PH_S, PH_EH, PH_N, PH_T, PH_SCHWA, PH_N, PH_S };
static const phoneme_id_t g_word_en_she[] = { PH_SH, PH_I };
static const phoneme_id_t g_word_en_sharply[] = { PH_SH, PH_A, PH_R, PH_P, PH_L, PH_I };
static const phoneme_id_t g_word_en_the[] = { PH_DH, PH_SCHWA };
static const phoneme_id_t g_word_en_these[] = { PH_DH, PH_I, PH_Z };
static const phoneme_id_t g_word_en_those[] = { PH_DH, PH_OH, PH_W, PH_Z };
static const phoneme_id_t g_word_en_to[] = { PH_T, PH_U };
static const phoneme_id_t g_word_en_worth[] = { PH_W, PH_R, PH_TH };
static const phoneme_id_t g_word_en_both[] = { PH_B, PH_OH, PH_TH };
static const phoneme_id_t g_word_en_teeth[] = { PH_T, PH_I, PH_TH };
static const phoneme_id_t g_word_en_i[] = { PH_AH, PH_J };
static const phoneme_id_t g_word_en_you[] = { PH_J, PH_U };
static const phoneme_id_t g_word_en_we[] = { PH_W, PH_I };
static const phoneme_id_t g_word_en_they[] = { PH_DH, PH_E, PH_J };
static const phoneme_id_t g_word_en_he[] = { PH_H, PH_I };
static const phoneme_id_t g_word_en_are[] = { PH_AH, PH_R };
static const phoneme_id_t g_word_en_have[] = { PH_H, PH_AE, PH_V };
static const phoneme_id_t g_word_en_will[] = { PH_W, PH_IH, PH_L };
static const phoneme_id_t g_word_en_am[] = { PH_AE, PH_M };
static const phoneme_id_t g_word_en_it[] = { PH_IH, PH_T };
static const phoneme_id_t g_word_en_cant[] = { PH_K, PH_AE, PH_N, PH_T };
static const phoneme_id_t g_word_en_dont[] = { PH_D, PH_OH, PH_W, PH_N, PH_T };
static const phoneme_id_t g_word_en_wont[] = { PH_W, PH_OH, PH_N, PH_T };
static const phoneme_id_t g_word_en_isnt[] = { PH_IH, PH_Z, PH_N, PH_T };
static const phoneme_id_t g_word_en_arent[] = { PH_AH, PH_R, PH_N, PH_T };
static const phoneme_id_t g_word_en_wasnt[] = { PH_W, PH_AH, PH_Z, PH_N, PH_T };
static const phoneme_id_t g_word_en_werent[] = { PH_W, PH_EH, PH_R, PH_N, PH_T };
static const phoneme_id_t g_word_en_hasnt[] = { PH_H, PH_AE, PH_Z, PH_N, PH_T };
static const phoneme_id_t g_word_en_havent[] = { PH_H, PH_AE, PH_V, PH_N, PH_T };
static const phoneme_id_t g_word_en_hadnt[] = { PH_H, PH_AE, PH_D, PH_N, PH_T };
static const phoneme_id_t g_word_en_doesnt[] = { PH_D, PH_AH, PH_Z, PH_N, PH_T };
static const phoneme_id_t g_word_en_didnt[] = { PH_D, PH_IH, PH_D, PH_N, PH_T };
static const phoneme_id_t g_word_en_couldnt[] = { PH_K, PH_U, PH_D, PH_N, PH_T };
static const phoneme_id_t g_word_en_wouldnt[] = { PH_W, PH_U, PH_D, PH_N, PH_T };
static const phoneme_id_t g_word_en_shouldnt[] = { PH_SH, PH_U, PH_D, PH_N, PH_T };
static const phoneme_id_t g_word_en_im[] = { PH_AH, PH_J, PH_M };
static const phoneme_id_t g_word_en_ive[] = { PH_AH, PH_J, PH_V };
static const phoneme_id_t g_word_en_ill[] = { PH_AH, PH_J, PH_L };
static const phoneme_id_t g_word_en_id[] = { PH_AH, PH_J, PH_D };
static const phoneme_id_t g_word_en_youre[] = { PH_J, PH_U, PH_R };
static const phoneme_id_t g_word_en_youve[] = { PH_J, PH_U, PH_V };
static const phoneme_id_t g_word_en_youll[] = { PH_J, PH_U, PH_L };
static const phoneme_id_t g_word_en_youd[] = { PH_J, PH_U, PH_D };
static const phoneme_id_t g_word_en_were[] = { PH_W, PH_I, PH_R };
static const phoneme_id_t g_word_en_weve[] = { PH_W, PH_I, PH_V };
static const phoneme_id_t g_word_en_well[] = { PH_W, PH_I, PH_L };
static const phoneme_id_t g_word_en_wed[] = { PH_W, PH_I, PH_D };
static const phoneme_id_t g_word_en_theyre[] = { PH_DH, PH_E, PH_J, PH_R };
static const phoneme_id_t g_word_en_theyve[] = { PH_DH, PH_E, PH_J, PH_V };
static const phoneme_id_t g_word_en_theyll[] = { PH_DH, PH_E, PH_J, PH_L };
static const phoneme_id_t g_word_en_theyd[] = { PH_DH, PH_E, PH_J, PH_D };
static const phoneme_id_t g_word_en_its[] = { PH_IH, PH_T, PH_S };
static const phoneme_id_t g_word_en_thats[] = { PH_DH, PH_AE, PH_T, PH_S };
static const phoneme_id_t g_word_en_theres[] = { PH_DH, PH_EH, PH_R, PH_Z };
static const phoneme_id_t g_word_en_lets[] = { PH_L, PH_EH, PH_T, PH_S };

static const phoneme_id_t g_word_fr_bonjour[] = { PH_B, PH_ON, PH_ZH, PH_U, PH_R };
static const phoneme_id_t g_word_fr_je[] = { PH_ZH, PH_SCHWA };
static const phoneme_id_t g_word_fr_le[] = { PH_L, PH_SCHWA };
static const phoneme_id_t g_word_fr_ce[] = { PH_S, PH_SCHWA };
static const phoneme_id_t g_word_fr_ne[] = { PH_N, PH_SCHWA };
static const phoneme_id_t g_word_fr_vous[] = { PH_V, PH_U };
static const phoneme_id_t g_word_fr_depuis[] = { PH_D, PH_SCHWA, PH_P, PH_J, PH_I };
static const phoneme_id_t g_word_fr_ceci[] = { PH_S, PH_SCHWA, PH_S, PH_I };
static const phoneme_id_t g_word_fr_est[] = { PH_EH };
static const phoneme_id_t g_word_fr_une[] = { PH_Y, PH_N };
static const phoneme_id_t g_word_fr_phrase[] = { PH_F, PH_R, PH_A, PH_Z };
static const phoneme_id_t g_word_fr_demonstration[] = { PH_D, PH_E, PH_M, PH_ON, PH_S, PH_T, PH_R, PH_A, PH_S, PH_J, PH_ON };
static const phoneme_id_t g_word_fr_de[] = { PH_D, PH_SCHWA };
static const phoneme_id_t g_word_fr_en[] = { PH_AN };
static const phoneme_id_t g_word_fr_francais[] = { PH_F, PH_R, PH_AN, PH_S, PH_E };
static const phoneme_id_t g_word_fr_lib[] = { PH_L, PH_I, PH_B };
static const phoneme_id_t g_word_fr_say[] = { PH_S, PH_E, PH_J };
static const phoneme_id_t g_word_fr_aujourdhui[] = { PH_O, PH_ZH, PH_U, PH_R, PH_D, PH_Y, PH_I };

static const phoneme_id_t g_rule_en_tion[] = { PH_SH, PH_SCHWA, PH_N };
static const phoneme_id_t g_rule_en_sion[] = { PH_ZH, PH_SCHWA, PH_N };
static const phoneme_id_t g_rule_en_tch[] = { PH_CH };
static const phoneme_id_t g_rule_en_dge[] = { PH_JH };
static const phoneme_id_t g_rule_en_igh[] = { PH_AH, PH_J };
static const phoneme_id_t g_rule_en_ee[] = { PH_I };
static const phoneme_id_t g_rule_en_oo[] = { PH_U };
static const phoneme_id_t g_rule_en_ow[] = { PH_AH, PH_W };
static const phoneme_id_t g_rule_en_oi[] = { PH_OH, PH_J };
static const phoneme_id_t g_rule_en_ai[] = { PH_E, PH_J };
static const phoneme_id_t g_rule_en_ph[] = { PH_F };
static const phoneme_id_t g_rule_en_sh[] = { PH_SH };
static const phoneme_id_t g_rule_en_ch[] = { PH_CH };
static const phoneme_id_t g_rule_en_ng[] = { PH_NG };
static const phoneme_id_t g_rule_en_qu[] = { PH_K, PH_W };
static const phoneme_id_t g_rule_en_ck[] = { PH_K };
static const phoneme_id_t g_rule_en_wr[] = { PH_R };
static const phoneme_id_t g_rule_en_wh[] = { PH_W };
static const phoneme_id_t g_rule_en_kn[] = { PH_N };

static const phoneme_id_t g_rule_fr_tion[] = { PH_S, PH_J, PH_ON };
static const phoneme_id_t g_rule_fr_eau[] = { PH_O };
static const phoneme_id_t g_rule_fr_ou[] = { PH_U };
static const phoneme_id_t g_rule_fr_oi[] = { PH_W, PH_A };
static const phoneme_id_t g_rule_fr_oy[] = { PH_W, PH_A, PH_I };
static const phoneme_id_t g_rule_fr_eu[] = { PH_EU };
static const phoneme_id_t g_rule_fr_au[] = { PH_O };
static const phoneme_id_t g_rule_fr_ai[] = { PH_E };
static const phoneme_id_t g_rule_fr_gn[] = { PH_NY };
static const phoneme_id_t g_rule_fr_ch[] = { PH_SH };
static const phoneme_id_t g_rule_fr_ph[] = { PH_F };
static const phoneme_id_t g_rule_fr_qu[] = { PH_K };

static const phoneme_id_t g_elision_fr_c[] = { PH_S };
static const phoneme_id_t g_elision_fr_d[] = { PH_D };
static const phoneme_id_t g_elision_fr_j[] = { PH_ZH };
static const phoneme_id_t g_elision_fr_l[] = { PH_L };
static const phoneme_id_t g_elision_fr_m[] = { PH_M };
static const phoneme_id_t g_elision_fr_n[] = { PH_N };
static const phoneme_id_t g_elision_fr_qu[] = { PH_K };
static const phoneme_id_t g_elision_fr_s[] = { PH_S };
static const phoneme_id_t g_elision_fr_t[] = { PH_T };

static const lexicon_entry_t g_english_lexicon[] = {
    { "an", g_word_en_an, sizeof(g_word_en_an) / sizeof(g_word_en_an[0]), 0.74, 1, 0 },
    { "be", g_word_en_be, sizeof(g_word_en_be) / sizeof(g_word_en_be[0]), 0.82, 1, 0 },
    { "both", g_word_en_both, sizeof(g_word_en_both) / sizeof(g_word_en_both[0]), 0.94, 0, 1 },
    { "by", g_word_en_by, sizeof(g_word_en_by) / sizeof(g_word_en_by[0]), 0.94, 0, 1 },
    { "can't", g_word_en_cant, sizeof(g_word_en_cant) / sizeof(g_word_en_cant[0]), 0.96, 0, 1 },
    { "changing", g_word_en_changing, sizeof(g_word_en_changing) / sizeof(g_word_en_changing[0]), 1.00, 0, 1 },
    { "church", g_word_en_church, sizeof(g_word_en_church) / sizeof(g_word_en_church[0]), 0.96, 0, 1 },
    { "clothes", g_word_en_clothes, sizeof(g_word_en_clothes) / sizeof(g_word_en_clothes[0]), 1.00, 0, 1 },
    { "couldn't", g_word_en_couldnt, sizeof(g_word_en_couldnt) / sizeof(g_word_en_couldnt[0]), 0.94, 0, 1 },
    { "demo", g_word_en_demo, sizeof(g_word_en_demo) / sizeof(g_word_en_demo[0]), 0.88, 0, 1 },
    { "didn't", g_word_en_didnt, sizeof(g_word_en_didnt) / sizeof(g_word_en_didnt[0]), 0.94, 0, 1 },
    { "doesn't", g_word_en_doesnt, sizeof(g_word_en_doesnt) / sizeof(g_word_en_doesnt[0]), 0.94, 0, 1 },
    { "don't", g_word_en_dont, sizeof(g_word_en_dont) / sizeof(g_word_en_dont[0]), 0.94, 0, 1 },
    { "english", g_word_en_english, sizeof(g_word_en_english) / sizeof(g_word_en_english[0]), 1.00, 0, 1 },
    { "effort", g_word_en_effort, sizeof(g_word_en_effort) / sizeof(g_word_en_effort[0]), 1.00, 0, 1 },
    { "feathers", g_word_en_feathers, sizeof(g_word_en_feathers) / sizeof(g_word_en_feathers[0]), 1.00, 0, 1 },
    { "fricatives", g_word_en_fricatives, sizeof(g_word_en_fricatives) / sizeof(g_word_en_fricatives[0]), 0.96, 0, 1 },
    { "from", g_word_en_from, sizeof(g_word_en_from) / sizeof(g_word_en_from[0]), 0.86, 1, 0 },
    { "gather", g_word_en_gather, sizeof(g_word_en_gather) / sizeof(g_word_en_gather[0]), 1.00, 0, 1 },
    { "hadn't", g_word_en_hadnt, sizeof(g_word_en_hadnt) / sizeof(g_word_en_hadnt[0]), 0.94, 0, 1 },
    { "hasn't", g_word_en_hasnt, sizeof(g_word_en_hasnt) / sizeof(g_word_en_hasnt[0]), 0.94, 0, 1 },
    { "haven't", g_word_en_havent, sizeof(g_word_en_havent) / sizeof(g_word_en_havent[0]), 0.94, 0, 1 },
    { "he", g_word_en_he, sizeof(g_word_en_he) / sizeof(g_word_en_he[0]), 0.76, 1, 0 },
    { "hello", g_word_en_hello, sizeof(g_word_en_hello) / sizeof(g_word_en_hello[0]), 0.86, 0, 2 },
    { "i", g_word_en_i, sizeof(g_word_en_i) / sizeof(g_word_en_i[0]), 0.78, 1, 0 },
    { "i'd", g_word_en_id, sizeof(g_word_en_id) / sizeof(g_word_en_id[0]), 0.78, 1, 0 },
    { "i'll", g_word_en_ill, sizeof(g_word_en_ill) / sizeof(g_word_en_ill[0]), 0.78, 1, 0 },
    { "i'm", g_word_en_im, sizeof(g_word_en_im) / sizeof(g_word_en_im[0]), 0.78, 1, 0 },
    { "i've", g_word_en_ive, sizeof(g_word_en_ive) / sizeof(g_word_en_ive[0]), 0.78, 1, 0 },
    { "is", g_word_en_is, sizeof(g_word_en_is) / sizeof(g_word_en_is[0]), 0.78, 1, 0 },
    { "isn't", g_word_en_isnt, sizeof(g_word_en_isnt) / sizeof(g_word_en_isnt[0]), 0.92, 0, 1 },
    { "it", g_word_en_it, sizeof(g_word_en_it) / sizeof(g_word_en_it[0]), 0.76, 1, 0 },
    { "it's", g_word_en_its, sizeof(g_word_en_its) / sizeof(g_word_en_its[0]), 0.76, 1, 0 },
    { "let's", g_word_en_lets, sizeof(g_word_en_lets) / sizeof(g_word_en_lets[0]), 0.82, 1, 0 },
    { "not", g_word_en_not, sizeof(g_word_en_not) / sizeof(g_word_en_not[0]), 0.94, 0, 1 },
    { "question", g_word_en_question, sizeof(g_word_en_question) / sizeof(g_word_en_question[0]), 1.00, 0, 1 },
    { "shouldn't", g_word_en_shouldnt, sizeof(g_word_en_shouldnt) / sizeof(g_word_en_shouldnt[0]), 0.94, 0, 1 },
    { "sentence", g_word_en_sentence, sizeof(g_word_en_sentence) / sizeof(g_word_en_sentence[0]), 1.00, 0, 1 },
    { "she", g_word_en_she, sizeof(g_word_en_she) / sizeof(g_word_en_she[0]), 0.94, 0, 1 },
    { "sharply", g_word_en_sharply, sizeof(g_word_en_sharply) / sizeof(g_word_en_sharply[0]), 0.96, 0, 1 },
    { "teeth", g_word_en_teeth, sizeof(g_word_en_teeth) / sizeof(g_word_en_teeth[0]), 0.96, 0, 1 },
    { "that's", g_word_en_thats, sizeof(g_word_en_thats) / sizeof(g_word_en_thats[0]), 0.90, 0, 1 },
    { "the", g_word_en_the, sizeof(g_word_en_the) / sizeof(g_word_en_the[0]), 0.72, 1, 0 },
    { "there's", g_word_en_theres, sizeof(g_word_en_theres) / sizeof(g_word_en_theres[0]), 0.84, 0, 1 },
    { "they", g_word_en_they, sizeof(g_word_en_they) / sizeof(g_word_en_they[0]), 0.86, 1, 0 },
    { "they'd", g_word_en_theyd, sizeof(g_word_en_theyd) / sizeof(g_word_en_theyd[0]), 0.84, 1, 0 },
    { "they'll", g_word_en_theyll, sizeof(g_word_en_theyll) / sizeof(g_word_en_theyll[0]), 0.84, 1, 0 },
    { "they're", g_word_en_theyre, sizeof(g_word_en_theyre) / sizeof(g_word_en_theyre[0]), 0.84, 1, 0 },
    { "they've", g_word_en_theyve, sizeof(g_word_en_theyve) / sizeof(g_word_en_theyve[0]), 0.84, 1, 0 },
    { "these", g_word_en_these, sizeof(g_word_en_these) / sizeof(g_word_en_these[0]), 0.94, 0, 1 },
    { "this", g_word_en_this, sizeof(g_word_en_this) / sizeof(g_word_en_this[0]), 0.92, 0, 1 },
    { "those", g_word_en_those, sizeof(g_word_en_those) / sizeof(g_word_en_those[0]), 0.94, 0, 1 },
    { "to", g_word_en_to, sizeof(g_word_en_to) / sizeof(g_word_en_to[0]), 0.76, 1, 0 },
    { "we", g_word_en_we, sizeof(g_word_en_we) / sizeof(g_word_en_we[0]), 0.80, 1, 0 },
    { "we'd", g_word_en_wed, sizeof(g_word_en_wed) / sizeof(g_word_en_wed[0]), 0.80, 1, 0 },
    { "we'll", g_word_en_well, sizeof(g_word_en_well) / sizeof(g_word_en_well[0]), 0.80, 1, 0 },
    { "we're", g_word_en_were, sizeof(g_word_en_were) / sizeof(g_word_en_were[0]), 0.80, 1, 0 },
    { "we've", g_word_en_weve, sizeof(g_word_en_weve) / sizeof(g_word_en_weve[0]), 0.80, 1, 0 },
    { "weren't", g_word_en_werent, sizeof(g_word_en_werent) / sizeof(g_word_en_werent[0]), 0.94, 0, 1 },
    { "worth", g_word_en_worth, sizeof(g_word_en_worth) / sizeof(g_word_en_worth[0]), 1.00, 0, 1 },
    { "won't", g_word_en_wont, sizeof(g_word_en_wont) / sizeof(g_word_en_wont[0]), 0.94, 0, 1 },
    { "wouldn't", g_word_en_wouldnt, sizeof(g_word_en_wouldnt) / sizeof(g_word_en_wouldnt[0]), 0.94, 0, 1 },
    { "you", g_word_en_you, sizeof(g_word_en_you) / sizeof(g_word_en_you[0]), 0.80, 1, 0 },
    { "you'd", g_word_en_youd, sizeof(g_word_en_youd) / sizeof(g_word_en_youd[0]), 0.80, 1, 0 },
    { "you'll", g_word_en_youll, sizeof(g_word_en_youll) / sizeof(g_word_en_youll[0]), 0.80, 1, 0 },
    { "you're", g_word_en_youre, sizeof(g_word_en_youre) / sizeof(g_word_en_youre[0]), 0.80, 1, 0 },
    { "you've", g_word_en_youve, sizeof(g_word_en_youve) / sizeof(g_word_en_youve[0]), 0.80, 1, 0 },
    { "are", g_word_en_are, sizeof(g_word_en_are) / sizeof(g_word_en_are[0]), 0.74, 1, 0 },
    { "am", g_word_en_am, sizeof(g_word_en_am) / sizeof(g_word_en_am[0]), 0.74, 1, 0 },
    { "aren't", g_word_en_arent, sizeof(g_word_en_arent) / sizeof(g_word_en_arent[0]), 0.94, 0, 1 },
    { "have", g_word_en_have, sizeof(g_word_en_have) / sizeof(g_word_en_have[0]), 0.82, 1, 0 },
    { "will", g_word_en_will, sizeof(g_word_en_will) / sizeof(g_word_en_will[0]), 0.82, 1, 0 },
    { "wasn't", g_word_en_wasnt, sizeof(g_word_en_wasnt) / sizeof(g_word_en_wasnt[0]), 0.94, 0, 1 }
};

static const lexicon_entry_t g_french_lexicon[] = {
    { "aujourd'hui", g_word_fr_aujourdhui, sizeof(g_word_fr_aujourdhui) / sizeof(g_word_fr_aujourdhui[0]), 1.00, 0, 3 },
    { "bonjour", g_word_fr_bonjour, sizeof(g_word_fr_bonjour) / sizeof(g_word_fr_bonjour[0]), 1.00, 0, 2 },
    { "ce", g_word_fr_ce, sizeof(g_word_fr_ce) / sizeof(g_word_fr_ce[0]), 0.72, 1, 0 },
    { "ceci", g_word_fr_ceci, sizeof(g_word_fr_ceci) / sizeof(g_word_fr_ceci[0]), 0.96, 0, 2 },
    { "demonstration", g_word_fr_demonstration, sizeof(g_word_fr_demonstration) / sizeof(g_word_fr_demonstration[0]), 1.00, 0, 4 },
    { "de", g_word_fr_de, sizeof(g_word_fr_de) / sizeof(g_word_fr_de[0]), 0.68, 1, 0 },
    { "depuis", g_word_fr_depuis, sizeof(g_word_fr_depuis) / sizeof(g_word_fr_depuis[0]), 0.92, 0, 2 },
    { "en", g_word_fr_en, sizeof(g_word_fr_en) / sizeof(g_word_fr_en[0]), 0.74, 1, 0 },
    { "je", g_word_fr_je, sizeof(g_word_fr_je) / sizeof(g_word_fr_je[0]), 0.72, 1, 0 },
    { "le", g_word_fr_le, sizeof(g_word_fr_le) / sizeof(g_word_fr_le[0]), 0.70, 1, 0 },
    { "ne", g_word_fr_ne, sizeof(g_word_fr_ne) / sizeof(g_word_fr_ne[0]), 0.70, 1, 0 },
    { "est", g_word_fr_est, sizeof(g_word_fr_est) / sizeof(g_word_fr_est[0]), 0.74, 1, 0 },
    { "francais", g_word_fr_francais, sizeof(g_word_fr_francais) / sizeof(g_word_fr_francais[0]), 1.00, 0, 2 },
    { "lib", g_word_fr_lib, sizeof(g_word_fr_lib) / sizeof(g_word_fr_lib[0]), 0.92, 0, 1 },
    { "phrase", g_word_fr_phrase, sizeof(g_word_fr_phrase) / sizeof(g_word_fr_phrase[0]), 1.00, 0, 1 },
    { "say", g_word_fr_say, sizeof(g_word_fr_say) / sizeof(g_word_fr_say[0]), 0.92, 0, 1 },
    { "une", g_word_fr_une, sizeof(g_word_fr_une) / sizeof(g_word_fr_une[0]), 0.78, 1, 0 },
    { "vous", g_word_fr_vous, sizeof(g_word_fr_vous) / sizeof(g_word_fr_vous[0]), 0.78, 1, 0 }
};

static const phoneme_pattern_rule_t g_english_patterns[] = {
    { "tion", g_rule_en_tion, SAY_ARRAY_COUNT(g_rule_en_tion), 0, 0 },
    { "sion", g_rule_en_sion, SAY_ARRAY_COUNT(g_rule_en_sion), 0, 0 },
    { "tch", g_rule_en_tch, SAY_ARRAY_COUNT(g_rule_en_tch), 0, 0 },
    { "dge", g_rule_en_dge, SAY_ARRAY_COUNT(g_rule_en_dge), 0, 0 },
    { "igh", g_rule_en_igh, SAY_ARRAY_COUNT(g_rule_en_igh), 0, 0 },
    { "ee", g_rule_en_ee, SAY_ARRAY_COUNT(g_rule_en_ee), 0, 0 },
    { "ea", g_rule_en_ee, SAY_ARRAY_COUNT(g_rule_en_ee), 0, 0 },
    { "oo", g_rule_en_oo, SAY_ARRAY_COUNT(g_rule_en_oo), 0, 0 },
    { "ow", g_rule_en_ow, SAY_ARRAY_COUNT(g_rule_en_ow), 0, 0 },
    { "ou", g_rule_en_ow, SAY_ARRAY_COUNT(g_rule_en_ow), 0, 0 },
    { "oi", g_rule_en_oi, SAY_ARRAY_COUNT(g_rule_en_oi), 0, 0 },
    { "oy", g_rule_en_oi, SAY_ARRAY_COUNT(g_rule_en_oi), 0, 0 },
    { "ai", g_rule_en_ai, SAY_ARRAY_COUNT(g_rule_en_ai), 0, 0 },
    { "ay", g_rule_en_ai, SAY_ARRAY_COUNT(g_rule_en_ai), 0, 0 },
    { "ei", g_rule_en_ai, SAY_ARRAY_COUNT(g_rule_en_ai), 0, 0 },
    { "ey", g_rule_en_ai, SAY_ARRAY_COUNT(g_rule_en_ai), 0, 0 },
    { "ph", g_rule_en_ph, SAY_ARRAY_COUNT(g_rule_en_ph), 0, 0 },
    { "sh", g_rule_en_sh, SAY_ARRAY_COUNT(g_rule_en_sh), 0, 0 },
    { "ch", g_rule_en_ch, SAY_ARRAY_COUNT(g_rule_en_ch), 0, 0 },
    { "ng", g_rule_en_ng, SAY_ARRAY_COUNT(g_rule_en_ng), 0, 0 },
    { "qu", g_rule_en_qu, SAY_ARRAY_COUNT(g_rule_en_qu), 0, 0 },
    { "ck", g_rule_en_ck, SAY_ARRAY_COUNT(g_rule_en_ck), 0, 0 },
    { "wr", g_rule_en_wr, SAY_ARRAY_COUNT(g_rule_en_wr), 0, 0 },
    { "wh", g_rule_en_wh, SAY_ARRAY_COUNT(g_rule_en_wh), 0, 0 },
    { "kn", g_rule_en_kn, SAY_ARRAY_COUNT(g_rule_en_kn), 1, 0 }
};

static const phoneme_pattern_rule_t g_french_patterns[] = {
    { "tion", g_rule_fr_tion, SAY_ARRAY_COUNT(g_rule_fr_tion), 0, 0 },
    { "eaux", g_rule_fr_eau, SAY_ARRAY_COUNT(g_rule_fr_eau), 0, 0 },
    { "eau", g_rule_fr_eau, SAY_ARRAY_COUNT(g_rule_fr_eau), 0, 0 },
    { "ou", g_rule_fr_ou, SAY_ARRAY_COUNT(g_rule_fr_ou), 0, 0 },
    { "oi", g_rule_fr_oi, SAY_ARRAY_COUNT(g_rule_fr_oi), 0, 0 },
    { "oy", g_rule_fr_oy, SAY_ARRAY_COUNT(g_rule_fr_oy), 0, 0 },
    { "oeu", g_rule_fr_eu, SAY_ARRAY_COUNT(g_rule_fr_eu), 0, 0 },
    { "eu", g_rule_fr_eu, SAY_ARRAY_COUNT(g_rule_fr_eu), 0, 0 },
    { "au", g_rule_fr_au, SAY_ARRAY_COUNT(g_rule_fr_au), 0, 0 },
    { "ai", g_rule_fr_ai, SAY_ARRAY_COUNT(g_rule_fr_ai), 0, 0 },
    { "ei", g_rule_fr_ai, SAY_ARRAY_COUNT(g_rule_fr_ai), 0, 0 },
    { "er", g_rule_fr_ai, SAY_ARRAY_COUNT(g_rule_fr_ai), 0, 1 },
    { "ez", g_rule_fr_ai, SAY_ARRAY_COUNT(g_rule_fr_ai), 0, 1 },
    { "gn", g_rule_fr_gn, SAY_ARRAY_COUNT(g_rule_fr_gn), 0, 0 },
    { "ch", g_rule_fr_ch, SAY_ARRAY_COUNT(g_rule_fr_ch), 0, 0 },
    { "ph", g_rule_fr_ph, SAY_ARRAY_COUNT(g_rule_fr_ph), 0, 0 },
    { "qu", g_rule_fr_qu, SAY_ARRAY_COUNT(g_rule_fr_qu), 0, 0 }
};

static const elision_prefix_t g_french_elision_prefixes[] = {
    { "c", g_elision_fr_c, SAY_ARRAY_COUNT(g_elision_fr_c), 0.68 },
    { "d", g_elision_fr_d, SAY_ARRAY_COUNT(g_elision_fr_d), 0.68 },
    { "j", g_elision_fr_j, SAY_ARRAY_COUNT(g_elision_fr_j), 0.72 },
    { "l", g_elision_fr_l, SAY_ARRAY_COUNT(g_elision_fr_l), 0.68 },
    { "m", g_elision_fr_m, SAY_ARRAY_COUNT(g_elision_fr_m), 0.72 },
    { "n", g_elision_fr_n, SAY_ARRAY_COUNT(g_elision_fr_n), 0.72 },
    { "qu", g_elision_fr_qu, SAY_ARRAY_COUNT(g_elision_fr_qu), 0.76 },
    { "s", g_elision_fr_s, SAY_ARRAY_COUNT(g_elision_fr_s), 0.72 },
    { "t", g_elision_fr_t, SAY_ARRAY_COUNT(g_elision_fr_t), 0.72 }
};

static void say_set_error(char *error, size_t error_size, const char *fmt, ...)
{
    va_list args;

    if (error == NULL || error_size == 0) {
        return;
    }

    va_start(args, fmt);
    vsnprintf(error, error_size, fmt, args);
    va_end(args);
}

static const phoneme_def_t *say_get_phoneme(phoneme_id_t id)
{
    if ((int) id < 0 || id >= PH_COUNT) {
        return &g_phonemes[PH_PAUSE];
    }
    return &g_phonemes[id];
}

static int say_is_vowel_phone(phoneme_id_t id)
{
    return say_get_phoneme(id)->is_vowel;
}

static int say_is_boundary_char(char c)
{
    return c == '.' || c == ',' || c == '!' || c == '?' || c == ';' || c == ':';
}

static int say_is_word_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

static int say_is_token_char(char c)
{
    return say_is_word_char(c) || c == '\'' || c == '-';
}

static int say_equals_icase(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char) *a) != tolower((unsigned char) *b)) {
            return 0;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int say_match_at(const char *word, size_t index, const char *pattern);
static int say_append_text_word(const char *word, say_language_t language, segment_buffer_t *segments);

static int say_segment_buffer_reserve(segment_buffer_t *buffer, size_t extra)
{
    size_t required;
    size_t capacity;
    segment_t *data;

    required = buffer->count + extra;
    if (required <= buffer->capacity) {
        return 1;
    }

    capacity = buffer->capacity == 0 ? 32 : buffer->capacity;
    while (capacity < required) {
        capacity *= 2;
    }

    data = (segment_t *) realloc(buffer->data, capacity * sizeof(*buffer->data));
    if (data == NULL) {
        return 0;
    }

    buffer->data = data;
    buffer->capacity = capacity;
    return 1;
}

static int say_segment_buffer_push(
    segment_buffer_t *buffer,
    phoneme_id_t phoneme,
    double duration_scale,
    int boundary_type
)
{
    segment_t *segment;

    if (!say_segment_buffer_reserve(buffer, 1)) {
        return 0;
    }

    segment = &buffer->data[buffer->count++];
    segment->phoneme = phoneme;
    segment->duration_scale = duration_scale;
    segment->word_start = 0;
    segment->word_end = 0;
    segment->boundary_type = boundary_type;
    segment->weak_word = 0;
    segment->stress = 0;
    return 1;
}

static int say_segment_buffer_append(segment_buffer_t *buffer, const segment_buffer_t *extra)
{
    if (extra == NULL || extra->count == 0) {
        return 1;
    }
    if (!say_segment_buffer_reserve(buffer, extra->count)) {
        return 0;
    }

    memcpy(buffer->data + buffer->count, extra->data, extra->count * sizeof(*extra->data));
    buffer->count += extra->count;
    return 1;
}

static int say_frame_buffer_reserve(frame_buffer_t *buffer, size_t extra)
{
    size_t required;
    size_t capacity;
    frame_t *data;

    required = buffer->count + extra;
    if (required <= buffer->capacity) {
        return 1;
    }

    capacity = buffer->capacity == 0 ? 64 : buffer->capacity;
    while (capacity < required) {
        capacity *= 2;
    }

    data = (frame_t *) realloc(buffer->data, capacity * sizeof(*buffer->data));
    if (data == NULL) {
        return 0;
    }

    buffer->data = data;
    buffer->capacity = capacity;
    return 1;
}

static int say_frame_buffer_push(frame_buffer_t *buffer, const frame_t *frame)
{
    if (!say_frame_buffer_reserve(buffer, 1)) {
        return 0;
    }
    buffer->data[buffer->count++] = *frame;
    return 1;
}

static int say_text_buffer_reserve(text_buffer_t *buffer, size_t extra)
{
    size_t required;
    size_t capacity;
    char *data;

    required = buffer->count + extra + 1;
    if (required <= buffer->capacity) {
        return 1;
    }

    capacity = buffer->capacity == 0 ? 256 : buffer->capacity;
    while (capacity < required) {
        capacity *= 2;
    }

    data = (char *) realloc(buffer->data, capacity);
    if (data == NULL) {
        return 0;
    }

    buffer->data = data;
    buffer->capacity = capacity;
    return 1;
}

static int say_text_buffer_append(text_buffer_t *buffer, const char *text)
{
    size_t length;

    length = strlen(text);
    if (!say_text_buffer_reserve(buffer, length)) {
        return 0;
    }

    memcpy(buffer->data + buffer->count, text, length);
    buffer->count += length;
    buffer->data[buffer->count] = '\0';
    return 1;
}

static int say_text_buffer_appendf(text_buffer_t *buffer, const char *fmt, ...)
{
    va_list args;
    va_list copy;
    int written;

    va_start(args, fmt);
    va_copy(copy, args);
    written = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (written < 0) {
        va_end(args);
        return 0;
    }

    if (!say_text_buffer_reserve(buffer, (size_t) written)) {
        va_end(args);
        return 0;
    }

    vsnprintf(buffer->data + buffer->count, buffer->capacity - buffer->count, fmt, args);
    buffer->count += (size_t) written;
    va_end(args);
    return 1;
}

static size_t say_utf8_decode(const unsigned char *text, unsigned int *codepoint)
{
    unsigned int cp;

    if (text[0] < 0x80) {
        *codepoint = text[0];
        return 1;
    }
    if ((text[0] & 0xE0) == 0xC0 && (text[1] & 0xC0) == 0x80) {
        cp = ((unsigned int) (text[0] & 0x1F) << 6) | (unsigned int) (text[1] & 0x3F);
        if (cp >= 0x80) {
            *codepoint = cp;
            return 2;
        }
    }
    if ((text[0] & 0xF0) == 0xE0 && (text[1] & 0xC0) == 0x80 && (text[2] & 0xC0) == 0x80) {
        cp = ((unsigned int) (text[0] & 0x0F) << 12) |
             ((unsigned int) (text[1] & 0x3F) << 6) |
             (unsigned int) (text[2] & 0x3F);
        if (cp >= 0x800) {
            *codepoint = cp;
            return 3;
        }
    }
    if ((text[0] & 0xF8) == 0xF0 &&
        (text[1] & 0xC0) == 0x80 &&
        (text[2] & 0xC0) == 0x80 &&
        (text[3] & 0xC0) == 0x80) {
        cp = ((unsigned int) (text[0] & 0x07) << 18) |
             ((unsigned int) (text[1] & 0x3F) << 12) |
             ((unsigned int) (text[2] & 0x3F) << 6) |
             (unsigned int) (text[3] & 0x3F);
        if (cp >= 0x10000 && cp <= 0x10FFFF) {
            *codepoint = cp;
            return 4;
        }
    }

    *codepoint = text[0];
    return 1;
}

static const char *say_fold_codepoint(unsigned int cp)
{
    switch (cp) {
        case 0x00C0: case 0x00C1: case 0x00C2: case 0x00C3: case 0x00C4: case 0x00C5:
        case 0x00E0: case 0x00E1: case 0x00E2: case 0x00E3: case 0x00E4: case 0x00E5:
            return "a";
        case 0x00C7: case 0x00E7:
            return "c";
        case 0x00C8: case 0x00C9: case 0x00CA: case 0x00CB:
        case 0x00E8: case 0x00E9: case 0x00EA: case 0x00EB:
            return "e";
        case 0x00CC: case 0x00CD: case 0x00CE: case 0x00CF:
        case 0x00EC: case 0x00ED: case 0x00EE: case 0x00EF:
            return "i";
        case 0x00D2: case 0x00D3: case 0x00D4: case 0x00D5: case 0x00D6:
        case 0x00F2: case 0x00F3: case 0x00F4: case 0x00F5: case 0x00F6:
            return "o";
        case 0x00D9: case 0x00DA: case 0x00DB: case 0x00DC:
        case 0x00F9: case 0x00FA: case 0x00FB: case 0x00FC:
            return "u";
        case 0x00DD: case 0x00FD: case 0x00FF:
            return "y";
        case 0x0152: case 0x0153:
            return "oe";
        case 0x00C6: case 0x00E6:
            return "ae";
        case 0x2019: case 0x2018:
            return "'";
        case 0x2010: case 0x2011: case 0x2012: case 0x2013: case 0x2014:
            return "-";
        default:
            return NULL;
    }
}

static char *say_normalize_text(const char *input, char *error, size_t error_size)
{
    const unsigned char *ptr;
    size_t input_len;
    size_t capacity;
    size_t count;
    char *output;

    if (input == NULL) {
        say_set_error(error, error_size, "missing input text");
        return NULL;
    }

    input_len = strlen(input);
    capacity = input_len * 2 + 16;
    output = (char *) malloc(capacity);
    if (output == NULL) {
        say_set_error(error, error_size, "out of memory while normalizing text");
        return NULL;
    }

    count = 0;
    ptr = (const unsigned char *) input;
    while (*ptr != '\0') {
        unsigned int cp;
        size_t consumed;
        const char *folded;
        char ascii;

        consumed = say_utf8_decode(ptr, &cp);
        ptr += consumed;

        if (cp < 0x80) {
            ascii = (char) cp;
            if (ascii >= 'A' && ascii <= 'Z') {
                ascii = (char) (ascii - 'A' + 'a');
            }
            if (ascii == '\n' || ascii == '\r' || ascii == '\t') {
                ascii = ' ';
            }

            if (say_is_token_char(ascii) || say_is_boundary_char(ascii) || ascii == ' ') {
                output[count++] = ascii;
            }
            else {
                output[count++] = ' ';
            }
        }
        else {
            folded = say_fold_codepoint(cp);
            if (folded != NULL) {
                while (*folded != '\0') {
                    ascii = *folded++;
                    if (!say_is_token_char(ascii) && ascii != ' ' && !say_is_boundary_char(ascii)) {
                        ascii = ' ';
                    }
                    output[count++] = ascii;
                }
            }
            else {
                output[count++] = ' ';
            }
        }

        if (count + 8 >= capacity) {
            char *grown;
            capacity *= 2;
            grown = (char *) realloc(output, capacity);
            if (grown == NULL) {
                free(output);
                say_set_error(error, error_size, "out of memory while growing normalized buffer");
                return NULL;
            }
            output = grown;
        }
    }

    output[count] = '\0';
    return output;
}

static int say_append_phone(segment_buffer_t *segments, phoneme_id_t phoneme)
{
    return say_segment_buffer_push(segments, phoneme, 1.0, 0);
}

static int say_append_phone_list(
    segment_buffer_t *segments,
    const phoneme_id_t *phonemes,
    size_t phoneme_count
)
{
    size_t i;

    for (i = 0; i < phoneme_count; ++i) {
        if (!say_append_phone(segments, phonemes[i])) {
            return 0;
        }
    }
    return 1;
}

static int say_pattern_rule_matches(const phoneme_pattern_rule_t *rule, const char *word, size_t index)
{
    size_t pattern_len;

    if (rule->initial_only && index != 0) {
        return 0;
    }
    if (!say_match_at(word, index, rule->pattern)) {
        return 0;
    }

    pattern_len = strlen(rule->pattern);
    if (rule->final_only && word[index + pattern_len] != '\0') {
        return 0;
    }
    return 1;
}

static int say_try_apply_pattern_rules(
    const char *word,
    size_t index,
    const phoneme_pattern_rule_t *rules,
    size_t rule_count,
    segment_buffer_t *segments,
    size_t *out_consumed
)
{
    size_t i;

    for (i = 0; i < rule_count; ++i) {
        if (!say_pattern_rule_matches(&rules[i], word, index)) {
            continue;
        }
        if (!say_append_phone_list(segments, rules[i].phonemes, rules[i].phoneme_count)) {
            return -1;
        }
        *out_consumed = strlen(rules[i].pattern);
        return 1;
    }
    return 0;
}

static int say_append_phone_sequence(
    segment_buffer_t *segments,
    const phoneme_id_t *phonemes,
    size_t phoneme_count,
    double duration_scale,
    int weak_word,
    int primary_stress_vowel
)
{
    size_t start;
    size_t i;
    int vowel_rank;

    start = segments->count;
    for (i = 0; i < phoneme_count; ++i) {
        if (!say_segment_buffer_push(segments, phonemes[i], duration_scale, 0)) {
            return 0;
        }
    }

    if (segments->count > start) {
        segments->data[start].word_start = 1;
        segments->data[segments->count - 1].word_end = 1;
        vowel_rank = 0;
        for (i = start; i < segments->count; ++i) {
            segments->data[i].weak_word = weak_word;
            if (say_is_vowel_phone(segments->data[i].phoneme)) {
                ++vowel_rank;
                if (primary_stress_vowel > 0 && vowel_rank == primary_stress_vowel) {
                    segments->data[i].stress = 2;
                }
            }
        }
    }
    return 1;
}

static int say_try_append_lexicon_word(
    const char *word,
    say_language_t language,
    segment_buffer_t *segments
)
{
    const lexicon_entry_t *entries;
    size_t entry_count;
    size_t i;
    int primary_stress_vowel;

    if (language == SAY_LANG_FR) {
        entries = g_french_lexicon;
        entry_count = sizeof(g_french_lexicon) / sizeof(g_french_lexicon[0]);
    }
    else {
        entries = g_english_lexicon;
        entry_count = sizeof(g_english_lexicon) / sizeof(g_english_lexicon[0]);
    }

    for (i = 0; i < entry_count; ++i) {
        if (strcmp(word, entries[i].word) == 0) {
            primary_stress_vowel = language == SAY_LANG_FR ? 0 : entries[i].primary_stress_vowel;
            return say_append_phone_sequence(
                segments,
                entries[i].phonemes,
                entries[i].phoneme_count,
                entries[i].duration_scale,
                entries[i].weak_word,
                primary_stress_vowel) ? 1 : -1;
        }
    }

    return 0;
}

static int say_match_at(const char *word, size_t index, const char *pattern)
{
    size_t i;

    for (i = 0; pattern[i] != '\0'; ++i) {
        if (word[index + i] == '\0' || word[index + i] != pattern[i]) {
            return 0;
        }
    }
    return 1;
}

static int say_has_vowel_after(const char *word, size_t index)
{
    for (; word[index] != '\0'; ++index) {
        switch (word[index]) {
            case 'a': case 'e': case 'i': case 'o': case 'u': case 'y':
                return 1;
            default:
                break;
        }
    }
    return 0;
}

static int say_is_vowel_char(char c)
{
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y';
}

static phoneme_id_t say_last_word_phone(const segment_buffer_t *segments, size_t start)
{
    if (segments->count <= start) {
        return PH_PAUSE;
    }
    return segments->data[segments->count - 1].phoneme;
}

static int say_phone_is_sibilant_or_affricate(phoneme_id_t id)
{
    return id == PH_S || id == PH_Z || id == PH_SH || id == PH_ZH ||
           id == PH_CH || id == PH_JH || id == PH_TS || id == PH_DZ;
}

static int say_phone_is_plural_voiced(phoneme_id_t id)
{
    return id != PH_PAUSE && say_get_phoneme(id)->voiced;
}

static int say_is_english_voiced_th_word(const char *word)
{
    static const char *const g_voiced_th_words[] = {
        "the", "this", "that", "these", "those", "there", "their", "theirs",
        "them", "then", "than", "though", "thus", "they", "thee",
        "other", "mother", "father", "brother", "weather", "whether"
    };
    size_t i;

    for (i = 0; i < sizeof(g_voiced_th_words) / sizeof(g_voiced_th_words[0]); ++i) {
        if (strcmp(word, g_voiced_th_words[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

static phoneme_id_t say_english_th_phone(const char *word, size_t index)
{
    if (index == 0) {
        return say_is_english_voiced_th_word(word) ? PH_DH : PH_TH;
    }
    if (index > 0 && say_is_vowel_char(word[index - 1]) && say_has_vowel_after(word, index + 2)) {
        return PH_DH;
    }
    return PH_TH;
}

static int say_word_has_suffix(const char *word, const char *suffix)
{
    size_t word_len;
    size_t suffix_len;

    word_len = strlen(word);
    suffix_len = strlen(suffix);
    if (suffix_len > word_len) {
        return 0;
    }
    return strcmp(word + (word_len - suffix_len), suffix) == 0;
}

static int say_count_segment_vowels(const segment_buffer_t *segments, size_t start)
{
    int vowel_count;
    size_t i;

    vowel_count = 0;
    for (i = start; i < segments->count; ++i) {
        if (say_is_vowel_phone(segments->data[i].phoneme)) {
            ++vowel_count;
        }
        if (segments->data[i].word_end) {
            break;
        }
    }
    return vowel_count;
}

static int say_guess_english_primary_stress(
    const char *word,
    const segment_buffer_t *segments,
    size_t start
)
{
    int vowel_count;

    vowel_count = say_count_segment_vowels(segments, start);
    if (vowel_count <= 1) {
        return vowel_count;
    }
    if (say_word_has_suffix(word, "tion") || say_word_has_suffix(word, "sion") ||
        say_word_has_suffix(word, "cial") || say_word_has_suffix(word, "tial")) {
        return vowel_count - 1;
    }
    if (say_word_has_suffix(word, "ic") || say_word_has_suffix(word, "ics") ||
        say_word_has_suffix(word, "ical") || say_word_has_suffix(word, "ian")) {
        return vowel_count - 1;
    }
    if (say_word_has_suffix(word, "ity") || say_word_has_suffix(word, "ify") ||
        say_word_has_suffix(word, "ety") || say_word_has_suffix(word, "ology") ||
        say_word_has_suffix(word, "graphy")) {
        return vowel_count >= 3 ? vowel_count - 2 : 1;
    }
    return 1;
}

static int say_guess_french_primary_stress(const segment_buffer_t *segments, size_t start)
{
    (void) segments;
    (void) start;
    return 0;
}

static void say_finalize_word_metadata(
    segment_buffer_t *segments,
    size_t start,
    double duration_scale,
    int weak_word,
    int primary_stress_vowel
)
{
    int vowel_rank;
    size_t i;

    if (segments->count <= start) {
        return;
    }

    segments->data[start].word_start = 1;
    segments->data[segments->count - 1].word_end = 1;
    vowel_rank = 0;
    for (i = start; i < segments->count; ++i) {
        segments->data[i].duration_scale *= duration_scale;
        segments->data[i].weak_word = weak_word;
        if (say_is_vowel_phone(segments->data[i].phoneme)) {
            ++vowel_rank;
            if (primary_stress_vowel > 0 && vowel_rank == primary_stress_vowel) {
                segments->data[i].stress = 2;
            }
        }
    }
}

static phoneme_id_t say_english_default_vowel(const char *word, size_t index)
{
    char c;
    char next;

    c = word[index];
    next = word[index + 1];

    switch (c) {
        case 'a':
            if (next == 'r') {
                return PH_AH;
            }
            if (word[index + 1] == '\0') {
                return PH_E;
            }
            return PH_AE;
        case 'e':
            if (word[index + 1] == '\0' && index > 0) {
                return PH_PAUSE;
            }
            return PH_EH;
        case 'i':
            if (word[index + 1] == '\0') {
                return PH_I;
            }
            return PH_IH;
        case 'o':
            if (next == 'r') {
                return PH_OH;
            }
            return PH_O;
        case 'u':
            if (index == 0) {
                return PH_U;
            }
            return PH_AH;
        case 'y':
            return (index == 0) ? PH_J : PH_I;
        default:
            return PH_SCHWA;
    }
}

static int say_phonemize_english_word(const char *word, segment_buffer_t *segments)
{
    size_t i;
    size_t start;
    size_t consumed;
    int rule_result;

    start = segments->count;
    i = 0;
    while (word[i] != '\0') {
        if (i > 0 && word[i] == word[i - 1] && !say_is_vowel_char(word[i])) {
            ++i;
            continue;
        }
        rule_result = say_try_apply_pattern_rules(
            word,
            i,
            g_english_patterns,
            SAY_ARRAY_COUNT(g_english_patterns),
            segments,
            &consumed);
        if (rule_result < 0) {
            return 0;
        }
        if (rule_result > 0) {
            i += consumed;
            continue;
        }
        if (say_match_at(word, i, "th")) {
            phoneme_id_t phone = say_english_th_phone(word, i);
            if (!say_append_phone(segments, phone)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (word[i] == 'e' && word[i + 1] == 's' && word[i + 2] == '\0') {
            phoneme_id_t previous = say_last_word_phone(segments, start);
            if (say_phone_is_sibilant_or_affricate(previous)) {
                if (!say_append_phone(segments, PH_SCHWA) || !say_append_phone(segments, PH_Z)) {
                    return 0;
                }
            }
            else {
                phoneme_id_t final_phone = say_phone_is_plural_voiced(previous) ? PH_Z : PH_S;
                if (!say_append_phone(segments, final_phone)) {
                    return 0;
                }
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "ers") && word[i + 3] == '\0') {
            if (!say_append_phone(segments, PH_R) || !say_append_phone(segments, PH_Z)) {
                return 0;
            }
            i += 3;
            continue;
        }
        if (say_match_at(word, i, "er") && word[i + 2] == '\0') {
            if (!say_append_phone(segments, PH_R)) {
                return 0;
            }
            i += 2;
            continue;
        }

        switch (word[i]) {
            case '\'':
            case '-':
                ++i;
                break;
            case 'a':
            case 'e':
            case 'i':
            case 'o':
            case 'u':
            case 'y': {
                phoneme_id_t vowel = say_english_default_vowel(word, i);
                if (vowel != PH_PAUSE && !say_append_phone(segments, vowel)) {
                    return 0;
                }
                ++i;
                break;
            }
            case 'b':
                if (!say_append_phone(segments, PH_B)) {
                    return 0;
                }
                ++i;
                break;
            case 'c':
                if (word[i + 1] == 'e' || word[i + 1] == 'i' || word[i + 1] == 'y') {
                    if (!say_append_phone(segments, PH_S)) {
                        return 0;
                    }
                }
                else if (!say_append_phone(segments, PH_K)) {
                    return 0;
                }
                ++i;
                break;
            case 'd':
                if (!say_append_phone(segments, PH_D)) {
                    return 0;
                }
                ++i;
                break;
            case 'f':
                if (!say_append_phone(segments, PH_F)) {
                    return 0;
                }
                ++i;
                break;
            case 'g':
                if (word[i + 1] == 'e' || word[i + 1] == 'i' || word[i + 1] == 'y') {
                    if (!say_append_phone(segments, PH_JH)) {
                        return 0;
                    }
                }
                else if (!say_append_phone(segments, PH_G)) {
                    return 0;
                }
                ++i;
                break;
            case 'h':
                if (!say_append_phone(segments, PH_H)) {
                    return 0;
                }
                ++i;
                break;
            case 'j':
                if (!say_append_phone(segments, PH_JH)) {
                    return 0;
                }
                ++i;
                break;
            case 'k':
                if (!say_append_phone(segments, PH_K)) {
                    return 0;
                }
                ++i;
                break;
            case 'l':
                if (!say_append_phone(segments, PH_L)) {
                    return 0;
                }
                ++i;
                break;
            case 'm':
                if (!say_append_phone(segments, PH_M)) {
                    return 0;
                }
                ++i;
                break;
            case 'n':
                if (!say_append_phone(segments, PH_N)) {
                    return 0;
                }
                ++i;
                break;
            case 'p':
                if (!say_append_phone(segments, PH_P)) {
                    return 0;
                }
                ++i;
                break;
            case 'q':
                if (!say_append_phone(segments, PH_K)) {
                    return 0;
                }
                ++i;
                break;
            case 'r':
                if (!say_append_phone(segments, PH_R)) {
                    return 0;
                }
                ++i;
                break;
            case 's':
                {
                    phoneme_id_t phone = PH_S;
                    phoneme_id_t previous = say_last_word_phone(segments, start);
                    if (word[i + 1] == '\0') {
                        if (say_phone_is_plural_voiced(previous) && !say_phone_is_sibilant_or_affricate(previous)) {
                            phone = PH_Z;
                        }
                    }
                    else if (i > 0 && say_is_vowel_char(word[i - 1]) && say_is_vowel_char(word[i + 1])) {
                        phone = PH_Z;
                    }
                    if (!say_append_phone(segments, phone)) {
                        return 0;
                    }
                }
                ++i;
                break;
            case 't':
                if (!say_append_phone(segments, PH_T)) {
                    return 0;
                }
                ++i;
                break;
            case 'v':
                if (!say_append_phone(segments, PH_V)) {
                    return 0;
                }
                ++i;
                break;
            case 'w':
                if (!say_append_phone(segments, PH_W)) {
                    return 0;
                }
                ++i;
                break;
            case 'x':
                if (!say_append_phone(segments, PH_K) || !say_append_phone(segments, PH_S)) {
                    return 0;
                }
                ++i;
                break;
            case 'z':
                if (!say_append_phone(segments, PH_Z)) {
                    return 0;
                }
                ++i;
                break;
            default:
                ++i;
                break;
        }
    }

    if (segments->count > start) {
        say_finalize_word_metadata(
            segments,
            start,
            1.0,
            0,
            say_guess_english_primary_stress(word, segments, start));
    }
    return 1;
}

static int say_french_nasal_applicable(const char *word, size_t after_pair)
{
    char c;

    c = word[after_pair];
    if (c == '\0') {
        return 1;
    }
    if (!say_is_vowel_char(c) && c != 'n' && c != 'm') {
        return 1;
    }
    return 0;
}

static int say_phonemize_french_word(const char *word, segment_buffer_t *segments)
{
    size_t i;
    size_t start;
    size_t len;
    size_t consumed;
    int rule_result;

    start = segments->count;
    len = strlen(word);
    i = 0;
    while (i < len) {
        if (i > 0 && word[i] == word[i - 1] && !say_is_vowel_char(word[i])) {
            ++i;
            continue;
        }
        if (i + 2 == len && say_match_at(word, i, "es")) {
            break;
        }
        if (i + 3 == len && say_match_at(word, i, "ent")) {
            break;
        }
        if (i + 1 == len && word[i] == 'e') {
            break;
        }
        if ((say_match_at(word, i, "ain") || say_match_at(word, i, "ein") || say_match_at(word, i, "aim") || say_match_at(word, i, "eim")) &&
            say_french_nasal_applicable(word, i + 3)) {
            if (!say_append_phone(segments, PH_IN)) {
                return 0;
            }
            i += 3;
            continue;
        }
        if ((say_match_at(word, i, "an") || say_match_at(word, i, "am") || say_match_at(word, i, "en") || say_match_at(word, i, "em")) &&
            say_french_nasal_applicable(word, i + 2)) {
            if (!say_append_phone(segments, PH_AN)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if ((say_match_at(word, i, "on") || say_match_at(word, i, "om")) &&
            say_french_nasal_applicable(word, i + 2)) {
            if (!say_append_phone(segments, PH_ON)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if ((say_match_at(word, i, "in") || say_match_at(word, i, "im") || say_match_at(word, i, "yn") || say_match_at(word, i, "ym") ||
             say_match_at(word, i, "un") || say_match_at(word, i, "um")) &&
            say_french_nasal_applicable(word, i + 2)) {
            if (!say_append_phone(segments, PH_IN)) {
                return 0;
            }
            i += 2;
            continue;
        }
        rule_result = say_try_apply_pattern_rules(
            word,
            i,
            g_french_patterns,
            SAY_ARRAY_COUNT(g_french_patterns),
            segments,
            &consumed);
        if (rule_result < 0) {
            return 0;
        }
        if (rule_result > 0) {
            i += consumed;
            continue;
        }
        if (say_match_at(word, i, "ill") && i > 0 && say_is_vowel_char(word[i - 1])) {
            if (!say_append_phone(segments, PH_J)) {
                return 0;
            }
            i += 3;
            continue;
        }

        switch (word[i]) {
            case '\'':
            case '-':
                ++i;
                break;
            case 'a':
                if (!say_append_phone(segments, PH_A)) {
                    return 0;
                }
                ++i;
                break;
            case 'b':
                if (!say_append_phone(segments, PH_B)) {
                    return 0;
                }
                ++i;
                break;
            case 'c':
                if (word[i + 1] == 'e' || word[i + 1] == 'i' || word[i + 1] == 'y') {
                    if (!say_append_phone(segments, PH_S)) {
                        return 0;
                    }
                }
                else if (!say_append_phone(segments, PH_K)) {
                    return 0;
                }
                ++i;
                break;
            case 'd':
                if (!say_append_phone(segments, PH_D)) {
                    return 0;
                }
                ++i;
                break;
            case 'e':
                if (!say_append_phone(segments, PH_SCHWA)) {
                    return 0;
                }
                ++i;
                break;
            case 'f':
                if (!say_append_phone(segments, PH_F)) {
                    return 0;
                }
                ++i;
                break;
            case 'g':
                if (word[i + 1] == 'e' || word[i + 1] == 'i' || word[i + 1] == 'y') {
                    if (!say_append_phone(segments, PH_ZH)) {
                        return 0;
                    }
                }
                else if (!say_append_phone(segments, PH_G)) {
                    return 0;
                }
                ++i;
                break;
            case 'h':
                ++i;
                break;
            case 'i':
                if (!say_append_phone(segments, PH_I)) {
                    return 0;
                }
                ++i;
                break;
            case 'j':
                if (!say_append_phone(segments, PH_ZH)) {
                    return 0;
                }
                ++i;
                break;
            case 'k':
                if (!say_append_phone(segments, PH_K)) {
                    return 0;
                }
                ++i;
                break;
            case 'l':
                if (!say_append_phone(segments, PH_L)) {
                    return 0;
                }
                ++i;
                break;
            case 'm':
                if (!say_append_phone(segments, PH_M)) {
                    return 0;
                }
                ++i;
                break;
            case 'n':
                if (!say_append_phone(segments, PH_N)) {
                    return 0;
                }
                ++i;
                break;
            case 'o':
                if (!say_append_phone(segments, PH_O)) {
                    return 0;
                }
                ++i;
                break;
            case 'p':
                if (!say_append_phone(segments, PH_P)) {
                    return 0;
                }
                ++i;
                break;
            case 'q':
                if (!say_append_phone(segments, PH_K)) {
                    return 0;
                }
                ++i;
                break;
            case 'r':
                if (!say_append_phone(segments, PH_R)) {
                    return 0;
                }
                ++i;
                break;
            case 's':
                if (i == len - 1) {
                    ++i;
                }
                else if (i > 0 && word[i + 1] != '\0' && say_is_vowel_char(word[i - 1]) && say_is_vowel_char(word[i + 1])) {
                    if (!say_append_phone(segments, PH_Z)) {
                        return 0;
                    }
                }
                else if (!say_append_phone(segments, PH_S)) {
                    return 0;
                }
                ++i;
                break;
            case 't':
                if (!say_append_phone(segments, PH_T)) {
                    return 0;
                }
                ++i;
                break;
            case 'u':
                if (!say_append_phone(segments, PH_Y)) {
                    return 0;
                }
                ++i;
                break;
            case 'v':
                if (!say_append_phone(segments, PH_V)) {
                    return 0;
                }
                ++i;
                break;
            case 'w':
                if (!say_append_phone(segments, PH_W)) {
                    return 0;
                }
                ++i;
                break;
            case 'x':
                if (i == len - 1) {
                    ++i;
                }
                else {
                    if (!say_append_phone(segments, PH_K) || !say_append_phone(segments, PH_S)) {
                        return 0;
                    }
                    ++i;
                }
                break;
            case 'y':
                if (!say_append_phone(segments, PH_I)) {
                    return 0;
                }
                ++i;
                break;
            case 'z':
                if (!say_append_phone(segments, PH_Z)) {
                    return 0;
                }
                ++i;
                break;
            default:
                ++i;
                break;
        }
    }

    if (segments->count > start) {
        say_finalize_word_metadata(
            segments,
            start,
            1.0,
            0,
            say_guess_french_primary_stress(segments, start));
    }
    return 1;
}

static int say_append_simple_text_word(const char *word, say_language_t language, segment_buffer_t *segments)
{
    if (language == SAY_LANG_FR) {
        return say_phonemize_french_word(word, segments);
    }
    return say_phonemize_english_word(word, segments);
}

static int say_try_append_compound_word(const char *word, say_language_t language, segment_buffer_t *segments)
{
    const char *part_start;
    const char *cursor;
    int appended_any;

    if (strchr(word, '-') == NULL) {
        return 0;
    }

    part_start = word;
    cursor = word;
    appended_any = 0;
    for (;;) {
        if (*cursor == '-' || *cursor == '\0') {
            size_t length;

            length = (size_t) (cursor - part_start);
            if (length > 0) {
                char part[128];

                if (length >= sizeof(part)) {
                    return 0;
                }
                memcpy(part, part_start, length);
                part[length] = '\0';
                if (!say_append_text_word(part, language, segments)) {
                    return -1;
                }
                appended_any = 1;
            }
            if (*cursor == '\0') {
                break;
            }
            part_start = cursor + 1;
        }
        ++cursor;
    }
    return appended_any ? 1 : 0;
}

static int say_try_append_french_elision_word(const char *word, segment_buffer_t *segments)
{
    const char *apostrophe;
    char prefix[16];
    const elision_prefix_t *rule;
    size_t prefix_len;
    size_t i;

    apostrophe = strchr(word, '\'');
    if (apostrophe == NULL || apostrophe == word || apostrophe[1] == '\0' || strchr(apostrophe + 1, '\'') != NULL) {
        return 0;
    }

    prefix_len = (size_t) (apostrophe - word);
    if (prefix_len >= sizeof(prefix)) {
        return 0;
    }

    memcpy(prefix, word, prefix_len);
    prefix[prefix_len] = '\0';

    rule = NULL;
    for (i = 0; i < SAY_ARRAY_COUNT(g_french_elision_prefixes); ++i) {
        if (strcmp(prefix, g_french_elision_prefixes[i].text) == 0) {
            rule = &g_french_elision_prefixes[i];
            break;
        }
    }
    if (rule == NULL) {
        return 0;
    }

    if (!say_append_phone_sequence(segments, rule->phonemes, rule->phoneme_count, rule->duration_scale, 1, 0)) {
        return -1;
    }
    return say_append_text_word(apostrophe + 1, SAY_LANG_FR, segments) ? 1 : -1;
}

static int say_append_english_contraction_suffix(segment_buffer_t *segments, const char *suffix)
{
    size_t old_count;
    size_t append_start;
    int weak_word;
    phoneme_id_t previous;

    if (segments->count == 0) {
        return 0;
    }

    old_count = segments->count;
    append_start = segments->count;
    weak_word = segments->data[0].weak_word;
    previous = segments->data[segments->count - 1].phoneme;

    if (strcmp(suffix, "s") == 0) {
        if (say_phone_is_sibilant_or_affricate(previous)) {
            if (!say_append_phone(segments, PH_SCHWA) || !say_append_phone(segments, PH_Z)) {
                return -1;
            }
        }
        else {
            if (!say_append_phone(segments, say_phone_is_plural_voiced(previous) ? PH_Z : PH_S)) {
                return -1;
            }
        }
    }
    else if (strcmp(suffix, "re") == 0) {
        if (!say_append_phone(segments, PH_R)) {
            return -1;
        }
    }
    else if (strcmp(suffix, "ve") == 0) {
        if (!say_append_phone(segments, PH_V)) {
            return -1;
        }
    }
    else if (strcmp(suffix, "ll") == 0) {
        if (!say_append_phone(segments, PH_L)) {
            return -1;
        }
    }
    else if (strcmp(suffix, "d") == 0) {
        if (!say_append_phone(segments, PH_D)) {
            return -1;
        }
    }
    else if (strcmp(suffix, "m") == 0) {
        if (!say_append_phone(segments, PH_M)) {
            return -1;
        }
    }
    else {
        return 0;
    }

    segments->data[old_count - 1].word_end = 0;
    for (; append_start < segments->count; ++append_start) {
        segments->data[append_start].weak_word = weak_word;
    }
    segments->data[segments->count - 1].word_end = 1;
    return 1;
}

static int say_try_append_english_contraction_word(const char *word, segment_buffer_t *segments)
{
    const char *apostrophe;
    char base[128];
    segment_buffer_t temp_segments;
    size_t base_len;
    int suffix_result;
    int append_ok;

    apostrophe = strchr(word, '\'');
    if (apostrophe == NULL || apostrophe == word || apostrophe[1] == '\0' || strchr(apostrophe + 1, '\'') != NULL) {
        return 0;
    }

    base_len = (size_t) (apostrophe - word);
    if (base_len >= sizeof(base)) {
        return 0;
    }

    memcpy(base, word, base_len);
    base[base_len] = '\0';

    memset(&temp_segments, 0, sizeof(temp_segments));
    if (!say_append_text_word(base, SAY_LANG_EN, &temp_segments)) {
        free(temp_segments.data);
        return -1;
    }

    suffix_result = say_append_english_contraction_suffix(&temp_segments, apostrophe + 1);
    if (suffix_result <= 0) {
        free(temp_segments.data);
        return suffix_result;
    }

    append_ok = say_segment_buffer_append(segments, &temp_segments);
    free(temp_segments.data);
    return append_ok ? 1 : -1;
}

static int say_append_text_word(const char *word, say_language_t language, segment_buffer_t *segments)
{
    int lexicon_result;
    int special_result;

    if (word[0] == '\0') {
        return 1;
    }

    lexicon_result = say_try_append_lexicon_word(word, language, segments);
    if (lexicon_result != 0) {
        return lexicon_result > 0;
    }

    special_result = say_try_append_compound_word(word, language, segments);
    if (special_result != 0) {
        return special_result > 0;
    }

    if (language == SAY_LANG_FR) {
        special_result = say_try_append_french_elision_word(word, segments);
    }
    else {
        special_result = say_try_append_english_contraction_word(word, segments);
    }
    if (special_result != 0) {
        return special_result > 0;
    }

    return say_append_simple_text_word(word, language, segments);
}

static const char *say_digit_word(char digit, say_language_t language)
{
    static const char *g_en_digits[] = {
        "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"
    };
    static const char *g_fr_digits[] = {
        "zero", "un", "deux", "trois", "quatre", "cinq", "six", "sept", "huit", "neuf"
    };

    if (digit < '0' || digit > '9') {
        return NULL;
    }
    if (language == SAY_LANG_FR) {
        return g_fr_digits[digit - '0'];
    }
    return g_en_digits[digit - '0'];
}

static int say_append_digit_word(char digit, say_language_t language, segment_buffer_t *segments)
{
    const char *word;

    word = say_digit_word(digit, language);
    if (word == NULL) {
        return 0;
    }
    return say_append_text_word(word, language, segments);
}

static int say_phonemize_text(
    const char *normalized_text,
    say_language_t language,
    segment_buffer_t *segments,
    char *error,
    size_t error_size
)
{
    size_t i;

    i = 0;
    while (normalized_text[i] != '\0') {
        if (say_is_boundary_char(normalized_text[i])) {
            int boundary_type;
            int repetitions;
            int n;

            switch (normalized_text[i]) {
                case ',':
                case ';':
                case ':':
                    boundary_type = 1;
                    repetitions = 1;
                    break;
                case '?':
                    boundary_type = 3;
                    repetitions = 2;
                    break;
                case '!':
                    boundary_type = 4;
                    repetitions = 2;
                    break;
                default:
                    boundary_type = 2;
                    repetitions = 2;
                    break;
            }

            for (n = 0; n < repetitions; ++n) {
                if (!say_segment_buffer_push(segments, PH_PAUSE, 1.0 + 0.35 * boundary_type, boundary_type)) {
                    say_set_error(error, error_size, "out of memory while adding pause");
                    return 0;
                }
            }
            ++i;
            continue;
        }
        if (normalized_text[i] == ' ') {
            ++i;
            continue;
        }
        if (normalized_text[i] >= '0' && normalized_text[i] <= '9') {
            if (!say_append_digit_word(normalized_text[i], language, segments)) {
                say_set_error(error, error_size, "out of memory while phonemizing digit");
                return 0;
            }
            ++i;
            continue;
        }
        if (say_is_token_char(normalized_text[i])) {
            size_t start;
            char word[128];
            size_t count;

            start = i;
            while (normalized_text[i] != '\0' && say_is_token_char(normalized_text[i])) {
                ++i;
            }

            count = i - start;
            if (count >= sizeof(word)) {
                count = sizeof(word) - 1;
            }
            memcpy(word, normalized_text + start, count);
            word[count] = '\0';

            if (!say_append_text_word(word, language, segments)) {
                say_set_error(error, error_size, "out of memory while phonemizing word");
                return 0;
            }
            continue;
        }
        ++i;
    }

    if (segments->count == 0) {
        say_set_error(error, error_size, "input did not produce any phonemes");
        return 0;
    }

    return 1;
}

static const phoneme_def_t *say_lookup_symbol(const char *symbol)
{
    size_t i;

    for (i = 0; i < PH_COUNT; ++i) {
        if (say_equals_icase(g_phonemes[i].symbol, symbol)) {
            return &g_phonemes[i];
        }
    }
    return NULL;
}

static int say_parse_phoneme_input(
    const char *input,
    segment_buffer_t *segments,
    char *error,
    size_t error_size
)
{
    char *copy;
    char *token;
    char *context;

    copy = _strdup(input);
    if (copy == NULL) {
        say_set_error(error, error_size, "out of memory while parsing phoneme input");
        return 0;
    }

    context = NULL;
    token = strtok_s(copy, " \t\r\n", &context);
    while (token != NULL) {
        const phoneme_def_t *phoneme;

        if (strcmp(token, "|") == 0 || strcmp(token, "/") == 0) {
            if (!say_segment_buffer_push(segments, PH_PAUSE, 1.3, 1)) {
                free(copy);
                say_set_error(error, error_size, "out of memory while adding phoneme pause");
                return 0;
            }
            token = strtok_s(NULL, " \t\r\n", &context);
            continue;
        }
        if (strcmp(token, ".") == 0 || strcmp(token, "!") == 0) {
            if (!say_segment_buffer_push(segments, PH_PAUSE, 1.7, 2)) {
                free(copy);
                say_set_error(error, error_size, "out of memory while adding phoneme pause");
                return 0;
            }
            token = strtok_s(NULL, " \t\r\n", &context);
            continue;
        }
        if (strcmp(token, "?") == 0) {
            if (!say_segment_buffer_push(segments, PH_PAUSE, 1.8, 3)) {
                free(copy);
                say_set_error(error, error_size, "out of memory while adding phoneme pause");
                return 0;
            }
            token = strtok_s(NULL, " \t\r\n", &context);
            continue;
        }

        phoneme = say_lookup_symbol(token);
        if (phoneme == NULL) {
            free(copy);
            say_set_error(error, error_size, "unknown phoneme symbol: %s", token);
            return 0;
        }
        if (!say_segment_buffer_push(segments, phoneme->id, 1.0, 0)) {
            free(copy);
            say_set_error(error, error_size, "out of memory while adding phoneme");
            return 0;
        }
        token = strtok_s(NULL, " \t\r\n", &context);
    }

    free(copy);

    if (segments->count == 0) {
        say_set_error(error, error_size, "phoneme input is empty");
        return 0;
    }
    return 1;
}

static int say_find_next_non_pause(const segment_t *segments, size_t count, size_t start_index)
{
    size_t i;

    for (i = start_index; i < count; ++i) {
        if (segments[i].phoneme != PH_PAUSE) {
            return (int) i;
        }
    }
    return -1;
}

static double say_pause_duration_ms(int boundary_type)
{
    switch (boundary_type) {
        case 1:
            return 120.0;
        case 3:
            return 190.0;
        case 4:
            return 180.0;
        case 2:
        default:
            return 170.0;
    }
}

static double say_glottal_pulse(double phase)
{
    double flow;

    if (phase < 0.46) {
        double x = phase / 0.46;
        flow = sin(0.5 * M_PI * x);
        flow *= flow;
    }
    else if (phase < 0.74) {
        double x = (phase - 0.46) / 0.28;
        flow = cos(x * (M_PI * 0.5));
    }
    else {
        double x = (phase - 0.74) / 0.26;
        flow = -0.12 * sin(M_PI * x);
    }

    return flow;
}

static double say_clamp01(double value)
{
    if (value < 0.0) {
        return 0.0;
    }
    if (value > 1.0) {
        return 1.0;
    }
    return value;
}

static double say_smoothstep01(double value)
{
    value = say_clamp01(value);
    return value * value * (3.0 - 2.0 * value);
}

static double say_transition_alpha(double position, double steady_ratio)
{
    if (position <= steady_ratio) {
        return 0.0;
    }
    return say_smoothstep01((position - steady_ratio) / (1.0 - steady_ratio));
}

static int say_is_plosive_phone(phoneme_id_t id)
{
    return id == PH_P || id == PH_B || id == PH_T || id == PH_D || id == PH_K || id == PH_G;
}

static int say_is_fricative_phone(phoneme_id_t id)
{
    return id == PH_F || id == PH_V || id == PH_S || id == PH_Z || id == PH_SH ||
           id == PH_ZH || id == PH_H || id == PH_TH || id == PH_DH;
}

static int say_is_affricate_phone(phoneme_id_t id)
{
    return id == PH_CH || id == PH_JH || id == PH_TS || id == PH_DZ;
}

static int say_is_sonorant_phone(phoneme_id_t id)
{
    return id == PH_W || id == PH_J || id == PH_R || id == PH_L || id == PH_M ||
           id == PH_N || id == PH_NY || id == PH_NG;
}

static int say_is_nasal_vowel_phone(phoneme_id_t id)
{
    return id == PH_AN || id == PH_ON || id == PH_IN;
}

static int say_is_sibilant_phone(phoneme_id_t id)
{
    return id == PH_S || id == PH_Z || id == PH_SH || id == PH_ZH;
}

static int say_is_dental_fricative_phone(phoneme_id_t id)
{
    return id == PH_TH || id == PH_DH;
}

static double say_lerp(double a, double b, double t)
{
    return a + (b - a) * t;
}

static double say_segment_progress(size_t index, size_t start, size_t end, double alpha)
{
    if (end <= start) {
        return say_clamp01(alpha);
    }
    return say_clamp01(((double) (index - start) + alpha) / (double) (end - start));
}

static int say_is_last_vowel_in_word(const segment_t *segments, size_t segment_count, size_t index)
{
    size_t i;

    if (!say_is_vowel_phone(segments[index].phoneme)) {
        return 0;
    }

    for (i = index + 1; i < segment_count; ++i) {
        if (say_is_vowel_phone(segments[i].phoneme)) {
            return 0;
        }
        if (segments[i].word_end || segments[i].phoneme == PH_PAUSE) {
            break;
        }
    }
    return 1;
}

static int say_is_clause_anchor(
    const segment_t *segments,
    size_t segment_count,
    say_language_t language,
    size_t index
)
{
    if (!say_is_vowel_phone(segments[index].phoneme) || segments[index].weak_word) {
        return 0;
    }
    if (language == SAY_LANG_EN) {
        return segments[index].stress >= 2;
    }
    return say_is_last_vowel_in_word(segments, segment_count, index);
}

static void say_analyze_clause(
    const segment_t *segments,
    size_t segment_count,
    say_language_t language,
    size_t start_index,
    clause_prosody_t *clause
)
{
    size_t i;

    clause->start = (size_t) -1;
    clause->end = (size_t) -1;
    clause->boundary_type = 2;
    clause->first_vowel = (size_t) -1;
    clause->last_vowel = (size_t) -1;
    clause->first_anchor = (size_t) -1;
    clause->nucleus = (size_t) -1;
    clause->anchor_count = 0;

    while (start_index < segment_count && segments[start_index].phoneme == PH_PAUSE) {
        ++start_index;
    }
    if (start_index >= segment_count) {
        return;
    }

    clause->start = start_index;
    clause->end = start_index;
    while (clause->end + 1 < segment_count && segments[clause->end + 1].phoneme != PH_PAUSE) {
        ++clause->end;
    }
    if (clause->end + 1 < segment_count && segments[clause->end + 1].phoneme == PH_PAUSE) {
        clause->boundary_type = segments[clause->end + 1].boundary_type;
    }

    for (i = clause->start; i <= clause->end; ++i) {
        if (say_is_vowel_phone(segments[i].phoneme)) {
            if (clause->first_vowel == (size_t) -1) {
                clause->first_vowel = i;
            }
            clause->last_vowel = i;
        }
        if (say_is_clause_anchor(segments, segment_count, language, i)) {
            if (clause->first_anchor == (size_t) -1) {
                clause->first_anchor = i;
            }
            clause->nucleus = i;
            ++clause->anchor_count;
        }
    }

    if (clause->first_anchor == (size_t) -1) {
        clause->first_anchor = clause->first_vowel;
        clause->nucleus = clause->last_vowel;
    }
}

static prosody_role_t say_clause_role_for_segment(const clause_prosody_t *clause, size_t index)
{
    if (clause->first_anchor == (size_t) -1 || clause->nucleus == (size_t) -1) {
        return SAY_PROSODY_PREHEAD;
    }
    if (index < clause->first_anchor) {
        return SAY_PROSODY_PREHEAD;
    }
    if (index == clause->nucleus) {
        return SAY_PROSODY_NUCLEUS;
    }
    if (index < clause->nucleus) {
        return SAY_PROSODY_HEAD;
    }
    return SAY_PROSODY_TAIL;
}

static prosody_tune_t say_select_tune(say_language_t language, int boundary_type)
{
    prosody_tune_t tune;

    if (language == SAY_LANG_FR) {
        tune.register_base = 118.0;
        tune.prehead_start = -4.0;
        tune.prehead_end = 0.0;
        tune.head_start = 3.5;
        tune.head_end = 0.0;
        tune.head_anchor_bump = 0.8;
        tune.head_unstressed_drop = -1.8;
        tune.nucleus_start = 8.0;
        tune.nucleus_end = 1.0;
        tune.nucleus_no_tail_end = -10.0;
        tune.tail_start = 1.0;
        tune.tail_end = -8.0;

        switch (boundary_type) {
            case 1:
                tune.prehead_start = -3.0;
                tune.prehead_end = 0.0;
                tune.head_start = 3.0;
                tune.head_end = 1.0;
                tune.head_anchor_bump = 0.6;
                tune.head_unstressed_drop = -1.4;
                tune.nucleus_start = 6.0;
                tune.nucleus_end = 4.0;
                tune.nucleus_no_tail_end = 4.0;
                tune.tail_start = 4.0;
                tune.tail_end = 5.0;
                break;
            case 3:
                tune.prehead_start = -3.0;
                tune.prehead_end = 0.0;
                tune.head_start = 3.0;
                tune.head_end = 1.0;
                tune.head_anchor_bump = 0.6;
                tune.head_unstressed_drop = -1.4;
                tune.nucleus_start = 4.0;
                tune.nucleus_end = 12.0;
                tune.nucleus_no_tail_end = 14.0;
                tune.tail_start = 12.0;
                tune.tail_end = 14.0;
                break;
            case 4:
                tune.prehead_start = -2.0;
                tune.prehead_end = 1.0;
                tune.head_start = 5.0;
                tune.head_end = 2.0;
                tune.head_anchor_bump = 0.8;
                tune.head_unstressed_drop = -1.0;
                tune.nucleus_start = 10.0;
                tune.nucleus_end = 0.0;
                tune.nucleus_no_tail_end = -8.0;
                tune.tail_start = 0.0;
                tune.tail_end = -6.0;
                break;
            default:
                break;
        }
        return tune;
    }

    tune.register_base = 124.0;
    tune.prehead_start = -8.0;
    tune.prehead_end = -2.0;
    tune.head_start = 12.0;
    tune.head_end = 4.0;
    tune.head_anchor_bump = 3.5;
    tune.head_unstressed_drop = -5.5;
    tune.nucleus_start = 15.0;
    tune.nucleus_end = 1.0;
    tune.nucleus_no_tail_end = -14.0;
    tune.tail_start = 1.0;
    tune.tail_end = -14.0;

    switch (boundary_type) {
        case 1:
            tune.prehead_start = -6.0;
            tune.prehead_end = 0.0;
            tune.head_start = 10.0;
            tune.head_end = 6.0;
            tune.head_anchor_bump = 2.5;
            tune.head_unstressed_drop = -4.0;
            tune.nucleus_start = 12.0;
            tune.nucleus_end = 6.0;
            tune.nucleus_no_tail_end = 4.0;
            tune.tail_start = 6.0;
            tune.tail_end = 2.0;
            break;
        case 3:
            tune.prehead_start = -6.0;
            tune.prehead_end = 0.0;
            tune.head_start = 9.0;
            tune.head_end = 4.0;
            tune.head_anchor_bump = 2.5;
            tune.head_unstressed_drop = -4.0;
            tune.nucleus_start = 4.0;
            tune.nucleus_end = 16.0;
            tune.nucleus_no_tail_end = 18.0;
            tune.tail_start = 16.0;
            tune.tail_end = 18.0;
            break;
        case 4:
            tune.prehead_start = -4.0;
            tune.prehead_end = 2.0;
            tune.head_start = 14.0;
            tune.head_end = 8.0;
            tune.head_anchor_bump = 3.0;
            tune.head_unstressed_drop = -3.0;
            tune.nucleus_start = 18.0;
            tune.nucleus_end = 2.0;
            tune.nucleus_no_tail_end = -10.0;
            tune.tail_start = 2.0;
            tune.tail_end = -10.0;
            break;
        default:
            break;
    }
    return tune;
}

static double say_clause_pitch_offset(
    const segment_t *segments,
    size_t segment_count,
    say_language_t language,
    const clause_prosody_t *clause,
    const prosody_tune_t *tune,
    size_t index,
    double alpha
)
{
    prosody_role_t role;
    double t;
    double pitch;
    size_t head_end;
    size_t tail_start;
    int anchor;

    (void) segment_count;

    if (clause->start == (size_t) -1 || clause->first_vowel == (size_t) -1) {
        return 0.0;
    }

    role = say_clause_role_for_segment(clause, index);
    anchor = say_is_clause_anchor(segments, segment_count, language, index);

    switch (role) {
        case SAY_PROSODY_PREHEAD:
            t = say_segment_progress(
                index,
                clause->start,
                clause->first_anchor > clause->start ? clause->first_anchor - 1 : clause->start,
                alpha);
            return say_lerp(tune->prehead_start, tune->prehead_end, say_smoothstep01(t));

        case SAY_PROSODY_HEAD:
            head_end = clause->nucleus > clause->first_anchor ? clause->nucleus - 1 : clause->first_anchor;
            t = say_segment_progress(index, clause->first_anchor, head_end, alpha);
            pitch = say_lerp(tune->head_start, tune->head_end, say_smoothstep01(t));
            if (say_is_vowel_phone(segments[index].phoneme)) {
                pitch += anchor ? tune->head_anchor_bump : tune->head_unstressed_drop;
            }
            else if (segments[index].weak_word) {
                pitch += 0.65 * tune->head_unstressed_drop;
            }
            return pitch;

        case SAY_PROSODY_NUCLEUS:
            if (clause->last_vowel == clause->nucleus || clause->nucleus == clause->end) {
                return say_lerp(tune->nucleus_start, tune->nucleus_no_tail_end, say_smoothstep01(alpha));
            }
            return say_lerp(tune->nucleus_start, tune->nucleus_end, say_smoothstep01(alpha));

        case SAY_PROSODY_TAIL:
        default:
            tail_start = clause->nucleus < clause->end ? clause->nucleus + 1 : clause->nucleus;
            t = say_segment_progress(index, tail_start, clause->end, alpha);
            pitch = say_lerp(tune->tail_start, tune->tail_end, say_smoothstep01(t));
            if (say_is_vowel_phone(segments[index].phoneme) && segments[index].weak_word) {
                pitch += 0.35 * tune->head_unstressed_drop;
            }
            return pitch;
    }
}

static int say_generate_frames(
    const segment_t *segments,
    size_t segment_count,
    const say_options_t *options,
    frame_buffer_t *frames,
    char *error,
    size_t error_size
)
{
    size_t i;
    clause_prosody_t clause;

    clause.start = (size_t) -1;
    clause.end = (size_t) -1;
    clause.boundary_type = 2;
    clause.first_vowel = (size_t) -1;
    clause.last_vowel = (size_t) -1;
    clause.first_anchor = (size_t) -1;
    clause.nucleus = (size_t) -1;
    clause.anchor_count = 0;

    for (i = 0; i < segment_count; ++i) {
        const phoneme_def_t *current;
        const phoneme_def_t *target;
        prosody_role_t prosody_role;
        prosody_tune_t tune;
        double duration_ms;
        int frame_count;
        size_t frame_index;
        frame_t frame;
        double base_pitch;
        double stress_boost;
        double steady_ratio;
        int dental_fricative;
        int word_final_fricative;
        int word_final_sibilant;
        int next_index;
        int vowel_count_in_word;
        int clause_anchor;
        int clause_nucleus;
        size_t j;

        current = say_get_phoneme(segments[i].phoneme);
        if (segments[i].phoneme == PH_PAUSE) {
            duration_ms = say_pause_duration_ms(segments[i].boundary_type) * segments[i].duration_scale;
            if (options->language == SAY_LANG_EN && segments[i].boundary_type >= 2) {
                duration_ms *= 0.78;
            }
            frame_count = (int) ceil(duration_ms / options->frame_ms);
            if (frame_count < 1) {
                frame_count = 1;
            }
            memset(&frame, 0, sizeof(frame));
            frame.is_pause = 1;
            frame.voiced = 0;
            for (frame_index = 0; frame_index < (size_t) frame_count; ++frame_index) {
                if (!say_frame_buffer_push(frames, &frame)) {
                    say_set_error(error, error_size, "out of memory while generating pause frames");
                    return 0;
                }
            }
            continue;
        }

        if (clause.start == (size_t) -1 || i > clause.end) {
            say_analyze_clause(segments, segment_count, options->language, i, &clause);
        }
        prosody_role = say_clause_role_for_segment(&clause, i);
        tune = say_select_tune(options->language, clause.boundary_type);
        clause_anchor = say_is_clause_anchor(segments, segment_count, options->language, i);
        clause_nucleus = clause.nucleus != (size_t) -1 && i == clause.nucleus;

        next_index = say_find_next_non_pause(segments, segment_count, i + 1);
        target = next_index >= 0 ? say_get_phoneme(segments[next_index].phoneme) : current;
        dental_fricative = say_is_dental_fricative_phone(segments[i].phoneme);
        word_final_fricative = segments[i].word_end &&
            (say_is_fricative_phone(segments[i].phoneme) || say_is_affricate_phone(segments[i].phoneme));
        word_final_sibilant = segments[i].word_end && say_is_sibilant_phone(segments[i].phoneme);
        if (word_final_fricative) {
            target = current;
        }
        duration_ms = current->base_ms * segments[i].duration_scale;

        vowel_count_in_word = 0;
        if (say_is_vowel_phone(segments[i].phoneme)) {
            for (j = i; j < segment_count && !segments[j].word_end; ++j) {
                if (say_is_vowel_phone(segments[j].phoneme)) {
                    ++vowel_count_in_word;
                }
            }
            if (segments[j].word_end && say_is_vowel_phone(segments[j].phoneme)) {
                ++vowel_count_in_word;
            }
        }

        base_pitch = tune.register_base;

        stress_boost = 0.0;
        if (say_is_vowel_phone(segments[i].phoneme)) {
            if (options->language == SAY_LANG_EN) {
                if (segments[i].weak_word) {
                    stress_boost = segments[i].phoneme == PH_SCHWA ? -0.58 : -0.46;
                }
                else if (clause_nucleus) {
                    stress_boost = 1.18;
                }
                else if (clause_anchor) {
                    stress_boost = vowel_count_in_word <= 1 ? 0.82 : 0.66;
                }
                else if (prosody_role == SAY_PROSODY_PREHEAD) {
                    stress_boost = segments[i].phoneme == PH_SCHWA ? -0.42 : -0.28;
                }
                else if (segments[i].phoneme == PH_SCHWA) {
                    stress_boost = -0.35;
                }
                else if (prosody_role == SAY_PROSODY_TAIL) {
                    stress_boost = -0.12;
                }
                else {
                    stress_boost = -0.06;
                }
            }
            else {
                if (segments[i].weak_word) {
                    stress_boost = segments[i].phoneme == PH_SCHWA ? -0.56 : -0.34;
                }
                else if (clause_nucleus) {
                    stress_boost = say_is_nasal_vowel_phone(segments[i].phoneme) ? 0.48 : 0.58;
                }
                else if (clause_anchor) {
                    stress_boost = 0.08;
                }
                else if (segments[i].phoneme == PH_SCHWA) {
                    stress_boost = -0.26;
                }
                else if (prosody_role == SAY_PROSODY_PREHEAD) {
                    stress_boost = -0.08;
                }
                else {
                    stress_boost = -0.02;
                }
            }
        }
        else if (segments[i].weak_word) {
            if (options->language == SAY_LANG_EN) {
                stress_boost = current->voiced ? -0.14 : -0.20;
            }
            else {
                stress_boost = current->voiced ? -0.10 : -0.16;
            }
        }
        else if (clause_nucleus && current->voiced) {
            stress_boost = options->language == SAY_LANG_FR ? 0.10 : 0.18;
        }
        else if (clause_anchor && current->voiced) {
            stress_boost = options->language == SAY_LANG_FR ? 0.04 : 0.10;
        }
        else if (current->voiced) {
            stress_boost = options->language == SAY_LANG_FR ? 0.04 : 0.08;
        }

        duration_ms *= 1.0 + (options->language == SAY_LANG_FR ? 0.12 : 0.16) * stress_boost;
        if (options->language == SAY_LANG_EN) {
            if (say_is_vowel_phone(segments[i].phoneme)) {
                if (segments[i].weak_word) {
                    duration_ms *= segments[i].phoneme == PH_SCHWA ? 0.84 : 0.90;
                }
                else if (clause_nucleus) {
                    duration_ms *= vowel_count_in_word <= 1 ? 1.16 : 1.22;
                }
                else if (clause_anchor) {
                    duration_ms *= vowel_count_in_word <= 1 ? 1.08 : 1.12;
                }
                else if (prosody_role == SAY_PROSODY_PREHEAD) {
                    duration_ms *= 0.92;
                }
                else if (prosody_role == SAY_PROSODY_TAIL) {
                    duration_ms *= 0.95;
                }
            }
            else if (segments[i].weak_word) {
                duration_ms *= current->voiced ? 0.94 : 0.92;
            }
        }
        if (say_is_vowel_phone(segments[i].phoneme) && options->language == SAY_LANG_FR) {
            if (segments[i].weak_word) {
                duration_ms *= segments[i].phoneme == PH_SCHWA ? 0.76 : 0.82;
            }
            else if (clause_nucleus) {
                duration_ms *= vowel_count_in_word <= 1 ? 1.10 : 1.14;
            }
            else if (clause_anchor) {
                duration_ms *= 0.98;
            }
            else if (segments[i].phoneme == PH_SCHWA) {
                duration_ms *= 0.84;
            }
            else {
                duration_ms *= 0.96;
            }
            if (say_is_nasal_vowel_phone(segments[i].phoneme)) {
                duration_ms *= 1.05;
            }
        }
        else if (options->language == SAY_LANG_FR &&
                 (segments[i].phoneme == PH_J || segments[i].phoneme == PH_W)) {
            duration_ms *= 0.82;
        }
        if (options->language == SAY_LANG_FR && say_is_sibilant_phone(segments[i].phoneme)) {
            duration_ms *= current->voiced ? 0.90 : 0.86;
        }
        else if (options->language == SAY_LANG_EN && say_is_sibilant_phone(segments[i].phoneme)) {
            duration_ms *= current->voiced ? 1.08 : 1.14;
        }
        else if (options->language == SAY_LANG_EN && say_is_fricative_phone(segments[i].phoneme)) {
            duration_ms *= current->voiced ? 1.06 : 1.12;
        }
        else if (options->language == SAY_LANG_EN && say_is_affricate_phone(segments[i].phoneme)) {
            duration_ms *= current->voiced ? 1.08 : 1.14;
        }
        if (options->language == SAY_LANG_EN &&
            segments[i].word_end &&
            (segments[i].phoneme == PH_W || segments[i].phoneme == PH_J)) {
            duration_ms *= 0.72;
        }
        if (word_final_fricative) {
            if (options->language == SAY_LANG_EN) {
                duration_ms *= word_final_sibilant ? 1.03 : 1.01;
            }
            else {
                duration_ms *= word_final_sibilant ? 1.04 : 1.02;
            }
        }
        if (options->language == SAY_LANG_EN && dental_fricative) {
            duration_ms *= current->voiced ? 1.10 : 1.14;
        }

        frame_count = (int) ceil(duration_ms / options->frame_ms);
        if (frame_count < 1) {
            frame_count = 1;
        }

        for (frame_index = 0; frame_index < (size_t) frame_count; ++frame_index) {
            double alpha;
            double transition_alpha;
            double segment_envelope;
            double local_noise_mix;
            int aff_fric_phase;

            aff_fric_phase = 0;
            alpha = frame_count == 1 ? 0.0 : (double) frame_index / (double) (frame_count - 1);
            steady_ratio = say_is_vowel_phone(segments[i].phoneme) ?
                (options->language == SAY_LANG_FR ? 0.68 : 0.62) :
                (options->language == SAY_LANG_FR ? 0.46 : 0.42);
            if (options->language == SAY_LANG_FR && say_is_nasal_vowel_phone(segments[i].phoneme)) {
                steady_ratio = 0.74;
            }
            else if (word_final_fricative) {
                steady_ratio = options->language == SAY_LANG_EN ? 0.64 : 0.70;
            }
            else if (options->language == SAY_LANG_EN && dental_fricative) {
                steady_ratio = 0.50;
            }
            else if (options->language == SAY_LANG_EN && say_is_sibilant_phone(segments[i].phoneme)) {
                steady_ratio = 0.46;
            }
            else if (options->language == SAY_LANG_EN &&
                     (say_is_fricative_phone(segments[i].phoneme) || say_is_affricate_phone(segments[i].phoneme))) {
                steady_ratio = 0.44;
            }
            transition_alpha = say_transition_alpha(alpha, steady_ratio);
            segment_envelope = 1.0;
            local_noise_mix = current->noise_mix;

            if (say_is_vowel_phone(segments[i].phoneme)) {
                local_noise_mix *= options->language == SAY_LANG_FR ? 0.24 : 0.30;
                segment_envelope = options->language == SAY_LANG_FR ?
                    (0.95 + 0.05 * sin(alpha * M_PI)) :
                    (0.94 + 0.06 * sin(alpha * M_PI));
                if (options->language == SAY_LANG_FR && say_is_nasal_vowel_phone(segments[i].phoneme)) {
                    local_noise_mix *= 0.72;
                    segment_envelope = 0.96 + 0.04 * sin(alpha * M_PI);
                }
            }
            else if (say_is_sonorant_phone(segments[i].phoneme)) {
                local_noise_mix *= options->language == SAY_LANG_FR ? 0.16 : 0.22;
                segment_envelope = options->language == SAY_LANG_FR ?
                    (0.88 + 0.10 * sin(alpha * M_PI)) :
                    (0.86 + 0.14 * sin(alpha * M_PI));
            }
            else if (say_is_plosive_phone(segments[i].phoneme)) {
                double closure = options->language == SAY_LANG_FR ? 0.34 : 0.30;
                double burst_attack = say_smoothstep01((alpha - (options->language == SAY_LANG_FR ? 0.36 : 0.32)) /
                                                       (options->language == SAY_LANG_FR ? 0.10 : 0.12));
                double burst_decay = 1.0 - say_smoothstep01((alpha - (options->language == SAY_LANG_FR ? 0.52 : 0.58)) /
                                                            (options->language == SAY_LANG_FR ? 0.14 : 0.18));
                double burst = burst_attack * burst_decay;
                if (alpha < closure) {
                    segment_envelope = 0.03;
                    local_noise_mix = 0.0;
                }
                else {
                    if (options->language == SAY_LANG_FR) {
                        segment_envelope = current->voiced ? (0.08 + 0.24 * burst) : (0.08 + 0.28 * burst);
                        local_noise_mix = current->voiced ? (0.02 + 0.05 * burst) : (0.08 + 0.20 * burst);
                    }
                    else {
                        segment_envelope = current->voiced ? (0.12 + 0.34 * burst) : (0.10 + 0.40 * burst);
                        local_noise_mix = current->voiced ? (0.04 + 0.08 * burst) : (0.10 + 0.28 * burst);
                    }
                }
            }
            else if (say_is_affricate_phone(segments[i].phoneme)) {
                local_noise_mix = current->voiced ?
                    (options->language == SAY_LANG_FR ? 0.18 : 0.22) :
                    (options->language == SAY_LANG_FR ? 0.38 : 0.46);
                segment_envelope = options->language == SAY_LANG_FR ?
                    (0.52 + 0.12 * sin(alpha * M_PI)) :
                    (0.56 + 0.16 * sin(alpha * M_PI));
                if (options->language == SAY_LANG_EN) {
                    const double aff_closure_end = 0.30;
                    const double aff_burst_end   = 0.48;
                    if (alpha < aff_closure_end) {
                        /* Phase 1: closure — near-silence like a plosive */
                        local_noise_mix = 0.0;
                        segment_envelope = 0.03;
                    } else if (alpha < aff_burst_end) {
                        /* Phase 2: burst — broadband noise transient */
                        double burst_t = say_smoothstep01((alpha - aff_closure_end) / (aff_burst_end - aff_closure_end));
                        local_noise_mix = burst_t * 0.60;
                        segment_envelope = 0.04 + burst_t * (current->voiced ? 1.28 : 1.32);
                    } else {
                        /* Phase 3: fricative — SH character for CH, ZH for JH */
                        double fric_alpha = (alpha - aff_burst_end) / (1.0 - aff_burst_end);
                        local_noise_mix = current->voiced ? 0.38 : 0.44;
                        segment_envelope = current->voiced ?
                            (0.62 + 0.14 * sin(fric_alpha * M_PI)) :
                            (0.58 + 0.18 * sin(fric_alpha * M_PI));
                        aff_fric_phase = 1;
                    }
                }
            }
            else if (say_is_fricative_phone(segments[i].phoneme)) {
                if (segments[i].phoneme == PH_H) {
                    local_noise_mix = options->language == SAY_LANG_EN ? 0.36 : 0.30;
                    segment_envelope = options->language == SAY_LANG_EN ?
                        (0.48 + 0.12 * sin(alpha * M_PI)) :
                        (0.42 + 0.12 * sin(alpha * M_PI));
                }
                else {
                    if (options->language == SAY_LANG_FR) {
                        if (say_is_sibilant_phone(segments[i].phoneme)) {
                            local_noise_mix = current->voiced ? 0.08 : 0.24;
                            if (segments[i].phoneme == PH_ZH) {
                                local_noise_mix *= 0.85;
                            }
                            segment_envelope = current->voiced ? 0.50 : 0.44;
                        }
                        else {
                            local_noise_mix = current->voiced ? 0.12 : 0.34;
                            segment_envelope = current->voiced ? 0.60 : 0.54;
                        }
                    }
                    else {
                        if (dental_fricative) {
                            int strong_dh = current->voiced && !segments[i].weak_word;
                            local_noise_mix = strong_dh ? 0.22 : (current->voiced ? 0.18 : 0.22);
                            segment_envelope = strong_dh ? 0.70 : (current->voiced ? 0.60 : 0.94);
                        }
                        else if (say_is_sibilant_phone(segments[i].phoneme)) {
                            double voiced_z_nm = segments[i].weak_word ? 0.20 : 0.26;
                            local_noise_mix = current->voiced ? voiced_z_nm : 0.44;
                            if (segments[i].phoneme == PH_ZH) {
                                local_noise_mix *= 0.88;
                            }
                            else if (segments[i].phoneme == PH_SH) {
                                local_noise_mix *= 0.92;
                            }
                            segment_envelope = current->voiced ?
                                (0.56 + 0.08 * sin(alpha * M_PI)) :
                                (0.50 + 0.10 * sin(alpha * M_PI));
                        }
                        else {
                            local_noise_mix = current->voiced ? 0.28 : 0.42;
                            segment_envelope = current->voiced ? 0.66 : 0.58;
                        }
                        if (word_final_fricative) {
                            local_noise_mix *= options->language == SAY_LANG_EN ? 0.96 : 1.03;
                            segment_envelope += options->language == SAY_LANG_EN ? 0.00 : 0.02;
                        }
                    }
                }
            }

            memset(&frame, 0, sizeof(frame));
            frame.voiced = current->voiced;
            frame.is_pause = 0;
            if (say_is_plosive_phone(segments[i].phoneme) && alpha < 0.30) {
                frame.voiced = 0;
            }
            if (options->language == SAY_LANG_EN && say_is_affricate_phone(segments[i].phoneme) && alpha < 0.30) {
                frame.voiced = 0;
            }
            frame.pitch_hz = base_pitch +
                say_clause_pitch_offset(segments, segment_count, options->language, &clause, &tune, i, alpha) +
                (options->language == SAY_LANG_FR ? 7.5 : 10.0) * stress_boost;
            if (options->language == SAY_LANG_FR && say_is_nasal_vowel_phone(segments[i].phoneme)) {
                frame.pitch_hz -= 1.2;
            }
            frame.amplitude = current->amplitude *
                (options->language == SAY_LANG_FR ? (0.90 + 0.10 * stress_boost) : (0.88 + 0.14 * stress_boost)) *
                segment_envelope;
            frame.noise_mix = local_noise_mix;
            if (options->language == SAY_LANG_EN) {
                phoneme_noise_path_t np = say_get_noise_path_en(segments[i].phoneme);
                frame.noise_path_mix  = np.noise_path_mix;
                frame.noise_path_f_low  = np.f_low;
                frame.noise_path_f_high = np.f_high;
                frame.noise_path_gain = np.gain;
                frame.voicing_bar_amp = np.voicing_bar_amp;
            }
            if (options->language == SAY_LANG_FR && say_is_nasal_vowel_phone(segments[i].phoneme)) {
                frame.amplitude *= 0.95;
            }
            if (options->language == SAY_LANG_FR && say_is_sibilant_phone(segments[i].phoneme)) {
                frame.amplitude *= current->voiced ? 0.88 : 0.82;
            }
            if (options->language == SAY_LANG_EN && say_is_sibilant_phone(segments[i].phoneme)) {
                frame.amplitude *= current->voiced ? 0.92 : 0.94;
                if (segments[i].phoneme == PH_SH || segments[i].phoneme == PH_ZH) {
                    frame.amplitude *= 0.96;
                }
            }
            else if (options->language == SAY_LANG_EN && say_is_fricative_phone(segments[i].phoneme)) {
                frame.amplitude *= current->voiced ? 0.94 : 0.96;
            }
            else if (options->language == SAY_LANG_EN && say_is_affricate_phone(segments[i].phoneme)) {
                frame.amplitude *= current->voiced ? 1.12 : 1.18;
            }
            if (options->language == SAY_LANG_EN && dental_fricative) {
                frame.amplitude *= current->voiced ? 0.94 : 0.96;
            }
            if (word_final_fricative) {
                frame.amplitude *= options->language == SAY_LANG_EN ? 0.98 : 1.02;
            }

            for (j = 0; j < SAY_MAX_FORMANTS; ++j) {
                frame.formant_freq[j] = current->formant_freq[j] + (target->formant_freq[j] - current->formant_freq[j]) * transition_alpha;
                frame.bandwidth[j] = current->bandwidth[j] + (target->bandwidth[j] - current->bandwidth[j]) * transition_alpha;
                frame.gain[j] = current->gain[j] + (target->gain[j] - current->gain[j]) * transition_alpha;
            }
            if (options->language == SAY_LANG_EN && aff_fric_phase) {
                /* Blend toward SH (CH) or ZH (JH) formants in the fricative phase */
                const phoneme_def_t *fric_ph = say_get_phoneme(current->voiced ? PH_ZH : PH_SH);
                double fric_t = say_smoothstep01((alpha - 0.48) / 0.20);
                for (j = 0; j < SAY_MAX_FORMANTS; ++j) {
                    frame.formant_freq[j] = frame.formant_freq[j] * (1.0 - fric_t) + fric_ph->formant_freq[j] * fric_t;
                    frame.bandwidth[j]    = frame.bandwidth[j]    * (1.0 - fric_t) + fric_ph->bandwidth[j]    * fric_t;
                    frame.gain[j]         = frame.gain[j]         * (1.0 - fric_t) + fric_ph->gain[j]         * fric_t;
                }
            }
            if (options->language == SAY_LANG_FR && say_is_nasal_vowel_phone(segments[i].phoneme)) {
                frame.bandwidth[0] *= 1.12;
                frame.bandwidth[1] *= 1.10;
                frame.bandwidth[2] *= 1.20;
                frame.bandwidth[3] *= 1.24;
                frame.bandwidth[4] *= 1.28;
                frame.gain[0] *= 1.05;
                frame.gain[1] *= 0.92;
                frame.gain[2] *= 0.78;
                frame.gain[3] *= 0.72;
                frame.gain[4] *= 0.66;
            }
            else if (options->language == SAY_LANG_FR && say_is_sibilant_phone(segments[i].phoneme)) {
                frame.bandwidth[0] *= 1.04;
                frame.bandwidth[1] *= 1.08;
                frame.bandwidth[2] *= 1.10;
                frame.bandwidth[3] *= 1.14;
                frame.bandwidth[4] *= 1.18;
                frame.gain[0] *= 0.92;
                frame.gain[1] *= 0.88;
                frame.gain[2] *= 0.86;
                frame.gain[3] *= 0.78;
                frame.gain[4] *= 0.70;
            }
            else if (options->language == SAY_LANG_EN && say_is_sibilant_phone(segments[i].phoneme)) {
                frame.bandwidth[0] *= 1.08;
                frame.bandwidth[1] *= 1.02;
                frame.bandwidth[2] *= 0.96;
                frame.bandwidth[3] *= 0.92;
                frame.bandwidth[4] *= 0.90;
                frame.gain[0] *= 0.58;
                frame.gain[1] *= 0.72;
                frame.gain[2] *= current->voiced ? 1.00 : 1.06;
                frame.gain[3] *= current->voiced ? 1.02 : 1.10;
                frame.gain[4] *= current->voiced ? 0.96 : 1.04;
                if (segments[i].phoneme == PH_SH || segments[i].phoneme == PH_ZH) {
                    frame.gain[2] *= 1.02;
                    frame.gain[3] *= 0.94;
                    frame.gain[4] *= 0.88;
                    frame.bandwidth[3] *= 1.02;
                    frame.bandwidth[4] *= 1.04;
                }
            }
            else if (options->language == SAY_LANG_EN && say_is_affricate_phone(segments[i].phoneme)) {
                if (aff_fric_phase) {
                    /* Fricative phase: use SH/ZH-like spectral shaping */
                    frame.bandwidth[0] *= 1.08;
                    frame.bandwidth[1] *= 1.02;
                    frame.bandwidth[2] *= 0.96;
                    frame.bandwidth[3] *= 0.92;
                    frame.bandwidth[4] *= 0.90;
                    frame.gain[0] *= current->voiced ? 0.60 : 0.58;
                    frame.gain[1] *= current->voiced ? 0.72 : 0.72;
                    frame.gain[2] *= current->voiced ? 1.00 : 1.06;
                    frame.gain[3] *= current->voiced ? 1.02 : 1.10;
                    frame.gain[4] *= current->voiced ? 0.96 : 1.04;
                } else {
                    /* Closure/burst phases: broadband emphasis */
                    frame.bandwidth[0] *= 1.04;
                    frame.bandwidth[1] *= 1.00;
                    frame.bandwidth[2] *= 0.96;
                    frame.bandwidth[3] *= 0.92;
                    frame.bandwidth[4] *= 0.90;
                    frame.gain[0] *= current->voiced ? 0.62 : 0.78;
                    frame.gain[1] *= current->voiced ? 0.74 : 0.86;
                    frame.gain[2] *= current->voiced ? 1.12 : 1.12;
                    frame.gain[3] *= current->voiced ? 1.24 : 1.20;
                    frame.gain[4] *= current->voiced ? 1.36 : 1.24;
                }
            }
            else if (options->language == SAY_LANG_EN && dental_fricative && !current->voiced) {
                frame.bandwidth[0] *= 1.14;
                frame.bandwidth[1] *= 1.10;
                frame.bandwidth[2] *= 1.06;
                frame.bandwidth[3] *= 1.00;
                frame.bandwidth[4] *= 0.92;
                frame.gain[0] *= 1.00;
                frame.gain[1] *= 1.00;
                frame.gain[2] *= 0.80;
                frame.gain[3] *= 1.00;
                frame.gain[4] *= 1.00;
            }
            else if (options->language == SAY_LANG_EN && dental_fricative && current->voiced) {
                frame.bandwidth[0] *= 1.08;
                frame.bandwidth[1] *= 1.06;
                frame.bandwidth[2] *= 1.02;
                frame.bandwidth[3] *= 0.96;
                frame.bandwidth[4] *= 0.94;
                frame.gain[0] *= 0.60;
                frame.gain[1] *= 0.78;
                frame.gain[2] *= 1.06;
                frame.gain[3] *= 1.12;
                frame.gain[4] *= 1.02;
            }
            else if (options->language == SAY_LANG_EN && say_is_fricative_phone(segments[i].phoneme)) {
                frame.bandwidth[0] *= 1.04;
                frame.bandwidth[1] *= 1.02;
                frame.bandwidth[2] *= 0.98;
                frame.bandwidth[3] *= 0.95;
                frame.bandwidth[4] *= 0.94;
                frame.gain[0] *= 0.82;
                frame.gain[1] *= 0.90;
                frame.gain[2] *= current->voiced ? 1.04 : 1.08;
                frame.gain[3] *= current->voiced ? 1.10 : 1.16;
                frame.gain[4] *= current->voiced ? 1.12 : 1.18;
            }
            if (word_final_sibilant) {
                frame.gain[3] *= options->language == SAY_LANG_EN ? 1.00 : 1.03;
                frame.gain[4] *= options->language == SAY_LANG_EN ? 1.01 : 1.04;
            }

            if (!say_frame_buffer_push(frames, &frame)) {
                say_set_error(error, error_size, "out of memory while generating speech frames");
                return 0;
            }
        }
    }

    if (frames->count == 0) {
        say_set_error(error, error_size, "frame generator produced no output");
        return 0;
    }
    return 1;
}

static void say_biquad_set_bandpass(biquad_t *filter, double sample_rate, double frequency, double bandwidth, double gain)
{
    double q;
    double w0;
    double sin_w0;
    double cos_w0;
    double alpha;
    double a0;

    if (frequency < 80.0) {
        frequency = 80.0;
    }
    if (frequency > sample_rate * 0.45) {
        frequency = sample_rate * 0.45;
    }
    if (bandwidth < 30.0) {
        bandwidth = 30.0;
    }

    q = frequency / bandwidth;
    if (q < 0.25) {
        q = 0.25;
    }

    w0 = 2.0 * M_PI * frequency / sample_rate;
    sin_w0 = sin(w0);
    cos_w0 = cos(w0);
    alpha = sin_w0 / (2.0 * q);
    a0 = 1.0 + alpha;

    filter->b0 = gain * (alpha / a0);
    filter->b1 = 0.0;
    filter->b2 = gain * (-alpha / a0);
    filter->a1 = -2.0 * cos_w0 / a0;
    filter->a2 = (1.0 - alpha) / a0;
}

static void say_biquad_set_highpass(biquad_t *filter, double sample_rate, double frequency)
{
    double w0, sin_w0, cos_w0, alpha, a0;

    if (frequency < 20.0)                       frequency = 20.0;
    if (frequency > sample_rate * 0.45)         frequency = sample_rate * 0.45;

    w0     = 2.0 * M_PI * frequency / sample_rate;
    sin_w0 = sin(w0);
    cos_w0 = cos(w0);
    alpha  = sin_w0 / (2.0 * 0.7071);
    a0     = 1.0 + alpha;

    filter->b0 =  (1.0 + cos_w0) / (2.0 * a0);
    filter->b1 = -(1.0 + cos_w0) / a0;
    filter->b2 =  (1.0 + cos_w0) / (2.0 * a0);
    filter->a1 = -2.0 * cos_w0 / a0;
    filter->a2 = (1.0 - alpha) / a0;
}

static void say_biquad_set_lowpass(biquad_t *filter, double sample_rate, double frequency)
{
    double w0, sin_w0, cos_w0, alpha, a0;

    if (frequency < 20.0)                       frequency = 20.0;
    if (frequency > sample_rate * 0.45)         frequency = sample_rate * 0.45;

    w0     = 2.0 * M_PI * frequency / sample_rate;
    sin_w0 = sin(w0);
    cos_w0 = cos(w0);
    alpha  = sin_w0 / (2.0 * 0.7071);
    a0     = 1.0 + alpha;

    filter->b0 = (1.0 - cos_w0) / (2.0 * a0);
    filter->b1 = (1.0 - cos_w0) / a0;
    filter->b2 = (1.0 - cos_w0) / (2.0 * a0);
    filter->a1 = -2.0 * cos_w0 / a0;
    filter->a2 = (1.0 - alpha) / a0;
}

static double say_biquad_process(biquad_t *filter, double input)
{
    double output;

    output = filter->b0 * input + filter->z1;
    filter->z1 = filter->b1 * input - filter->a1 * output + filter->z2;
    filter->z2 = filter->b2 * input - filter->a2 * output;
    return output;
}

static int say_synthesize_frames(
    const frame_t *frames,
    size_t frame_count,
    int sample_rate,
    int frame_ms,
    int16_t **out_samples,
    size_t *out_sample_count,
    char *error,
    size_t error_size
)
{
    biquad_t voiced_filters[SAY_MAX_FORMANTS];
    biquad_t noise_filters[SAY_MAX_FORMANTS];
    size_t i;
    int64_t frame_sample_accum;
    size_t total_samples;
    double *mix;
    int16_t *samples;
    size_t sample_index;
    double phase;
    unsigned int rng;
    double amplitude_state;
    double pitch_state;
    double noise_mix_state;
    double voicing_state;
    double formant_freq_state[SAY_MAX_FORMANTS];
    double bandwidth_state[SAY_MAX_FORMANTS];
    double gain_state[SAY_MAX_FORMANTS];
    double source_state;
    double source_hp_x1;
    double source_hp_y1;
    double noise_hp1_x1;
    double noise_hp1_y1;
    double noise_hp2_x1;
    double noise_hp2_y1;
    double noise_soft_state;
    double hp_x1;
    double hp_y1;
    double limiter_env;
    double peak;
    double jitter_state;
    double jitter_target;
    int jitter_countdown;
    int state_ready;
    biquad_t noise_path_hp;
    biquad_t noise_path_lp;
    double np_mix_state;
    double np_f_low_state;
    double np_f_high_state;
    double np_gain_state;
    double vbar_lp_state;
    double vbar_amp_state;

    memset(voiced_filters, 0, sizeof(voiced_filters));
    memset(noise_filters, 0, sizeof(noise_filters));
    frame_sample_accum = 0;
    total_samples = 0;
    for (i = 0; i < frame_count; ++i) {
        int frame_samples;
        frame_sample_accum += (int64_t) sample_rate * (int64_t) frame_ms;
        frame_samples = (int) (frame_sample_accum / 1000);
        frame_sample_accum %= 1000;
        total_samples += (size_t) frame_samples;
    }

    mix = (double *) malloc(total_samples * sizeof(*mix));
    if (mix == NULL) {
        say_set_error(error, error_size, "out of memory while allocating %zu synthesis samples", total_samples);
        return 0;
    }

    phase = 0.0;
    rng = 0x12345678u;
    sample_index = 0;
    frame_sample_accum = 0;
    amplitude_state = 0.0;
    pitch_state = 120.0;
    noise_mix_state = 0.0;
    voicing_state = 0.0;
    source_state = 0.0;
    source_hp_x1 = 0.0;
    source_hp_y1 = 0.0;
    noise_hp1_x1 = 0.0;
    noise_hp1_y1 = 0.0;
    noise_hp2_x1 = 0.0;
    noise_hp2_y1 = 0.0;
    noise_soft_state = 0.0;
    hp_x1 = 0.0;
    hp_y1 = 0.0;
    limiter_env = 0.0;
    peak = 0.0;
    jitter_state = 0.0;
    jitter_target = 0.0;
    jitter_countdown = 0;
    state_ready = 0;
    memset(formant_freq_state, 0, sizeof(formant_freq_state));
    memset(bandwidth_state, 0, sizeof(bandwidth_state));
    memset(gain_state, 0, sizeof(gain_state));
    memset(&noise_path_hp, 0, sizeof(noise_path_hp));
    memset(&noise_path_lp, 0, sizeof(noise_path_lp));
    np_mix_state  = 0.0;
    np_f_low_state  = 1000.0;
    np_f_high_state = 4000.0;
    np_gain_state = 0.0;
    vbar_lp_state = 0.0;
    vbar_amp_state = 0.0;

    for (i = 0; i < frame_count; ++i) {
        size_t j;
        int frame_samples;
        double target_amplitude;
        double target_pitch;
        double target_noise_mix;
        double target_voicing;

        target_amplitude = frames[i].is_pause ? 0.0 : frames[i].amplitude;
        target_pitch = frames[i].pitch_hz > 1.0 ? frames[i].pitch_hz : pitch_state;
        target_noise_mix = frames[i].is_pause ? 0.0 : frames[i].noise_mix;
        target_voicing = frames[i].voiced ? 1.0 : 0.0;

        if (!state_ready) {
            for (j = 0; j < SAY_MAX_FORMANTS; ++j) {
                formant_freq_state[j] = frames[i].formant_freq[j];
                bandwidth_state[j] = frames[i].bandwidth[j];
                gain_state[j] = frames[i].gain[j];
                say_biquad_set_bandpass(&voiced_filters[j], (double) sample_rate, formant_freq_state[j], bandwidth_state[j], gain_state[j]);
                say_biquad_set_bandpass(&noise_filters[j], (double) sample_rate, formant_freq_state[j], bandwidth_state[j], gain_state[j]);
            }
            amplitude_state = target_amplitude;
            pitch_state = target_pitch;
            noise_mix_state = target_noise_mix;
            voicing_state = target_voicing;
            np_mix_state  = frames[i].noise_path_mix;
            if (frames[i].noise_path_f_low  > 0.0) np_f_low_state  = frames[i].noise_path_f_low;
            if (frames[i].noise_path_f_high > np_f_low_state) np_f_high_state = frames[i].noise_path_f_high;
            np_gain_state = frames[i].noise_path_gain;
            vbar_amp_state = frames[i].voicing_bar_amp;
            state_ready = 1;
        }

        frame_sample_accum += (int64_t) sample_rate * (int64_t) frame_ms;
        frame_samples = (int) (frame_sample_accum / 1000);
        frame_sample_accum %= 1000;

        for (j = 0; j < (size_t) frame_samples; ++j) {
            double glottal;
            double noise;
            double excitation;
            double voiced_excitation;
            double noise_excitation;
            double noise_raw;
            double voiced_mix;
            double voiced_output;
            double noise_output;
            double noise_path_out;
            double voicing_bar_out;
            double output;
            double source_hp;
            double noise_hp1;
            double noise_hp2;
            double noise_soft;
            double amplitude_rate;
            double noise_rate;
            double voicing_rate;
            size_t k;

            amplitude_rate = target_amplitude > amplitude_state ? 0.007 : 0.030;
            noise_rate = target_noise_mix > noise_mix_state ? 0.010 : 0.040;
            voicing_rate = target_voicing > voicing_state ? 0.014 : 0.050;

            amplitude_state += amplitude_rate * (target_amplitude - amplitude_state);
            pitch_state += 0.012 * (target_pitch - pitch_state);
            noise_mix_state += noise_rate * (target_noise_mix - noise_mix_state);
            voicing_state += voicing_rate * (target_voicing - voicing_state);

            np_mix_state += 0.012 * (frames[i].noise_path_mix - np_mix_state);
            np_gain_state += 0.020 * (frames[i].noise_path_gain - np_gain_state);
            vbar_amp_state += 0.020 * (frames[i].voicing_bar_amp - vbar_amp_state);
            if (frames[i].noise_path_f_low > 0.0) {
                np_f_low_state  += 0.020 * (frames[i].noise_path_f_low  - np_f_low_state);
                np_f_high_state += 0.020 * (frames[i].noise_path_f_high - np_f_high_state);
            }

            for (k = 0; k < SAY_MAX_FORMANTS; ++k) {
                double noise_focus;
                double sibilant_focus;
                double noise_gain_mul;
                double noise_bandwidth;

                formant_freq_state[k] += 0.025 * (frames[i].formant_freq[k] - formant_freq_state[k]);
                bandwidth_state[k] += 0.025 * (frames[i].bandwidth[k] - bandwidth_state[k]);
                gain_state[k] += 0.025 * (frames[i].gain[k] - gain_state[k]);

                noise_focus = say_clamp01((frames[i].noise_mix - 0.14) / 0.26);
                sibilant_focus = say_clamp01((frames[i].noise_mix - 0.24) / 0.18);
                switch (k) {
                    case 0:
                        noise_gain_mul = 1.0 - 0.86 * noise_focus;
                        break;
                    case 1:
                        noise_gain_mul = 1.0 - 0.64 * noise_focus;
                        break;
                    case 2:
                        noise_gain_mul = 1.0 - 0.12 * noise_focus;
                        break;
                    case 3:
                        noise_gain_mul = 1.0 + 0.16 * noise_focus;
                        break;
                    default:
                        noise_gain_mul = 1.0 + 0.28 * noise_focus;
                        break;
                }
                if (k < 2) {
                    noise_gain_mul *= 1.0 - 0.16 * sibilant_focus;
                }
                else if (k >= 3) {
                    noise_gain_mul *= 1.0 + 0.12 * sibilant_focus;
                }

                noise_bandwidth = bandwidth_state[k] * (1.10 + 0.26 * noise_focus);
                if (k < 2) {
                    noise_bandwidth *= 1.12 + 0.10 * sibilant_focus;
                }
                else if (k >= 3) {
                    noise_bandwidth *= 1.04 + 0.08 * sibilant_focus;
                }

                say_biquad_set_bandpass(&voiced_filters[k], (double) sample_rate, formant_freq_state[k], bandwidth_state[k], gain_state[k]);
                say_biquad_set_bandpass(&noise_filters[k], (double) sample_rate, formant_freq_state[k], noise_bandwidth, gain_state[k] * noise_gain_mul);
            }
            if (np_mix_state > 0.001) {
                say_biquad_set_highpass(&noise_path_hp, (double) sample_rate, np_f_low_state);
                say_biquad_set_lowpass (&noise_path_lp, (double) sample_rate, np_f_high_state);
            }

            if (voicing_state > 0.02 && pitch_state > 1.0) {
                if (jitter_countdown <= 0) {
                    rng = rng * 1664525u + 1013904223u;
                    jitter_target = ((((double) ((rng >> 8) & 0xFFFFu) / 32768.0) - 1.0) * 0.0045);
                    jitter_countdown = sample_rate / 180;
                    if (jitter_countdown < 1) {
                        jitter_countdown = 1;
                    }
                }
                --jitter_countdown;
                jitter_state += 0.010 * (jitter_target - jitter_state);
                phase += (pitch_state * (1.0 + jitter_state)) / (double) sample_rate;
                if (phase >= 1.0) {
                    phase -= floor(phase);
                }
                glottal = say_glottal_pulse(phase);
            }
            else {
                glottal = 0.0;
                jitter_state *= 0.98;
            }

            rng = rng * 1664525u + 1013904223u;
            noise = ((double) ((rng >> 8) & 0x00FFFFFFu) / 8388608.0) - 1.0;

            voiced_mix = voicing_state * (1.0 - noise_mix_state);
            voiced_excitation = voiced_mix * glottal;
            source_hp = voiced_excitation - source_hp_x1 + 0.972 * source_hp_y1;
            source_hp_x1 = voiced_excitation;
            source_hp_y1 = source_hp;
            source_state += 0.16 * (source_hp - source_state);
            voiced_excitation = 0.80 * source_state + 0.20 * source_hp;

            noise_hp1 = noise - noise_hp1_x1 + 0.985 * noise_hp1_y1;
            noise_hp1_x1 = noise;
            noise_hp1_y1 = noise_hp1;
            noise_hp2 = noise_hp1 - noise_hp2_x1 + 0.940 * noise_hp2_y1;
            noise_hp2_x1 = noise_hp1;
            noise_hp2_y1 = noise_hp2;
            noise_soft_state += 0.18 * (noise_hp2 - noise_soft_state);
            noise_soft = noise_soft_state;
            noise_raw = 0.44 * noise_hp1 + 0.26 * noise_hp2 + 0.30 * noise_soft;
            noise_excitation = noise_mix_state * noise_raw;

            voiced_output = 0.0;
            noise_output = 0.0;
            for (k = 0; k < SAY_MAX_FORMANTS; ++k) {
                voiced_output += say_biquad_process(&voiced_filters[k], voiced_excitation);
                noise_output += say_biquad_process(&noise_filters[k], noise_excitation);
            }

            /* Dedicated noise path: bandpass-shaped white noise bypassing formant filters */
            if (np_mix_state > 0.001) {
                double hp_out = say_biquad_process(&noise_path_hp, noise_raw);
                noise_path_out = say_biquad_process(&noise_path_lp, hp_out) * np_gain_state;
            } else {
                noise_path_out = 0.0;
            }

            /* Voicing bar: LP-filtered glottal source below ~250 Hz */
            vbar_lp_state += 0.072 * (glottal * voicing_state - vbar_lp_state);
            voicing_bar_out = vbar_lp_state * vbar_amp_state;

            excitation = voiced_excitation + noise_excitation;
            output = voiced_output
                   + (1.0 - np_mix_state) * noise_output
                   + np_mix_state * noise_path_out
                   + voicing_bar_out;

            if (frames[i].is_pause) {
                output *= 0.08;
                for (k = 0; k < SAY_MAX_FORMANTS; ++k) {
                    voiced_filters[k].z1 *= 0.84;
                    voiced_filters[k].z2 *= 0.84;
                    noise_filters[k].z1 *= 0.84;
                    noise_filters[k].z2 *= 0.84;
                }
                noise_path_hp.z1 *= 0.84;
                noise_path_hp.z2 *= 0.84;
                noise_path_lp.z1 *= 0.84;
                noise_path_lp.z2 *= 0.84;
                vbar_lp_state *= 0.84;
            }

            output *= amplitude_state * 0.86;
            {
                double hp_output = output - hp_x1 + 0.995 * hp_y1;
                hp_x1 = output;
                hp_y1 = hp_output;
                output = hp_output;
            }
            {
                double abs_output = fabs(output);
                double threshold = 0.34;
                double release = 0.9985;

                if (abs_output > limiter_env) {
                    limiter_env = abs_output;
                }
                else {
                    limiter_env *= release;
                }

                if (limiter_env > threshold) {
                    output *= threshold / limiter_env;
                }
            }
            mix[sample_index++] = output;
            if (fabs(output) > peak) {
                peak = fabs(output);
            }
        }
    }

    samples = (int16_t *) malloc(total_samples * sizeof(*samples));
    if (samples == NULL) {
        free(mix);
        say_set_error(error, error_size, "out of memory while allocating %zu pcm samples", total_samples);
        return 0;
    }

    if (peak < 1e-6) {
        peak = 1.0;
    }

    for (i = 0; i < sample_index; ++i) {
        double scaled = mix[i] * (0.82 / peak);
        double shaped = tanh(scaled * 1.15) * 0.96;
        if (shaped > 0.98) {
            shaped = 0.98;
        }
        if (shaped < -0.98) {
            shaped = -0.98;
        }
        samples[i] = (int16_t) lrint(shaped * 32767.0);
    }

    free(mix);
    *out_samples = samples;
    *out_sample_count = sample_index;
    return 1;
}

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

static void say_release_pipeline_buffers(
    char *normalized,
    segment_buffer_t *segments,
    frame_buffer_t *frames
)
{
    free(normalized);
    if (segments != NULL) {
        free(segments->data);
        memset(segments, 0, sizeof(*segments));
    }
    if (frames != NULL) {
        free(frames->data);
        memset(frames, 0, sizeof(*frames));
    }
}

static int say_prepare_pipeline(
    const char *input,
    const say_options_t *options,
    char **out_normalized,
    segment_buffer_t *segments,
    frame_buffer_t *frames,
    char *error,
    size_t error_size
)
{
    say_options_t resolved_options;
    char *normalized;

    if (input == NULL || options == NULL || segments == NULL || frames == NULL) {
        say_set_error(error, error_size, "pipeline arguments must not be null");
        return 0;
    }

    resolved_options = *options;
    if (resolved_options.sample_rate != 44100) {
        say_set_error(error, error_size, "sample rate must be 44100 Hz");
        return 0;
    }
    if (resolved_options.frame_ms < 5 || resolved_options.frame_ms > 10) {
        say_set_error(error, error_size, "frame size must be between 5 and 10 ms");
        return 0;
    }

    memset(segments, 0, sizeof(*segments));
    memset(frames, 0, sizeof(*frames));
    normalized = NULL;

    if (resolved_options.phoneme_input) {
        if (!say_parse_phoneme_input(input, segments, error, error_size)) {
            say_release_pipeline_buffers(NULL, segments, frames);
            return 0;
        }
    }
    else {
        normalized = say_normalize_text(input, error, error_size);
        if (normalized == NULL) {
            say_release_pipeline_buffers(NULL, segments, frames);
            return 0;
        }
        if (!say_phonemize_text(normalized, resolved_options.language, segments, error, error_size)) {
            say_release_pipeline_buffers(normalized, segments, frames);
            return 0;
        }
    }

    if (!say_generate_frames(segments->data, segments->count, &resolved_options, frames, error, error_size)) {
        say_release_pipeline_buffers(normalized, segments, frames);
        return 0;
    }

    if (out_normalized != NULL) {
        *out_normalized = normalized;
    }
    else {
        free(normalized);
    }
    return 1;
}

static const char *say_boundary_name(int boundary_type)
{
    switch (boundary_type) {
        case 1:
            return "clause";
        case 2:
            return "sentence";
        case 3:
            return "question";
        case 4:
            return "exclamation";
        default:
            return "manual";
    }
}

static int say_text_buffer_append_phoneme_stream(
    text_buffer_t *buffer,
    const segment_t *segments,
    size_t segment_count
)
{
    size_t i;
    int first;

    first = 1;
    for (i = 0; i < segment_count; ++i) {
        const phoneme_def_t *phoneme;

        if (segments[i].phoneme == PH_PAUSE) {
            size_t run;

            run = 1;
            while (i + run < segment_count && segments[i + run].phoneme == PH_PAUSE &&
                   segments[i + run].boundary_type == segments[i].boundary_type) {
                ++run;
            }
            if (!first && !say_text_buffer_append(buffer, " ")) {
                return 0;
            }
            if (!say_text_buffer_appendf(buffer, "[pause x%zu:%s]", run, say_boundary_name(segments[i].boundary_type))) {
                return 0;
            }
            first = 0;
            i += run - 1;
            continue;
        }

        phoneme = say_get_phoneme(segments[i].phoneme);
        if (!first) {
            if (!say_text_buffer_append(buffer, segments[i].word_start ? " / " : " ")) {
                return 0;
            }
        }
        if (segments[i].stress > 0) {
            if (!say_text_buffer_append(buffer, "'")) {
                return 0;
            }
        }
        if (!say_text_buffer_append(buffer, phoneme->symbol)) {
            return 0;
        }
        first = 0;
    }
    return 1;
}

static int say_text_buffer_append_word_debug(
    text_buffer_t *buffer,
    const char *normalized_text,
    say_language_t language,
    char *error,
    size_t error_size
)
{
    size_t i;

    if (normalized_text == NULL) {
        return 1;
    }

    i = 0;
    while (normalized_text[i] != '\0') {
        if (say_is_boundary_char(normalized_text[i])) {
            if (!say_text_buffer_appendf(buffer, "  [pause:%s]\n", say_boundary_name(
                normalized_text[i] == ',' || normalized_text[i] == ';' || normalized_text[i] == ':' ? 1 :
                normalized_text[i] == '?' ? 3 :
                normalized_text[i] == '!' ? 4 : 2))) {
                return 0;
            }
            ++i;
            continue;
        }
        if (normalized_text[i] == ' ') {
            ++i;
            continue;
        }
        if (normalized_text[i] >= '0' && normalized_text[i] <= '9') {
            segment_buffer_t temp_segments;
            const char *digit_word;

            memset(&temp_segments, 0, sizeof(temp_segments));
            digit_word = say_digit_word(normalized_text[i], language);
            if (digit_word == NULL || !say_append_text_word(digit_word, language, &temp_segments)) {
                free(temp_segments.data);
                say_set_error(error, error_size, "failed to debug digit token %c", normalized_text[i]);
                return 0;
            }
            if (!say_text_buffer_appendf(buffer, "  %c -> %s -> ", normalized_text[i], digit_word) ||
                !say_text_buffer_append_phoneme_stream(buffer, temp_segments.data, temp_segments.count) ||
                !say_text_buffer_append(buffer, "\n")) {
                free(temp_segments.data);
                return 0;
            }
            free(temp_segments.data);
            ++i;
            continue;
        }
        if (say_is_token_char(normalized_text[i])) {
            size_t start;
            char word[128];
            size_t count;
            segment_buffer_t temp_segments;

            start = i;
            while (normalized_text[i] != '\0' && say_is_token_char(normalized_text[i])) {
                ++i;
            }

            count = i - start;
            if (count >= sizeof(word)) {
                count = sizeof(word) - 1;
            }
            memcpy(word, normalized_text + start, count);
            word[count] = '\0';

            memset(&temp_segments, 0, sizeof(temp_segments));
            if (!say_append_text_word(word, language, &temp_segments)) {
                free(temp_segments.data);
                say_set_error(error, error_size, "failed to debug word token %s", word);
                return 0;
            }
            if (!say_text_buffer_appendf(buffer, "  %s -> ", word) ||
                !say_text_buffer_append_phoneme_stream(buffer, temp_segments.data, temp_segments.count) ||
                !say_text_buffer_append(buffer, "\n")) {
                free(temp_segments.data);
                return 0;
            }
            free(temp_segments.data);
            continue;
        }
        ++i;
    }

    return 1;
}

static int say_text_buffer_append_segment_summary(
    text_buffer_t *buffer,
    const segment_t *segments,
    size_t segment_count
)
{
    size_t i;

    for (i = 0; i < segment_count; ++i) {
        if (segments[i].phoneme == PH_PAUSE) {
            if (!say_text_buffer_appendf(
                buffer,
                "  %02zu  %-10s nominal_ms=%.1f boundary=%s scale=%.2f weak=%d\n",
                i,
                "PAUSE",
                say_pause_duration_ms(segments[i].boundary_type) * segments[i].duration_scale,
                say_boundary_name(segments[i].boundary_type),
                segments[i].duration_scale,
                segments[i].weak_word)) {
                return 0;
            }
        }
        else {
            const phoneme_def_t *phoneme;

            phoneme = say_get_phoneme(segments[i].phoneme);
            if (!say_text_buffer_appendf(
                buffer,
                "  %02zu  %-10s nominal_ms=%.1f voiced=%d vowel=%d noise=%.2f scale=%.2f weak=%d stress=%d\n",
                i,
                phoneme->symbol,
                phoneme->base_ms * segments[i].duration_scale,
                phoneme->voiced,
                phoneme->is_vowel,
                phoneme->noise_mix,
                segments[i].duration_scale,
                segments[i].weak_word,
                segments[i].stress)) {
                return 0;
            }
        }
    }
    return 1;
}

static int say_text_buffer_append_frame_stats(
    text_buffer_t *buffer,
    const frame_t *frames,
    size_t frame_count,
    int frame_ms
)
{
    size_t i;
    size_t voiced_count;
    size_t unvoiced_count;
    size_t pause_count;
    double pitch_min;
    double pitch_max;
    double pitch_sum;
    double amplitude_min;
    double amplitude_max;
    double amplitude_sum;
    double noise_min;
    double noise_max;
    double noise_sum;

    voiced_count = 0;
    unvoiced_count = 0;
    pause_count = 0;
    pitch_min = 1e9;
    pitch_max = 0.0;
    pitch_sum = 0.0;
    amplitude_min = 1e9;
    amplitude_max = 0.0;
    amplitude_sum = 0.0;
    noise_min = 1e9;
    noise_max = 0.0;
    noise_sum = 0.0;

    for (i = 0; i < frame_count; ++i) {
        if (frames[i].is_pause) {
            ++pause_count;
            continue;
        }
        if (frames[i].voiced) {
            ++voiced_count;
            if (frames[i].pitch_hz < pitch_min) {
                pitch_min = frames[i].pitch_hz;
            }
            if (frames[i].pitch_hz > pitch_max) {
                pitch_max = frames[i].pitch_hz;
            }
            pitch_sum += frames[i].pitch_hz;
        }
        else {
            ++unvoiced_count;
        }
        if (frames[i].amplitude < amplitude_min) {
            amplitude_min = frames[i].amplitude;
        }
        if (frames[i].amplitude > amplitude_max) {
            amplitude_max = frames[i].amplitude;
        }
        amplitude_sum += frames[i].amplitude;
        if (frames[i].noise_mix < noise_min) {
            noise_min = frames[i].noise_mix;
        }
        if (frames[i].noise_mix > noise_max) {
            noise_max = frames[i].noise_mix;
        }
        noise_sum += frames[i].noise_mix;
    }

    if (pitch_min > pitch_max) {
        pitch_min = 0.0;
    }
    if (amplitude_min > amplitude_max) {
        amplitude_min = 0.0;
    }
    if (noise_min > noise_max) {
        noise_min = 0.0;
    }

    return say_text_buffer_appendf(
        buffer,
        "frame_count: %zu\n"
        "frame_duration_ms: %d\n"
        "estimated_duration_ms: %.1f\n"
        "voiced_frames: %zu\n"
        "unvoiced_frames: %zu\n"
        "pause_frames: %zu\n"
        "pitch_hz: min=%.2f avg=%.2f max=%.2f\n"
        "amplitude: min=%.3f avg=%.3f max=%.3f\n"
        "noise_mix: min=%.3f avg=%.3f max=%.3f\n",
        frame_count,
        frame_ms,
        (double) frame_count * (double) frame_ms,
        voiced_count,
        unvoiced_count,
        pause_count,
        pitch_min,
        voiced_count > 0 ? pitch_sum / (double) voiced_count : 0.0,
        pitch_max,
        amplitude_min,
        (voiced_count + unvoiced_count) > 0 ? amplitude_sum / (double) (voiced_count + unvoiced_count) : 0.0,
        amplitude_max,
        noise_min,
        (voiced_count + unvoiced_count) > 0 ? noise_sum / (double) (voiced_count + unvoiced_count) : 0.0,
        noise_max);
}

void say_default_options(say_options_t *options)
{
    if (options == NULL) {
        return;
    }
    options->sample_rate = 44100;
    options->frame_ms = 5;
    options->language = SAY_LANG_EN;
    options->phoneme_input = 0;
}

const char *say_language_name(say_language_t language)
{
    switch (language) {
        case SAY_LANG_FR:
            return "fr";
        case SAY_LANG_EN:
        default:
            return "en";
    }
}

const char *say_audio_format_name(say_audio_format_t format)
{
    switch (format) {
        case SAY_FORMAT_AIFF:
            return "aiff";
        case SAY_FORMAT_WAV:
            return "wav";
        case SAY_FORMAT_RAW:
        default:
            return "raw";
    }
}

int say_parse_language(const char *name, say_language_t *out_language)
{
    if (name == NULL || out_language == NULL) {
        return 0;
    }
    if (say_equals_icase(name, "en")) {
        *out_language = SAY_LANG_EN;
        return 1;
    }
    if (say_equals_icase(name, "fr")) {
        *out_language = SAY_LANG_FR;
        return 1;
    }
    return 0;
}

int say_parse_audio_format(const char *name, say_audio_format_t *out_format)
{
    if (name == NULL || out_format == NULL) {
        return 0;
    }
    if (say_equals_icase(name, "raw")) {
        *out_format = SAY_FORMAT_RAW;
        return 1;
    }
    if (say_equals_icase(name, "aiff") || say_equals_icase(name, "aif")) {
        *out_format = SAY_FORMAT_AIFF;
        return 1;
    }
    if (say_equals_icase(name, "wav") || say_equals_icase(name, "wave")) {
        *out_format = SAY_FORMAT_WAV;
        return 1;
    }
    return 0;
}

say_audio_format_t say_guess_audio_format(const char *path)
{
    const char *dot;

    if (path == NULL) {
        return SAY_FORMAT_RAW;
    }

    dot = strrchr(path, '.');
    if (dot != NULL) {
        if (say_equals_icase(dot, ".aiff") || say_equals_icase(dot, ".aif")) {
            return SAY_FORMAT_AIFF;
        }
        if (say_equals_icase(dot, ".wav") || say_equals_icase(dot, ".wave")) {
            return SAY_FORMAT_WAV;
        }
    }
    return SAY_FORMAT_RAW;
}

int say_build_debug_report(
    const char *input,
    const say_options_t *options,
    char **out_report,
    char *error,
    size_t error_size
)
{
    say_options_t resolved_options;
    char *normalized;
    segment_buffer_t segments;
    frame_buffer_t frames;
    text_buffer_t report;
    int ok;

    if (out_report == NULL) {
        say_set_error(error, error_size, "debug report output pointer must not be null");
        return 0;
    }
    if (options == NULL) {
        say_set_error(error, error_size, "options must not be null");
        return 0;
    }

    resolved_options = *options;
    normalized = NULL;
    ok = 0;
    memset(&segments, 0, sizeof(segments));
    memset(&frames, 0, sizeof(frames));
    memset(&report, 0, sizeof(report));
    *out_report = NULL;

    if (!say_prepare_pipeline(input, &resolved_options, &normalized, &segments, &frames, error, error_size)) {
        goto cleanup;
    }

    if (!say_text_buffer_append(&report, "lib-say debug report\n") ||
        !say_text_buffer_append(&report, "====================\n") ||
        !say_text_buffer_appendf(&report, "input_mode: %s\n", resolved_options.phoneme_input ? "phonemes" : "text") ||
        !say_text_buffer_appendf(&report, "language: %s\n", say_language_name(resolved_options.language)) ||
        !say_text_buffer_appendf(&report, "sample_rate: %d\n", resolved_options.sample_rate) ||
        !say_text_buffer_appendf(&report, "frame_ms: %d\n", resolved_options.frame_ms) ||
        !say_text_buffer_appendf(&report, "raw_input: %s\n", input != NULL ? input : "")) {
        say_set_error(error, error_size, "out of memory while building debug report header");
        goto cleanup;
    }

    if (normalized != NULL) {
        if (!say_text_buffer_appendf(&report, "normalized_text: %s\n", normalized) ||
            !say_text_buffer_append(&report, "\nword_to_phoneme:\n") ||
            !say_text_buffer_append_word_debug(&report, normalized, resolved_options.language, error, error_size)) {
            say_set_error(error, error_size, "out of memory while building word debug section");
            goto cleanup;
        }
    }

    if (!say_text_buffer_append(&report, "\nphoneme_stream:\n  ") ||
        !say_text_buffer_append_phoneme_stream(&report, segments.data, segments.count) ||
        !say_text_buffer_append(&report, "\n\nsegments:\n") ||
        !say_text_buffer_appendf(&report, "segment_count: %zu\n", segments.count) ||
        !say_text_buffer_append_segment_summary(&report, segments.data, segments.count) ||
        !say_text_buffer_append(&report, "\nframes:\n") ||
        !say_text_buffer_append_frame_stats(&report, frames.data, frames.count, resolved_options.frame_ms)) {
        say_set_error(error, error_size, "out of memory while building debug report body");
        goto cleanup;
    }

    *out_report = report.data;
    report.data = NULL;
    ok = 1;

cleanup:
    free(report.data);
    say_release_pipeline_buffers(normalized, &segments, &frames);
    return ok;
}

int say_synthesize(
    const char *input,
    const say_options_t *options,
    int16_t **out_samples,
    size_t *out_sample_count,
    char *error,
    size_t error_size
)
{
    say_options_t resolved_options;
    char *normalized;
    segment_buffer_t segments;
    frame_buffer_t frames;
    int ok;

    if (out_samples == NULL || out_sample_count == NULL) {
        say_set_error(error, error_size, "output pointers must not be null");
        return 0;
    }
    if (options == NULL) {
        say_set_error(error, error_size, "options must not be null");
        return 0;
    }

    normalized = NULL;
    ok = 0;
    memset(&segments, 0, sizeof(segments));
    memset(&frames, 0, sizeof(frames));
    *out_samples = NULL;
    *out_sample_count = 0;
    resolved_options = *options;
    if (!say_prepare_pipeline(input, &resolved_options, &normalized, &segments, &frames, error, error_size)) {
        goto cleanup;
    }
    if (!say_synthesize_frames(frames.data, frames.count, resolved_options.sample_rate, resolved_options.frame_ms, out_samples, out_sample_count, error, error_size)) {
        goto cleanup;
    }

    ok = 1;

cleanup:
    say_release_pipeline_buffers(normalized, &segments, &frames);
    return ok;
}

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

void say_free(void *ptr)
{
    free(ptr);
}
