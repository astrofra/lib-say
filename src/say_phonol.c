#define _CRT_SECURE_NO_WARNINGS

#include "say_internal.h"

#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * say_phonol.c — C2 — feature-driven phonological rule pass.
 *
 * Sits between text → segments and frame generation, applying allophonic
 * substitutions / insertions / deletions that depend on neighbouring context.
 * Modelled on the reference's PHONOL.ASM + phonrules.i, but with a smaller
 * rule set tuned to lib-say's phoneme inventory.
 *
 * Each rule has three context slots — LEFT, CENTER, RIGHT — and one action.
 * A slot matcher can be:
 *   - PHONOL_ANY: matches any non-pause segment, or end-of-stream sentinel
 *   - PHONOL_ID:  matches a specific phoneme id (PH_PAUSE for boundary)
 *   - PHONOL_FEATURES: matches when (features & required) == required AND
 *                     (features & forbidden) == 0
 *
 * Actions:
 *   - PHONOL_DELETE:       remove the center segment
 *   - PHONOL_REPLACE:      replace center.phoneme with `arg`
 *   - PHONOL_INSERT_AFTER: keep center, insert a new segment after with
 *                         phoneme = arg. The new segment inherits the center's
 *                         word_end / boundary_type / weak_word and resets
 *                         duration_scale / accent_n / stress / diphthong_target.
 *
 * Rules fire single-pass, left to right. Order matters: earlier rules in the
 * table are tried first. After a delete or replace, the loop re-tests at the
 * same index (so the new center can be matched by other rules); after an
 * insert, the loop skips past the inserted segment.
 *
 * Adding a new rule is a single struct literal in the table below.
 * ------------------------------------------------------------------------- */

#define PHONOL_ANY        0
#define PHONOL_ID         1
#define PHONOL_FEATURES   2

typedef struct phonol_match_t {
    int            kind;
    phoneme_id_t   phoneme;
    say_features_t require;
    say_features_t forbid;
} phonol_match_t;

#define PHONOL_DELETE        0
#define PHONOL_REPLACE       1
#define PHONOL_INSERT_AFTER  2

typedef struct phonol_rule_t {
    phonol_match_t left;
    phonol_match_t center;
    phonol_match_t right;
    int            action;
    phoneme_id_t   arg;        /* REPLACE: new phoneme; INSERT_AFTER: phoneme to insert */
    const char    *name;
} phonol_rule_t;

/* Helpful shorthands for declaring matchers in the rule table. */
#define M_ANY              { PHONOL_ANY, 0, 0, 0 }
#define M_ID(p)            { PHONOL_ID, (p), 0, 0 }
#define M_FEAT(req)        { PHONOL_FEATURES, 0, (req), 0 }
#define M_FEAT_NOT(req, f) { PHONOL_FEATURES, 0, (req), (f) }

/* The boundary sentinel — matches the start/end of the segment stream OR a
 * dedicated pause segment. */
#define M_BOUNDARY         { PHONOL_ID, PH_PAUSE, 0, 0 }

/* ---- Rule table -------------------------------------------------------- */
/*
 * Rules are tried top-to-bottom; the first match fires. After delete/replace
 * the loop re-tests at the same index so a rule can chain into another. The
 * table mirrors the high-value subset of the Amiga's phonrules.i, ported per
 * P4 of documentation/amiga-substrate-port-plan.md.
 *
 * Rule outputs (PH_TQ, PH_LX, PH_RX, PH_DX, PH_Q, PH_AXP) are P4-introduced
 * allophones; the biquad synth treats them like their nearest base phoneme,
 * but the Amiga substrate (P5+) renders them distinctly via the bridge.
 */

static const phonol_rule_t g_phonol_rules[] = {
    /* === Cluster simplifications ============================================ */

    /* /d/ deletion in word-final /nd/ + consonant.
     * "and bags" → "an bags", "behind there" → "behin there".
     * Doesn't fire when the right context is a vowel (linking /d/ stays). */
    {
        M_ID(PH_N), M_ID(PH_D), M_FEAT_NOT(0, F_VOWEL),
        PHONOL_DELETE, PH_PAUSE,
        "nd-cluster simplification"
    },

    /* === Flap rule: intervocalic /t/ or /d/ → DX ============================ */
    /*
     * "letter" /lɛtər/ → /lɛɾər/, "ladder" /lædər/ → /lædɾər/. Fires when
     * the plosive is between two vowels and the FOLLOWING vowel is unstressed.
     * Stress check: the right-context segment's stress field must NOT be ≥ 2
     * (primary stress); we approximate by requiring the right segment to be
     * a vowel and accepting it (the stress test is missing from this simple
     * matcher — refining to honour stress is a future tightening). */
    {
        M_FEAT(F_VOWEL), M_ID(PH_T), M_FEAT(F_VOWEL),
        PHONOL_REPLACE, PH_DX,
        "T flap (vowel /t/ vowel)"
    },
    {
        M_FEAT(F_VOWEL), M_ID(PH_D), M_FEAT(F_VOWEL),
        PHONOL_REPLACE, PH_DX,
        "D flap (vowel /d/ vowel)"
    },
    /* lib-say collapses rhotacised /ɝ/ into PH_R, so "letter" /lɛtər/ comes
     * out as `L EH T R` rather than `L EH T ə R`. Accept R as a flap context
     * too — the underlying schwa is implicit in our R's formant data. */
    {
        M_FEAT(F_VOWEL), M_ID(PH_T), M_ID(PH_R),
        PHONOL_REPLACE, PH_DX,
        "T flap (vowel /t/ R)"
    },
    {
        M_FEAT(F_VOWEL), M_ID(PH_D), M_ID(PH_R),
        PHONOL_REPLACE, PH_DX,
        "D flap (vowel /d/ R)"
    },

    /* === Dark L: postvocalic /l/ → LX ====================================== */
    /*
     * "feel" /fiːl/ → /fiːɫ/, "bell" /bɛl/ → /bɛɫ/. Fires when /l/ follows
     * a vowel and is NOT followed by another vowel (which would keep it
     * "clear"). The Amiga rule keeps L between two vowels with stress on
     * the right; we use the simpler "vowel /l/ ~vowel" form. */
    {
        M_FEAT(F_VOWEL), M_ID(PH_L), M_FEAT_NOT(0, F_VOWEL),
        PHONOL_REPLACE, PH_LX,
        "Dark L (postvocalic)"
    },

    /* === Postvocalic R → RX (when not before stressed vowel) =============== */
    /*
     * The Amiga uses RR as the canonical /r/ and RX for "weakened" postvocalic
     * R. Matches /r/ after a vowel at word/clause end. */
    {
        M_FEAT(F_VOWEL), M_ID(PH_R), M_FEAT_NOT(0, F_VOWEL),
        PHONOL_REPLACE, PH_RX,
        "R weakening (postvocalic)"
    },

    /* === Glottal stop substitution: /t/ at clause end before silence ======= */
    /*
     * "what?" /wɒt/ → /wɒʔ/. The Amiga has multiple Q-substitution rules; we
     * port the simplest one — final voiceless plosive becomes Q at clause
     * boundary. Fires before our /nd/ rule so order matters. */
    {
        M_ANY, M_ID(PH_T), M_BOUNDARY,
        PHONOL_REPLACE, PH_Q,
        "Q glottal (final T at boundary)"
    },

    /* === AXP epenthesis: schwa release after final voiceless plosive ======= */
    /*
     * "but." /bʌt./ → /bʌtə./. After Q replacement (above) this won't fire
     * for word-final T because it's now Q; covers the remaining cases where
     * Q didn't substitute (P, K). Adds a short schwa-like release. */
    {
        M_ANY, M_FEAT_NOT(F_PLOS, F_VOICED), M_BOUNDARY,
        PHONOL_INSERT_AFTER, PH_AXP,
        "AXP epenthesis (final voiceless plosive)"
    },

    /* === Unreleased TQ: /t/ before syllabic /n/, /m/, /l/ ================== */
    /*
     * "button" /bʌtən/ → /bʌtʔn/ — the /t/ is unreleased before the
     * syllabic /n/. Our nearest match: /t/ followed by SCHWA + nasal/lateral.
     * Because phonol matchers see one segment at a time we approximate by
     * the simpler "vowel /t/ schwa" pattern; the syllabic check would need
     * a 2-segment lookahead which the framework currently doesn't expose. */
    /* (Intentionally omitted in this pass — needs lookahead support.) */
};

/* ---- Match + apply ----------------------------------------------------- */

static int phonol_match(const segment_t *seg, const phonol_match_t *m)
{
    say_features_t f;

    if (m->kind == PHONOL_ANY) {
        return seg != NULL && seg->phoneme != PH_PAUSE;
    }
    if (m->kind == PHONOL_ID) {
        if (m->phoneme == PH_PAUSE) {
            /* Boundary sentinel — matches end-of-stream OR a pause segment. */
            return seg == NULL || seg->phoneme == PH_PAUSE;
        }
        return seg != NULL && seg->phoneme == m->phoneme;
    }
    /* PHONOL_FEATURES.
     * End-of-stream and PAUSE segments are treated as "having no features
     * except F_PAUSE" — so a require-mask of 0 with a forbid-mask matches
     * them (e.g. M_FEAT_NOT(0, F_VOWEL) matches "boundary or non-vowel"),
     * while a require-mask with bits set rejects them (e.g. M_FEAT(F_VOWEL)
     * still rejects PAUSE). This is the natural behaviour for postvocalic
     * allophone rules at word/clause end. */
    if (seg == NULL) {
        f = 0;
    } else if (seg->phoneme == PH_PAUSE) {
        f = F_PAUSE;
    } else {
        f = say_get_phoneme(seg->phoneme)->features;
    }
    if (m->require != 0 && (f & m->require) != m->require) return 0;
    if (m->forbid  != 0 && (f & m->forbid)  != 0)          return 0;
    return 1;
}

static int phonol_segment_buffer_insert_after(
    segment_buffer_t *segments,
    size_t after_index,
    phoneme_id_t phoneme
)
{
    segment_t *src;
    segment_t *dst;
    size_t move_count;

    if (!say_segment_buffer_reserve(segments, 1)) {
        return 0;
    }
    /* Shift segments[i+1..count) one to the right, then write the new segment
     * at i+1. The center's metadata is partially inherited so a freshly-
     * inserted phoneme behaves like a continuation of the same word. */
    move_count = segments->count - (after_index + 1);
    if (move_count > 0) {
        memmove(
            &segments->data[after_index + 2],
            &segments->data[after_index + 1],
            move_count * sizeof(segment_t)
        );
    }
    src = &segments->data[after_index];
    dst = &segments->data[after_index + 1];
    dst->phoneme = phoneme;
    dst->duration_scale = 1.0;
    dst->word_start = 0;
    dst->word_end = src->word_end;
    dst->boundary_type = src->boundary_type;
    dst->weak_word = src->weak_word;
    dst->stress = 0;
    dst->diphthong_target = 0;
    dst->accent_n = 0;
    /* The center is no longer the last phoneme of its word. */
    src->word_end = 0;
    ++segments->count;
    return 1;
}

void say_apply_phonological_rules(segment_buffer_t *segments)
{
    size_t i;

    if (segments == NULL || segments->count == 0) {
        return;
    }

    i = 0;
    while (i < segments->count) {
        const segment_t *left   = (i > 0) ? &segments->data[i - 1] : NULL;
        const segment_t *center = &segments->data[i];
        const segment_t *right  = (i + 1 < segments->count) ? &segments->data[i + 1] : NULL;
        size_t r;
        int fired = 0;

        for (r = 0; r < SAY_ARRAY_COUNT(g_phonol_rules); ++r) {
            const phonol_rule_t *rule = &g_phonol_rules[r];
            if (!phonol_match(left, &rule->left))     continue;
            if (!phonol_match(center, &rule->center)) continue;
            if (!phonol_match(right, &rule->right))   continue;

            switch (rule->action) {
                case PHONOL_DELETE:
                    if (i + 1 < segments->count) {
                        memmove(
                            &segments->data[i],
                            &segments->data[i + 1],
                            (segments->count - i - 1) * sizeof(segment_t)
                        );
                    }
                    --segments->count;
                    /* Re-test at this index — the new center may match another rule. */
                    fired = 1;
                    break;
                case PHONOL_REPLACE:
                    segments->data[i].phoneme = rule->arg;
                    /* Re-test at this index. */
                    fired = 1;
                    break;
                case PHONOL_INSERT_AFTER:
                    if (!phonol_segment_buffer_insert_after(segments, i, rule->arg)) {
                        return;
                    }
                    /* Skip the inserted segment so we don't immediately re-fire
                     * on it. */
                    ++i;
                    fired = 1;
                    break;
            }
            if (fired) break;
        }

        if (!fired) {
            ++i;
        } else if (segments->count > 0 && i >= segments->count) {
            /* Last segment was deleted — we're done. */
            break;
        }
    }
}
