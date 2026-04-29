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

static const phonol_rule_t g_phonol_rules[] = {
    /* Cluster simplification: delete /d/ in word-final /nd/ + consonant.
     * Captures "and bags" → "an bags", "behind there" → "behin there". The
     * /d/ is rarely articulated in fluent speech in this position. The rule
     * doesn't fire when the right context is a vowel (e.g. "and I" keeps /d/
     * because it links into the next syllable). */
    {
        M_ID(PH_N),                              /* left = N */
        M_ID(PH_D),                              /* center = D */
        M_FEAT_NOT(0, F_VOWEL),                  /* right = not a vowel */
        PHONOL_DELETE,
        PH_PAUSE,
        "nd-cluster simplification"
    },
};

/* ---- Match + apply ----------------------------------------------------- */

static int phonol_match(const segment_t *seg, const phonol_match_t *m)
{
    say_features_t f;

    if (m->kind == PHONOL_ANY) {
        return seg != NULL && seg->phoneme != PH_PAUSE;
    }
    if (seg == NULL) {
        /* Off the end of the stream — only matches if rule explicitly asked
         * for the boundary sentinel. */
        return m->kind == PHONOL_ID && m->phoneme == PH_PAUSE;
    }
    if (m->kind == PHONOL_ID) {
        return seg->phoneme == m->phoneme;
    }
    /* PHONOL_FEATURES */
    if (seg->phoneme == PH_PAUSE) {
        return 0;
    }
    f = say_get_phoneme(seg->phoneme)->features;
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
