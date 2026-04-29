#define _CRT_SECURE_NO_WARNINGS

#include "say_internal.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * say_text.c — UTF-8 normalization, lexicon / pattern phonemizers (en+fr),
 * grow-on-demand buffers shared across the pipeline.
 * ------------------------------------------------------------------------- */

/* === say.c lines 162..184 === */
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

/* === say.c lines 323..614 === */
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

/* === say.c lines 652..677 === */
int say_is_boundary_char(char c)
{
    return c == '.' || c == ',' || c == '!' || c == '?' || c == ';' || c == ':';
}

int say_is_word_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

int say_is_token_char(char c)
{
    return say_is_word_char(c) || c == '\'' || c == '-';
}

int say_equals_icase(const char *a, const char *b)
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

/* === say.c lines 679..680 === */
static int say_match_at(const char *word, size_t index, const char *pattern);
int say_append_text_word(const char *word, say_language_t language, segment_buffer_t *segments);

/* === say.c lines 682..847 === */
int say_segment_buffer_reserve(segment_buffer_t *buffer, size_t extra)
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

int say_segment_buffer_push(
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
    segment->diphthong_target = 0;
    return 1;
}

int say_segment_buffer_append(segment_buffer_t *buffer, const segment_buffer_t *extra)
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

int say_frame_buffer_reserve(frame_buffer_t *buffer, size_t extra)
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

int say_frame_buffer_push(frame_buffer_t *buffer, const frame_t *frame)
{
    if (!say_frame_buffer_reserve(buffer, 1)) {
        return 0;
    }
    buffer->data[buffer->count++] = *frame;
    return 1;
}

int say_text_buffer_reserve(text_buffer_t *buffer, size_t extra)
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

int say_text_buffer_append(text_buffer_t *buffer, const char *text)
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

int say_text_buffer_appendf(text_buffer_t *buffer, const char *fmt, ...)
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

/* === say.c lines 849..1005 === */
size_t say_utf8_decode(const unsigned char *text, unsigned int *codepoint)
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

char *say_normalize_text(const char *input, char *error, size_t error_size)
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

/* === say.c lines 1007..2239 === */
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

int say_append_text_word(const char *word, say_language_t language, segment_buffer_t *segments)
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

const char *say_digit_word(char digit, say_language_t language)
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

int say_phonemize_text(
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


/* === say.c lines 2252..2322 === */
int say_parse_phoneme_input(
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
