#include "say_internal.h"

#include <string.h>

/* ---------------------------------------------------------------------------
 * Phoneme inventory: the canonical table of acoustic / articulatory data for
 * every phoneme this synthesizer knows about. Each row carries:
 *   - identity & legacy is_vowel/voiced ints (kept for ternary call sites),
 *   - C1 feature bit-mask (the source of truth for predicate queries),
 *   - B1 two-tier durations: min_ms (floor) + base_ms (INHDUR equivalent),
 *   - amplitude / noise_mix tuning,
 *   - 5-formant target frequencies, bandwidths, and gains.
 * Composite macros below keep the bit-mask column compact at the cost of being
 * scoped (#defined immediately above the table, #undef'd right after).
 * ------------------------------------------------------------------------- */

#define VOW       (F_VOWEL | F_VOICED)
#define NVW       (VOW | F_NASAL_VOWEL)
#define GLD       (F_VOICED | F_SONOR | F_GLIDE)
#define LIQ       (F_VOICED | F_SONOR | F_LIQUID)
#define NSL       (F_VOICED | F_SONOR | F_NASAL)
#define PLS       (F_PLOS)
#define PLSV      (F_PLOS | F_VOICED)
#define FRC       (F_FRIC)
#define FRCV      (F_FRIC | F_VOICED)
#define SIB       (F_FRIC | F_SIBILANT)
#define SIBV      (F_FRIC | F_VOICED | F_SIBILANT)
#define DEN       (F_FRIC | F_DENTAL)
#define DENV      (F_FRIC | F_VOICED | F_DENTAL)
#define AFR       (F_AFFRIC)
#define AFRV      (F_AFFRIC | F_VOICED)

static const phoneme_def_t g_phonemes[PH_COUNT] = {
    /* { id, sym, vowel, voiced, FEATURES, MIN, INH, amp, noise, F-freq[5], BW[5], gain[5] } */
    { PH_PAUSE, "PAUSE", 0, 0, F_PAUSE, 50.0,  90.0, 0.00, 0.00, { 0 }, { 0 }, { 0 } },
    { PH_A,     "A",     1, 1, VOW,  60.0, 115.0, 1.00, 0.02, { 800, 1200, 2800, 3600, 4500 }, { 90, 100, 150, 200, 260 }, { 1.00, 0.85, 0.35, 0.16, 0.08 } },
    { PH_AE,    "AE",    1, 1, VOW,  60.0, 110.0, 1.00, 0.02, { 700, 1700, 2500, 3600, 4500 }, { 90, 110, 150, 200, 260 }, { 1.00, 0.82, 0.34, 0.15, 0.07 } },
    { PH_AH,    "AH",    1, 1, VOW,  55.0, 105.0, 0.96, 0.03, { 650, 1200, 2500, 3400, 4400 }, { 100, 120, 160, 200, 260 }, { 0.98, 0.78, 0.30, 0.14, 0.07 } },
    { PH_E,     "E",     1, 1, VOW,  58.0, 108.0, 0.98, 0.02, { 500, 1900, 2600, 3400, 4300 }, { 90, 100, 130, 180, 240 }, { 0.98, 0.84, 0.34, 0.14, 0.07 } },
    { PH_EH,    "EH",    1, 1, VOW,  56.0, 104.0, 0.98, 0.02, { 600, 1700, 2500, 3400, 4300 }, { 95, 105, 135, 185, 240 }, { 0.98, 0.83, 0.34, 0.15, 0.07 } },
    { PH_I,     "I",     1, 1, VOW,  58.0, 108.0, 0.98, 0.02, { 300, 2200, 3000, 3800, 4700 }, { 70, 95, 120, 180, 240 }, { 0.96, 0.82, 0.32, 0.14, 0.06 } },
    { PH_IH,    "IH",    1, 1, VOW,  52.0,  96.0, 0.92, 0.02, { 400, 2000, 2800, 3600, 4500 }, { 80, 100, 130, 185, 240 }, { 0.92, 0.78, 0.30, 0.13, 0.06 } },
    { PH_O,     "O",     1, 1, VOW,  60.0, 110.0, 1.00, 0.02, { 450,  900, 2400, 3400, 4300 }, { 90, 100, 140, 200, 240 }, { 1.00, 0.84, 0.34, 0.14, 0.06 } },
    { PH_OH,    "OH",    1, 1, VOW,  58.0, 108.0, 0.98, 0.02, { 550,  900, 2400, 3400, 4300 }, { 95, 105, 145, 200, 240 }, { 0.98, 0.84, 0.34, 0.14, 0.06 } },
    { PH_U,     "U",     1, 1, VOW,  60.0, 112.0, 0.96, 0.02, { 350,  800, 2200, 3200, 4200 }, { 70, 95, 130, 190, 240 }, { 0.96, 0.80, 0.30, 0.13, 0.06 } },
    { PH_Y,     "Y",     1, 1, VOW,  58.0, 110.0, 0.96, 0.02, { 320, 1750, 2400, 3300, 4200 }, { 75, 100, 130, 180, 240 }, { 0.96, 0.80, 0.30, 0.13, 0.06 } },
    { PH_EU,    "EU",    1, 1, VOW,  56.0, 106.0, 0.95, 0.02, { 400, 1500, 2400, 3300, 4200 }, { 80, 100, 130, 180, 240 }, { 0.95, 0.80, 0.30, 0.13, 0.06 } },
    { PH_SCHWA, "SCHWA", 1, 1, VOW,  30.0,  85.0, 0.88, 0.03, { 500, 1500, 2500, 3400, 4300 }, { 100, 120, 160, 210, 260 }, { 0.88, 0.74, 0.28, 0.12, 0.06 } },
    { PH_AN,    "AN",    1, 1, NVW,  65.0, 118.0, 0.96, 0.04, { 650, 1100, 2300, 3200, 4100 }, { 110, 130, 170, 210, 260 }, { 0.94, 0.76, 0.30, 0.12, 0.06 } },
    { PH_ON,    "ON",    1, 1, NVW,  65.0, 116.0, 0.96, 0.04, { 500,  900, 2200, 3200, 4100 }, { 110, 130, 170, 210, 260 }, { 0.94, 0.76, 0.30, 0.12, 0.06 } },
    { PH_IN,    "IN",    1, 1, NVW,  62.0, 114.0, 0.95, 0.04, { 450, 1800, 2500, 3400, 4300 }, { 110, 125, 160, 210, 260 }, { 0.94, 0.80, 0.30, 0.12, 0.06 } },
    { PH_W,     "W",     0, 1, GLD,  38.0,  72.0, 0.70, 0.03, { 320,  800, 2200, 3000, 3900 }, { 110, 130, 180, 220, 280 }, { 0.72, 0.56, 0.18, 0.07, 0.03 } },
    { PH_J,     "J",     0, 1, GLD,  36.0,  68.0, 0.68, 0.04, { 320, 2100, 2900, 3600, 4400 }, { 110, 130, 180, 220, 280 }, { 0.70, 0.58, 0.18, 0.07, 0.03 } },
    { PH_R,     "R",     0, 1, LIQ,  40.0,  76.0, 0.72, 0.08, { 300, 1300, 1800, 2600, 3500 }, { 110, 140, 190, 240, 300 }, { 0.74, 0.56, 0.20, 0.08, 0.03 } },
    { PH_L,     "L",     0, 1, LIQ,  40.0,  76.0, 0.70, 0.04, { 350, 1500, 2600, 3400, 4300 }, { 110, 130, 180, 220, 280 }, { 0.72, 0.56, 0.18, 0.07, 0.03 } },
    { PH_M,     "M",     0, 1, NSL,  42.0,  78.0, 0.72, 0.05, { 250, 1200, 2100, 2900, 3800 }, { 130, 150, 200, 240, 300 }, { 0.74, 0.54, 0.18, 0.07, 0.03 } },
    { PH_N,     "N",     0, 1, NSL,  40.0,  76.0, 0.72, 0.05, { 300, 1500, 2500, 3400, 4300 }, { 130, 150, 200, 240, 300 }, { 0.74, 0.54, 0.18, 0.07, 0.03 } },
    { PH_NY,    "NY",    0, 1, NSL,  42.0,  78.0, 0.72, 0.06, { 300, 1800, 2600, 3500, 4300 }, { 130, 150, 200, 240, 300 }, { 0.74, 0.54, 0.18, 0.07, 0.03 } },
    { PH_NG,    "NG",    0, 1, NSL,  42.0,  78.0, 0.72, 0.06, { 300, 1400, 2200, 3000, 3800 }, { 130, 150, 200, 240, 300 }, { 0.74, 0.54, 0.18, 0.07, 0.03 } },
    { PH_P,     "P",     0, 0, PLS,  42.0,  68.0, 0.62, 1.00, { 500, 1400, 2400, 3500, 4500 }, { 300, 340, 380, 420, 500 }, { 0.40, 0.28, 0.24, 0.20, 0.14 } },
    { PH_B,     "B",     0, 1, PLSV, 44.0,  70.0, 0.64, 0.36, { 500, 1400, 2400, 3500, 4500 }, { 300, 340, 380, 420, 500 }, { 0.42, 0.30, 0.24, 0.20, 0.14 } },
    { PH_T,     "T",     0, 0, PLS,  40.0,  66.0, 0.62, 1.00, { 600, 1700, 2800, 3800, 4800 }, { 280, 330, 380, 420, 500 }, { 0.40, 0.28, 0.24, 0.20, 0.14 } },
    { PH_D,     "D",     0, 1, PLSV, 42.0,  68.0, 0.64, 0.34, { 600, 1700, 2800, 3800, 4800 }, { 280, 330, 380, 420, 500 }, { 0.42, 0.30, 0.24, 0.20, 0.14 } },
    { PH_K,     "K",     0, 0, PLS,  44.0,  70.0, 0.62, 1.00, { 700, 1800, 3000, 3900, 4900 }, { 280, 330, 380, 420, 500 }, { 0.40, 0.28, 0.24, 0.20, 0.14 } },
    { PH_G,     "G",     0, 1, PLSV, 46.0,  72.0, 0.64, 0.36, { 700, 1800, 3000, 3900, 4900 }, { 280, 330, 380, 420, 500 }, { 0.42, 0.30, 0.24, 0.20, 0.14 } },
    { PH_F,     "F",     0, 0, FRC,  44.0,  82.0, 0.66, 1.00, { 900, 1800, 2900, 3900, 5000 }, { 250, 290, 340, 390, 450 }, { 0.42, 0.34, 0.28, 0.22, 0.16 } },
    { PH_V,     "V",     0, 1, FRCV, 44.0,  84.0, 0.68, 0.18, { 900, 1800, 2900, 3900, 5000 }, { 250, 290, 340, 390, 450 }, { 0.44, 0.34, 0.28, 0.22, 0.16 } },
    { PH_S,     "S",     0, 0, SIB,  48.0,  92.0, 0.60, 1.00, { 1200, 2800, 4200, 5400, 6700 }, { 230, 300, 380, 470, 560 }, { 0.24, 0.32, 0.48, 0.52, 0.34 } },
    { PH_Z,     "Z",     0, 1, SIBV, 48.0,  90.0, 0.60, 0.44, { 1200, 2800, 4200, 5400, 6700 }, { 230, 300, 380, 470, 560 }, { 0.26, 0.32, 0.46, 0.48, 0.30 } },
    { PH_SH,    "SH",    0, 0, SIB,  50.0,  94.0, 0.62, 1.00, { 1050, 2400, 3500, 4650, 5900 }, { 230, 300, 380, 470, 560 }, { 0.24, 0.42, 0.54, 0.44, 0.26 } },
    { PH_ZH,    "ZH",    0, 1, SIBV, 48.0,  92.0, 0.62, 0.44, { 1050, 2400, 3500, 4650, 5900 }, { 230, 300, 380, 470, 560 }, { 0.26, 0.40, 0.50, 0.40, 0.24 } },
    { PH_H,     "H",     0, 0, FRC,  36.0,  68.0, 0.48, 0.68, { 1000, 1700, 2600, 3500, 4400 }, { 260, 310, 360, 410, 470 }, { 0.34, 0.26, 0.20, 0.16, 0.12 } },
    { PH_TH,    "TH",    0, 0, DEN,  48.0,  90.0, 0.72, 1.00, { 900, 2000, 3800, 6000, 8000 }, { 280, 360, 460, 580, 700 }, { 0.12, 0.22, 0.32, 0.46, 0.62 } },
    { PH_DH,    "DH",    0, 1, DENV, 48.0,  90.0, 0.60, 0.44, { 950, 2200, 3300, 4400, 5600 }, { 250, 320, 400, 500, 620 }, { 0.30, 0.34, 0.34, 0.28, 0.16 } },
    { PH_CH,    "CH",    0, 0, AFR,  52.0,  92.0, 0.68, 1.00, { 1300, 2600, 3700, 4800, 5900 }, { 220, 270, 320, 370, 420 }, { 0.20, 0.36, 0.54, 0.42, 0.24 } },
    { PH_JH,    "JH",    0, 1, AFRV, 52.0,  92.0, 0.70, 0.48, { 1300, 2600, 3700, 4800, 5900 }, { 220, 270, 320, 370, 420 }, { 0.22, 0.34, 0.52, 0.40, 0.22 } },
    { PH_TS,    "TS",    0, 0, AFR,  50.0,  90.0, 0.66, 1.00, { 1200, 2500, 3600, 4700, 5800 }, { 220, 270, 320, 370, 420 }, { 0.46, 0.38, 0.30, 0.24, 0.18 } },
    { PH_DZ,    "DZ",    0, 1, AFRV, 50.0,  90.0, 0.68, 0.48, { 1200, 2500, 3600, 4700, 5800 }, { 220, 270, 320, 370, 420 }, { 0.48, 0.38, 0.30, 0.24, 0.18 } }
};

#undef VOW
#undef NVW
#undef GLD
#undef LIQ
#undef NSL
#undef PLS
#undef PLSV
#undef FRC
#undef FRCV
#undef SIB
#undef SIBV
#undef DEN
#undef DENV
#undef AFR
#undef AFRV

const phoneme_def_t *say_get_phoneme(phoneme_id_t id)
{
    if ((int) id < 0 || id >= PH_COUNT) {
        return &g_phonemes[PH_PAUSE];
    }
    return &g_phonemes[id];
}

int say_phone_has_features(phoneme_id_t id, say_features_t mask)
{
    return (say_get_phoneme(id)->features & mask) == mask;
}

/* ---------------------------------------------------------------------------
 * Feature-mask predicates. Each one is a one-line query against the central
 * feature table, replacing what used to be 12 hand-written enum lists.
 * ------------------------------------------------------------------------- */

int say_is_vowel_phone(phoneme_id_t id)              { return say_phone_has_features(id, F_VOWEL); }
int say_is_glide_phone(phoneme_id_t id)              { return say_phone_has_features(id, F_GLIDE); }
int say_is_plosive_phone(phoneme_id_t id)            { return say_phone_has_features(id, F_PLOS); }
int say_is_fricative_phone(phoneme_id_t id)          { return say_phone_has_features(id, F_FRIC); }
int say_is_affricate_phone(phoneme_id_t id)          { return say_phone_has_features(id, F_AFFRIC); }
int say_is_sonorant_phone(phoneme_id_t id)           { return say_phone_has_features(id, F_SONOR); }
int say_is_nasal_vowel_phone(phoneme_id_t id)        { return say_phone_has_features(id, F_NASAL_VOWEL); }
int say_is_sibilant_phone(phoneme_id_t id)           { return say_phone_has_features(id, F_SIBILANT); }
int say_is_dental_fricative_phone(phoneme_id_t id)   { return say_phone_has_features(id, F_DENTAL); }
int say_is_voiced_fricative_phone(phoneme_id_t id)   { return say_phone_has_features(id, F_FRIC | F_VOICED); }
int say_is_voiced_plosive_phone(phoneme_id_t id)     { return say_phone_has_features(id, F_PLOS | F_VOICED); }

int say_is_voiceless_plosive_phone(phoneme_id_t id)
{
    say_features_t f = say_get_phoneme(id)->features;
    return (f & F_PLOS) && !(f & F_VOICED);
}

const phoneme_def_t *say_lookup_symbol(const char *symbol)
{
    size_t i;

    for (i = 0; i < PH_COUNT; ++i) {
        if (say_equals_icase(g_phonemes[i].symbol, symbol)) {
            return &g_phonemes[i];
        }
    }
    return NULL;
}

/* ---------------------------------------------------------------------------
 * English dedicated noise path config.
 *
 * The infrastructure is preserved from earlier experiments (HP+LP flat-spectrum
 * noise routed parallel to the formant filter bank) but currently disabled:
 * `noise_path_mix=1.0` produces an acoustically cleaner spectrum, but every
 * extractor we have is calibrated against formant-bank noise, so flat-spectrum
 * paths get classified as /S/ and trash the regression suite. Returning a zero
 * struct keeps the synthesis path active without changing audio output.
 * ------------------------------------------------------------------------- */

phoneme_noise_path_t say_get_noise_path_en(phoneme_id_t id)
{
    phoneme_noise_path_t p;
    memset(&p, 0, sizeof(p));
    (void) id;
    return p;
}
