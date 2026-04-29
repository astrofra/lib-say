#define _CRT_SECURE_NO_WARNINGS

#include "say_internal.h"

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * say_text_en_nrl.c — D1 — port of the reference` NRL letter-
 * to-sound rule engine.
 *
 * Rules and feature matrix originate from the matcher.
 * C transliteration of TLTranslate; the rule database is generated verbatim
 * by build/gen_nrl.py from the M68k DC.B literals (see say_nrl_rules.inc).
 *
 * Each rule has the form
 *
 *     LEFT [CENTER] RIGHT = REPLACEMENT <delim>
 *
 * where <delim> is '\\' (apply default accent post-processor) or '`' (rule is
 * an exception — don't accent). LEFT and RIGHT may contain literal letters,
 * the boundary character ' ', or one of these special symbols:
 *
 *   #  single vowel        *  1+ consonants     :  0+ consonants
 *   .  voiced consonant    &  sibilant          ^  single consonant
 *   $  cons + I/E          @  cons + U          +  front vowel
 *   %  suffix (E/ES/ED/    ?  digit             _  0+ digits
 *      ER/ELY/ING/ERS/INGS)
 *
 * The CENTER section accepts only literal characters.
 *
 * The replacement is ARPAbet-ish phoneme tokens with stress digits 1..9
 * scattered after vowels. say_nrl_emit_phonemes parses that into lib-say
 * `PH_*` segments.
 * ------------------------------------------------------------------------- */

/* ---- Letter feature matrix (12 bits per ASCII char), ported verbatim from
 * translator.asm `FEATURES` table. ----------------------------------------- */

#define NF_SYMBOL   0x001
#define NF_DIGIT    0x002
#define NF_UAFF     0x004
#define NF_VOICED   0x008
#define NF_SIBILANT 0x010
#define NF_CONS     0x020
#define NF_VOWEL    0x040
#define NF_LETTER   0x080
#define NF_FRONT    0x100
#define NF_IGNORE   0x200
#define NF_KEYWORD  0x400
#define NF_WORDBRK  0x800

static const uint16_t g_letter_features[128] = {
    /* 0x00..0x1F control codes */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* 0x20 ' ' */ NF_SYMBOL | NF_KEYWORD | NF_WORDBRK,
    /* 0x21 '!' */ NF_SYMBOL | NF_WORDBRK,
    /* 0x22 '"' */ NF_SYMBOL,
    /* 0x23 '#' */ NF_SYMBOL | NF_KEYWORD | NF_WORDBRK,
    /* 0x24 '$' */ NF_SYMBOL | NF_KEYWORD,
    /* 0x25 '%' */ NF_SYMBOL | NF_KEYWORD,
    /* 0x26 '&' */ NF_SYMBOL | NF_KEYWORD,
    /* 0x27 '\''*/ NF_SYMBOL,
    /* 0x28 '(' */ NF_SYMBOL | NF_WORDBRK,
    /* 0x29 ')' */ NF_SYMBOL | NF_WORDBRK,
    /* 0x2A '*' */ NF_SYMBOL | NF_KEYWORD,
    /* 0x2B '+' */ NF_SYMBOL | NF_KEYWORD,
    /* 0x2C ',' */ NF_SYMBOL | NF_WORDBRK,
    /* 0x2D '-' */ NF_SYMBOL | NF_WORDBRK,
    /* 0x2E '.' */ NF_SYMBOL | NF_KEYWORD | NF_WORDBRK,
    /* 0x2F '/' */ NF_SYMBOL,
    /* 0x30..0x39 digits */
    NF_DIGIT, NF_DIGIT, NF_DIGIT, NF_DIGIT, NF_DIGIT,
    NF_DIGIT, NF_DIGIT, NF_DIGIT, NF_DIGIT, NF_DIGIT,
    /* 0x3A ':' */ NF_SYMBOL | NF_KEYWORD,
    /* 0x3B ';' */ NF_SYMBOL,
    /* 0x3C '<' */ NF_SYMBOL,
    /* 0x3D '=' */ NF_SYMBOL,
    /* 0x3E '>' */ NF_SYMBOL,
    /* 0x3F '?' */ NF_SYMBOL | NF_KEYWORD | NF_WORDBRK,
    /* 0x40 '@' */ NF_SYMBOL | NF_KEYWORD,
    /* 0x41 'A' */ NF_LETTER | NF_VOWEL,
    /* 0x42 'B' */ NF_LETTER | NF_CONS | NF_VOICED,
    /* 0x43 'C' */ NF_LETTER | NF_CONS | NF_SIBILANT,
    /* 0x44 'D' */ NF_LETTER | NF_CONS | NF_VOICED | NF_UAFF,
    /* 0x45 'E' */ NF_LETTER | NF_VOWEL | NF_FRONT,
    /* 0x46 'F' */ NF_LETTER | NF_CONS,
    /* 0x47 'G' */ NF_LETTER | NF_CONS | NF_VOICED | NF_SIBILANT,
    /* 0x48 'H' */ NF_LETTER | NF_CONS,
    /* 0x49 'I' */ NF_LETTER | NF_VOWEL | NF_FRONT,
    /* 0x4A 'J' */ NF_LETTER | NF_CONS | NF_VOICED | NF_SIBILANT | NF_UAFF,
    /* 0x4B 'K' */ NF_LETTER | NF_CONS,
    /* 0x4C 'L' */ NF_LETTER | NF_CONS | NF_VOICED | NF_UAFF,
    /* 0x4D 'M' */ NF_LETTER | NF_CONS | NF_VOICED,
    /* 0x4E 'N' */ NF_LETTER | NF_CONS | NF_VOICED | NF_UAFF,
    /* 0x4F 'O' */ NF_LETTER | NF_VOWEL,
    /* 0x50 'P' */ NF_LETTER | NF_CONS,
    /* 0x51 'Q' */ NF_LETTER | NF_CONS,
    /* 0x52 'R' */ NF_LETTER | NF_CONS | NF_VOICED | NF_UAFF,
    /* 0x53 'S' */ NF_LETTER | NF_CONS | NF_SIBILANT | NF_UAFF,
    /* 0x54 'T' */ NF_LETTER | NF_CONS | NF_UAFF,
    /* 0x55 'U' */ NF_LETTER | NF_VOWEL,
    /* 0x56 'V' */ NF_LETTER | NF_CONS | NF_VOICED,
    /* 0x57 'W' */ NF_LETTER | NF_CONS | NF_VOICED,
    /* 0x58 'X' */ NF_LETTER | NF_CONS | NF_SIBILANT,
    /* 0x59 'Y' */ NF_LETTER | NF_VOWEL | NF_FRONT,
    /* 0x5A 'Z' */ NF_LETTER | NF_CONS | NF_VOICED | NF_SIBILANT | NF_UAFF,
    /* 0x5B '[' */ NF_SYMBOL,
    /* 0x5C '\\' */ NF_SYMBOL,
    /* 0x5D ']' */ NF_SYMBOL,
    /* 0x5E '^' */ NF_SYMBOL | NF_KEYWORD,
    /* 0x5F '_' */ NF_SYMBOL | NF_KEYWORD,
    /* 0x60 '`' */ NF_SYMBOL,
    /* 0x61..0x7A lowercase — never reached because PREPROC uppercases */
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* 0x7B '{' */ NF_SYMBOL,
    /* 0x7C '|' */ NF_SYMBOL,
    /* 0x7D '}' */ NF_SYMBOL,
    /* 0x7E '~' */ NF_SYMBOL,
    /* 0x7F DEL */ 0
};

static uint16_t nrl_features(unsigned char c)
{
    if (c >= 128) return 0;
    return g_letter_features[c];
}

#include "say_nrl_rules.inc"

/* ---- Symbol handlers ----------------------------------------------------- */

/* Word buffer wraps the input with leading/trailing spaces so that the matcher
 * can probe context past word boundaries without bounds checking. `pos` is the
 * 0-indexed offset into `buf` of the first letter of the CENTER context as the
 * symbol handlers run; left handlers walk backwards by predecrementing, right
 * handlers walk forward by postincrementing. */
typedef struct {
    char buf[140];   /* MAXLTRS=100 in the reference; we cap at 138 + 2 spaces */
    int len;
    int pos;
} nrl_word_t;

/* Direction +1 means walking left (predecrement), -1 means walking right
 * (postincrement). Matches the reference's D7 convention. */
static int nrl_fetch(nrl_word_t *w, int dir, char *out_ch, uint16_t *out_feat)
{
    int p;
    if (dir > 0) {
        p = --w->pos;
    } else {
        p = w->pos++;
    }
    if (p < 0 || p >= w->len) {
        *out_ch = ' ';
        *out_feat = nrl_features((unsigned char) ' ');
        return 0;
    }
    *out_ch = w->buf[p];
    *out_feat = nrl_features((unsigned char) *out_ch);
    return 1;
}

/* Symbol handlers — return non-zero if the rule fragment matches. */

static int nrl_svowel(nrl_word_t *w, int dir)   /* '#' single vowel */
{
    char c; uint16_t f;
    nrl_fetch(w, dir, &c, &f);
    return (f & NF_VOWEL) != 0;
}

static int nrl_omcons(nrl_word_t *w, int dir)   /* '*' 1+ consonants */
{
    char c; uint16_t f;
    nrl_fetch(w, dir, &c, &f);
    if (!(f & NF_CONS)) return 0;
    do { nrl_fetch(w, dir, &c, &f); } while (f & NF_CONS);
    /* back up one — we over-fetched */
    if (dir > 0) ++w->pos; else --w->pos;
    return 1;
}

static int nrl_svcons(nrl_word_t *w, int dir)   /* '.' voiced consonant */
{
    char c; uint16_t f;
    nrl_fetch(w, dir, &c, &f);
    return (f & NF_VOICED) && (f & NF_CONS);
}

static int nrl_sconsie(nrl_word_t *w, int dir)  /* '$' cons + I/E (right ctx) */
{
    char c1, c2; uint16_t f1, f2;
    nrl_fetch(w, dir, &c1, &f1);
    if (!(f1 & NF_CONS)) return 0;
    nrl_fetch(w, dir, &c2, &f2);
    return (c2 == 'I' || c2 == 'E');
}

static int nrl_suffixes(nrl_word_t *w, int dir) /* '%' E/ES/ED/ER/ELY/ING/ERS/INGS */
{
    char c; uint16_t f;
    nrl_fetch(w, dir, &c, &f);
    if (c == 'E') {
        nrl_fetch(w, dir, &c, &f);
        if (!(f & NF_LETTER)) return 1;            /* E# */
        if (c == 'S' || c == 'D') {                /* ES, ED */
            nrl_fetch(w, dir, &c, &f);
            return !(f & NF_LETTER);
        }
        if (c == 'R') {                            /* ER, ERS */
            nrl_fetch(w, dir, &c, &f);
            if (!(f & NF_LETTER)) return 1;
            if (c != 'S') return 0;
            nrl_fetch(w, dir, &c, &f);
            return !(f & NF_LETTER);
        }
        if (c == 'L') {                            /* ELY */
            nrl_fetch(w, dir, &c, &f);
            if (c != 'Y') return 0;
            nrl_fetch(w, dir, &c, &f);
            return !(f & NF_LETTER);
        }
        return 0;
    }
    if (c == 'I') {                                /* ING / INGS */
        nrl_fetch(w, dir, &c, &f);
        if (c != 'N') return 0;
        nrl_fetch(w, dir, &c, &f);
        if (c != 'G') return 0;
        nrl_fetch(w, dir, &c, &f);
        if (!(f & NF_LETTER)) return 1;
        if (c != 'S') return 0;
        nrl_fetch(w, dir, &c, &f);
        return !(f & NF_LETTER);
    }
    return 0;
}

static int nrl_sibil(nrl_word_t *w, int dir)    /* '&' sibilant */
{
    char c; uint16_t f;
    nrl_fetch(w, dir, &c, &f);
    return (f & NF_SIBILANT) != 0;
}

static int nrl_cuaff(nrl_word_t *w, int dir)    /* '@' cons + long-U affectation */
{
    char c; uint16_t f;
    nrl_fetch(w, dir, &c, &f);
    return (f & NF_UAFF) != 0;
}

static int nrl_scons(nrl_word_t *w, int dir)    /* '^' single consonant */
{
    char c; uint16_t f;
    nrl_fetch(w, dir, &c, &f);
    return (f & NF_CONS) != 0;
}

static int nrl_fvowel(nrl_word_t *w, int dir)   /* '+' front vowel */
{
    char c; uint16_t f;
    nrl_fetch(w, dir, &c, &f);
    return (f & NF_FRONT) != 0;
}

static int nrl_zmcons(nrl_word_t *w, int dir)   /* ':' 0+ consonants */
{
    char c; uint16_t f;
    nrl_fetch(w, dir, &c, &f);
    while (f & NF_CONS) {
        nrl_fetch(w, dir, &c, &f);
    }
    /* back up — Reference adds D7 to A2 to compensate */
    if (dir > 0) ++w->pos; else --w->pos;
    return 1;
}

static int nrl_snum(nrl_word_t *w, int dir)     /* '?' single digit */
{
    char c; uint16_t f;
    nrl_fetch(w, dir, &c, &f);
    return (f & NF_DIGIT) != 0;
}

static int nrl_zmnums(nrl_word_t *w, int dir)   /* '_' 0+ digits */
{
    char c; uint16_t f;
    nrl_fetch(w, dir, &c, &f);
    while (f & NF_DIGIT) {
        nrl_fetch(w, dir, &c, &f);
    }
    if (dir > 0) ++w->pos; else --w->pos;
    return 1;
}

static int nrl_nonltr(nrl_word_t *w, int dir)   /* ' ' (boundary) — non-letter */
{
    char c; uint16_t f;
    nrl_fetch(w, dir, &c, &f);
    return !(f & NF_LETTER);
}

typedef int (*nrl_symbol_fn)(nrl_word_t *, int);

static nrl_symbol_fn nrl_symbol_handler(char c)
{
    switch (c) {
        case '#': return nrl_svowel;
        case '*': return nrl_omcons;
        case '.': return nrl_svcons;
        case '$': return nrl_sconsie;
        case '%': return nrl_suffixes;
        case '&': return nrl_sibil;
        case '@': return nrl_cuaff;
        case '^': return nrl_scons;
        case '+': return nrl_fvowel;
        case ':': return nrl_zmcons;
        case '?': return nrl_snum;
        case '_': return nrl_zmnums;
        case ' ': return nrl_nonltr;
        default:  return NULL;
    }
}

/* ---- Rule matcher ------------------------------------------------------- */

/* Try to match one rule (starting at *rule_p) against the word at center_pos.
 * On success: returns 1, sets *replacement to the start of the replacement,
 * *replacement_len to its length, and advances *rule_p past the rule. Also
 * sets *new_pos to the byte offset just past the matched CENTER and *is_excp
 * to non-zero if the rule terminator was '`'.
 * On failure: returns 0 and advances *rule_p past the rule's terminator. */
static int nrl_try_rule(
    const char **rule_p,
    nrl_word_t *word,
    int center_pos,
    const char **replacement,
    int *replacement_len,
    int *new_pos,
    int *is_excp
)
{
    const char *rule = *rule_p;
    int center_start, right_start, repl_start, repl_len;
    int word_pos;
    int i;
    char terminator;

    /* Locate '[' (start of center). */
    center_start = -1;
    for (i = 0; rule[i] != '\0'; ++i) {
        if (rule[i] == '[') { center_start = i + 1; break; }
        if (rule[i] == '\\' || rule[i] == '`') {
            *rule_p = rule + i + 1;
            return 0;
        }
    }
    if (center_start < 0) {
        *rule_p = rule + i;
        return 0;
    }

    /* Match CENTER: literal char comparison. */
    word_pos = center_pos;
    for (i = center_start;; ++i) {
        char c = rule[i];
        if (c == ']') break;
        if (c == '\0' || c == '\\' || c == '`') {
            /* malformed rule */
            *rule_p = rule + i + (c == '\0' ? 0 : 1);
            return 0;
        }
        if (word_pos >= word->len || word->buf[word_pos] != c) {
            /* center mismatch — skip to end of rule and move on */
            while (rule[i] != '\\' && rule[i] != '`' && rule[i] != '\0') ++i;
            *rule_p = (rule[i] == '\0') ? rule + i : rule + i + 1;
            return 0;
        }
        ++word_pos;
    }
    /* `i` now points at ']'. Right context starts after that. */
    right_start = i + 1;

    /* Match LEFT context: walk rule[center_start-2 .. 0] (i.e. chars before
     * '[') from right to left, and word from center_pos-1 leftward. */
    word->pos = center_pos;
    for (i = center_start - 2; i >= 0; --i) {
        char c = rule[i];
        uint16_t f = nrl_features((unsigned char) c);
        if (f & NF_KEYWORD) {
            nrl_symbol_fn h = nrl_symbol_handler(c);
            if (!h || !h(word, +1)) goto fail;
        } else {
            char wc;
            uint16_t wf;
            nrl_fetch(word, +1, &wc, &wf);
            if (wc != c) goto fail;
        }
    }

    /* Match RIGHT context: walk rule[right_start..] until '=', word from
     * word_pos rightward. */
    word->pos = word_pos;
    for (i = right_start;; ++i) {
        char c = rule[i];
        uint16_t f;
        if (c == '=') break;
        if (c == '\0' || c == '\\' || c == '`') goto fail;
        f = nrl_features((unsigned char) c);
        if (f & NF_KEYWORD) {
            nrl_symbol_fn h = nrl_symbol_handler(c);
            if (!h || !h(word, -1)) goto fail;
        } else {
            char wc;
            uint16_t wf;
            nrl_fetch(word, -1, &wc, &wf);
            if (wc != c) goto fail;
        }
    }

    /* Replacement: from after '=' until '\\' or '`' (exclusive). */
    repl_start = i + 1;
    for (i = repl_start;; ++i) {
        if (rule[i] == '\\' || rule[i] == '`' || rule[i] == '\0') break;
    }
    repl_len = i - repl_start;
    terminator = rule[i];
    *replacement = rule + repl_start;
    *replacement_len = repl_len;
    *new_pos = word_pos;
    *is_excp = (terminator == '`');
    *rule_p = (terminator == '\0') ? rule + i : rule + i + 1;
    return 1;

fail:
    /* Skip to end-of-rule and advance. */
    for (i = right_start;; ++i) {
        if (rule[i] == '\\' || rule[i] == '`' || rule[i] == '\0') break;
    }
    *rule_p = (rule[i] == '\0') ? rule + i : rule + i + 1;
    return 0;
}

/* Pick the rule group for the character at center_pos. Maps:
 *   A..Z → 0..25,
 *   digit → 26 (DIGRULES),
 *   anything else (punctuation, space) → 27 (PUNRULES). */
static const char *nrl_rule_group_for(char c)
{
    uint16_t f = nrl_features((unsigned char) c);
    int idx;
    if (f & NF_LETTER)      idx = (c - 'A');
    else if (f & NF_DIGIT)  idx = 26;
    else                    idx = 27;
    return g_nrl_rule_groups[idx];
}

/* Run rules until one matches at center_pos. Returns 1 on success and fills
 * the replacement output; returns 0 if no rule matched (shouldn't happen for
 * the bundled English rule set — every letter group has a catch-all). */
static int nrl_match_at(
    nrl_word_t *word,
    int center_pos,
    const char **replacement,
    int *replacement_len,
    int *new_pos,
    int *is_excp
)
{
    const char *rule = nrl_rule_group_for(word->buf[center_pos]);
    while (*rule != '\0') {
        int matched = nrl_try_rule(&rule, word, center_pos, replacement,
                                   replacement_len, new_pos, is_excp);
        if (matched) return 1;
    }
    return 0;
}

/* ---- ARPAbet replacement → segment_t emitter --------------------------- */

/* Two-character ARPAbet tokens we recognize. Order matters: longest match
 * tried first (3-char) then 2-char. The mapping below reflects the reference's
 * narrator convention: "J" is /dʒ/ (lib-say PH_JH), "Y" is /j/ (lib-say PH_J).
 *
 * Multi-segment expansions (diphthongs, syllabic consonants) emit two PH_*
 * codes. Stress digit 1..9 attaches to the most recently emitted vowel; 4 or
 * higher is treated as primary stress (lib-say segment.stress = 2). */

typedef struct {
    const char *token;
    phoneme_id_t a;
    phoneme_id_t b;       /* PH_PAUSE = none */
} nrl_arpa_t;

/* 3-char tokens checked first. */
static const nrl_arpa_t g_arpa3[] = {
    { "AXP", PH_SCHWA, PH_PAUSE },
    { "AXR", PH_R,     PH_PAUSE },
    { "IXR", PH_IH,    PH_R     },
    { "OXR", PH_OH,    PH_R     },
    { "UXR", PH_U,     PH_R     },
    { "EXR", PH_EH,    PH_R     },
};

/* 2-char tokens. */
static const nrl_arpa_t g_arpa2[] = {
    { "AA", PH_A,     PH_PAUSE },
    { "AE", PH_AE,    PH_PAUSE },
    { "AH", PH_AH,    PH_PAUSE },
    { "AO", PH_OH,    PH_PAUSE },
    { "AW", PH_AH,    PH_W     },
    { "AX", PH_SCHWA, PH_PAUSE },
    { "AY", PH_AH,    PH_J     },
    { "CH", PH_CH,    PH_PAUSE },
    { "DH", PH_DH,    PH_PAUSE },
    { "DX", PH_D,     PH_PAUSE },
    { "EH", PH_EH,    PH_PAUSE },
    { "EL", PH_SCHWA, PH_L     },
    { "EM", PH_SCHWA, PH_M     },
    { "EN", PH_SCHWA, PH_N     },
    { "ER", PH_R,     PH_PAUSE },
    { "EY", PH_E,     PH_J     },
    { "GX", PH_G,     PH_PAUSE },
    { "HH", PH_H,     PH_PAUSE },
    { "HX", PH_H,     PH_PAUSE },
    { "IH", PH_IH,    PH_PAUSE },
    { "IL", PH_IH,    PH_L     },
    { "IM", PH_IH,    PH_M     },
    { "IN", PH_IH,    PH_N     },
    { "IX", PH_IH,    PH_PAUSE },
    { "IY", PH_I,     PH_PAUSE },
    { "JH", PH_JH,    PH_PAUSE },
    { "KX", PH_K,     PH_PAUSE },
    { "LX", PH_L,     PH_PAUSE },
    { "NG", PH_NG,    PH_PAUSE },
    { "NX", PH_NG,    PH_PAUSE },
    /* Reference's OH digraph is the long /ɔː/ monophthong (e.g. "or" /ɔːr/), distinct
     * from OW which is the /oʊ/ diphthong. Mapped to lib-say PH_OH (the lower
     * /ɔ/-like vowel; PH_O is closer to /o/). */
    { "OH", PH_OH,    PH_PAUSE },
    { "OW", PH_OH,    PH_W     },
    { "OY", PH_OH,    PH_J     },
    { "QX", PH_PAUSE, PH_PAUSE },
    { "RR", PH_R,     PH_PAUSE },
    { "RX", PH_R,     PH_PAUSE },
    { "SH", PH_SH,    PH_PAUSE },
    { "TH", PH_TH,    PH_PAUSE },
    { "TQ", PH_T,     PH_PAUSE },
    { "UH", PH_U,     PH_PAUSE },
    /* The Reference's UL / UM / UN digraphs are syllabic consonants (/l̩/, /m̩/, /n̩/),
     * not U+L/M/N. Map them to schwa + consonant — the closest approximation in
     * lib-say's inventory. */
    { "UL", PH_SCHWA, PH_L     },
    { "UM", PH_SCHWA, PH_M     },
    { "UN", PH_SCHWA, PH_N     },
    { "UW", PH_U,     PH_PAUSE },
    { "UX", PH_SCHWA, PH_PAUSE },  /* Reference UX (id 20) — schwa-ish reduced vowel */
    { "WH", PH_W,     PH_PAUSE },
    { "YU", PH_J,     PH_U     },
    { "ZH", PH_ZH,    PH_PAUSE },
};

/* 1-char tokens. */
static phoneme_id_t nrl_one_char(char c)
{
    switch (c) {
        case 'A': return PH_A;
        case 'B': return PH_B;
        case 'D': return PH_D;
        case 'E': return PH_EH;
        case 'F': return PH_F;
        case 'G': return PH_G;
        case 'H': return PH_H;
        case 'I': return PH_IH;
        case 'J': return PH_JH;
        case 'K': return PH_K;
        case 'L': return PH_L;
        case 'M': return PH_M;
        case 'N': return PH_N;
        case 'O': return PH_OH;
        case 'P': return PH_P;
        case 'Q': return PH_PAUSE;       /* glottal stop → silent */
        case 'R': return PH_R;
        case 'S': return PH_S;
        case 'T': return PH_T;
        case 'U': return PH_U;
        case 'V': return PH_V;
        case 'W': return PH_W;
        case 'Y': return PH_J;
        case 'Z': return PH_Z;
        default:  return PH_PAUSE;
    }
}

/* Emit phonemes for a string of ARPAbet-ish tokens to the segment buffer. The
 * stress digit (1..9) attaches to the most recently emitted vowel; >= 4 maps
 * to lib-say primary stress (segment.stress = 2). Returns 1 on success, 0 on
 * out-of-memory. The first emitted segment in `start` gets word_start = 1
 * and the last gets word_end = 1 (set by the caller, not here). */
static int nrl_emit_phonemes(
    const char *repl,
    int repl_len,
    segment_buffer_t *segments
)
{
    int p = 0;
    size_t last_vowel = (size_t) -1;
    while (p < repl_len) {
        char c = repl[p];
        size_t i;
        phoneme_id_t a = PH_PAUSE, b = PH_PAUSE;
        int matched = 0;

        if (c == ' ') { ++p; continue; }
        if (c >= '1' && c <= '9') {
            /* Reference rule digits are relative stress levels (1..9) within a word.
             * The rule writer's intent is "this vowel is stressed"; lib-say only
             * has a binary stress field, so any digit ≥ 1 marks the most-recent
             * vowel as primary-stressed. Words ending with no digit at all are
             * handled by the accenter post-processor below. */
            if (last_vowel != (size_t) -1) {
                segments->data[last_vowel].stress = 2;
            }
            ++p;
            continue;
        }
        if (c == '.' || c == '!' || c == '?' || c == ',' || c == '-') {
            /* Punctuation embedded in replacement — emit a small pause. */
            int boundary = (c == '.' || c == '!' || c == '?') ? 2 : 1;
            if (!say_segment_buffer_push(segments, PH_PAUSE, 1.0, boundary)) {
                return 0;
            }
            ++p;
            continue;
        }

        /* 3-char token? */
        if (p + 2 < repl_len) {
            for (i = 0; i < SAY_ARRAY_COUNT(g_arpa3); ++i) {
                if (repl[p] == g_arpa3[i].token[0] &&
                    repl[p+1] == g_arpa3[i].token[1] &&
                    repl[p+2] == g_arpa3[i].token[2]) {
                    a = g_arpa3[i].a;
                    b = g_arpa3[i].b;
                    p += 3;
                    matched = 1;
                    break;
                }
            }
        }
        /* 2-char token? */
        if (!matched && p + 1 < repl_len) {
            for (i = 0; i < SAY_ARRAY_COUNT(g_arpa2); ++i) {
                if (repl[p] == g_arpa2[i].token[0] &&
                    repl[p+1] == g_arpa2[i].token[1]) {
                    a = g_arpa2[i].a;
                    b = g_arpa2[i].b;
                    p += 2;
                    matched = 1;
                    break;
                }
            }
        }
        /* 1-char fallback. */
        if (!matched) {
            a = nrl_one_char(c);
            ++p;
        }

        if (a != PH_PAUSE || b != PH_PAUSE) {
            if (a != PH_PAUSE) {
                if (!say_segment_buffer_push(segments, a, 1.0, 0)) return 0;
                if (say_get_phoneme(a)->is_vowel) last_vowel = segments->count - 1;
            }
            if (b != PH_PAUSE) {
                if (!say_segment_buffer_push(segments, b, 1.0, 0)) return 0;
                if (say_get_phoneme(b)->is_vowel) last_vowel = segments->count - 1;
            }
        }
    }
    return 1;
}

/* Run the NRL engine over `word` and append the resulting phoneme segments to
 * `segments`. Returns 1 on success, 0 on internal failure (out of memory or
 * over-long word). The caller already removed apostrophes / hyphens from the
 * word; ASCII-only uppercase or digit characters are expected. word_start and
 * word_end markers + accenter post-processing are applied after this returns. */
int say_phonemize_english_nrl(const char *word, segment_buffer_t *segments)
{
    nrl_word_t w;
    int wlen = (int) strlen(word);
    int i;
    int p = 1;                /* center starts after the leading boundary */
    size_t segs_at_start = segments->count;
    int has_vowel_with_stress = 0;
    size_t first_vowel_seg = (size_t) -1;

    if (wlen == 0 || wlen > (int) sizeof(w.buf) - 4) {
        return 0;
    }

    /* Wrap the word with leading and trailing space — gives the matcher a
     * uniform boundary to test against (' ' is the NF_WORDBRK keyword). */
    w.buf[0] = ' ';
    for (i = 0; i < wlen; ++i) {
        char c = word[i];
        if (c >= 'a' && c <= 'z') c = (char) (c - 'a' + 'A');
        w.buf[1 + i] = c;
    }
    w.buf[1 + wlen] = ' ';
    w.buf[2 + wlen] = '\0';
    w.len = wlen + 2;
    w.pos = 0;

    while (p < w.len - 1) {  /* don't process the trailing boundary space */
        const char *repl;
        int repl_len, new_pos, is_excp;

        if (!nrl_match_at(&w, p, &repl, &repl_len, &new_pos, &is_excp)) {
            /* No rule matched — should not happen. Skip a char. */
            ++p;
            continue;
        }
        if (repl_len > 0) {
            if (!nrl_emit_phonemes(repl, repl_len, segments)) {
                return 0;
            }
        }
        if (new_pos == p) {
            /* defensive: empty match → advance to avoid infinite loop */
            ++p;
        } else {
            p = new_pos;
        }
        (void) is_excp;
    }

    /* Accenter post-processor: if no vowel in this word got a primary-stress
     * digit (>= 4), promote the FIRST vowel to primary stress. Mirrors
     * translator.asm's ACCENTER. */
    for (i = (int) segs_at_start; i < (int) segments->count; ++i) {
        if (say_get_phoneme(segments->data[i].phoneme)->is_vowel) {
            if (first_vowel_seg == (size_t) -1) first_vowel_seg = (size_t) i;
            if (segments->data[i].stress >= 2) has_vowel_with_stress = 1;
        }
    }
    if (!has_vowel_with_stress && first_vowel_seg != (size_t) -1) {
        segments->data[first_vowel_seg].stress = 2;
    }
    return segments->count > segs_at_start;
}
