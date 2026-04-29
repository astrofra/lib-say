#define _CRT_SECURE_NO_WARNINGS

#include "say_internal.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---------------------------------------------------------------------------
 * say_prosody.c — clause analysis, F0 contour, duration cascade, segment to
 * frame expansion (B1 two-tier durations + B2 PROSOD rules + A1 affricate
 * additive + A2 plosive aspiration colouring + A3 diphthong trajectory).
 * ------------------------------------------------------------------------- */

/* Prosody-internal types — used by clause analysis, role assignment and tune
 * selection only. Not exported because no other module needs them. */
typedef enum prosody_role_t {
    SAY_PROSODY_PREHEAD = 0,
    SAY_PROSODY_HEAD,
    SAY_PROSODY_NUCLEUS,
    SAY_PROSODY_TAIL
} prosody_role_t;

/* E2 — accented syllables in a clause, in order, capped at 16. The declination
 * model walks this list to compute peak F0 per AS; segments between adjacent
 * AS interpolate between their peaks. */
#define SAY_MAX_AS_PER_CLAUSE 16

typedef struct clause_prosody_t {
    size_t start;
    size_t end;
    int boundary_type;
    size_t first_vowel;
    size_t last_vowel;
    size_t first_anchor;
    size_t nucleus;
    size_t anchor_count;
    size_t as_indices[SAY_MAX_AS_PER_CLAUSE];
    int    as_accent_n[SAY_MAX_AS_PER_CLAUSE];
    int    as_count;
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

/* === say.c lines 2325..2350 === */
int say_find_next_non_pause(const segment_t *segments, size_t count, size_t start_index)
{
    size_t i;

    for (i = start_index; i < count; ++i) {
        if (segments[i].phoneme != PH_PAUSE) {
            return (int) i;
        }
    }
    return -1;
}

int say_find_prev_non_pause(const segment_t *segments, size_t end_index)
{
    size_t i;

    if (end_index == 0) {
        return -1;
    }
    for (i = end_index; i > 0; --i) {
        if (segments[i - 1].phoneme != PH_PAUSE) {
            return (int) (i - 1);
        }
    }
    return -1;
}

/* === say.c lines 2367..2382 === */

double say_pause_duration_ms(int boundary_type)
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


/* === say.c lines 2421..2427 === */
static double say_transition_alpha(double position, double steady_ratio)
{
    if (position <= steady_ratio) {
        return 0.0;
    }
    return say_smoothstep01((position - steady_ratio) / (1.0 - steady_ratio));
}

/* === say.c lines 2469..2475 === */
static double say_segment_progress(size_t index, size_t start, size_t end, double alpha)
{
    if (end <= start) {
        return say_clamp01(alpha);
    }
    return say_clamp01(((double) (index - start) + alpha) / (double) (end - start));
}

/* === say.c lines 2477..3505 === */
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
    clause->as_count = 0;

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
            /* E2 — record this AS into the ordered declination list. The first
             * SAY_MAX_AS_PER_CLAUSE anchors are kept; anything past that is rare
             * (very long clauses) and its excess is rolled into the tail. */
            if (clause->as_count < SAY_MAX_AS_PER_CLAUSE) {
                int n = segments[i].accent_n;
                if (n <= 0) n = 4;  /* defensive: anchor without explicit level → default */
                clause->as_indices[clause->as_count] = i;
                clause->as_accent_n[clause->as_count] = n;
                ++clause->as_count;
            }
        }
    }

    if (clause->first_anchor == (size_t) -1) {
        clause->first_anchor = clause->first_vowel;
        clause->nucleus = clause->last_vowel;
        /* No real anchors — synthesize a single AS at the last vowel so the
         * declination model still has somewhere to peak. */
        if (clause->last_vowel != (size_t) -1) {
            clause->as_indices[0] = clause->last_vowel;
            clause->as_accent_n[0] = 4;
            clause->as_count = 1;
        }
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

/* E2 — F0 contour from declination + per-AS accent modification.
 *
 * Mirrors the Reference's F0HDPEAK / F0B / F0B3B logic but compressed for our
 * smaller register and run through `prosody_tune_t` for cross-clause-type
 * variation:
 *
 *   1. The clause's "head peak" (offset from BOR) is `tune->head_start`.
 *   2. The clause's "final target" (last AS peak before any tail decay) is
 *      `tune->nucleus_no_tail_end` if the nucleus IS the last vowel (no tail),
 *      otherwise `tune->tail_end`.
 *   3. Each accented syllable sits on the linear declination line connecting
 *      the head peak to the final target (evenly spaced by AS index).
 *   4. Each AS peak is shifted by `(accent_n - 4) × emphasis_pct × local_room`,
 *      where local_room = peak - 0 (baseline already factored out) and
 *      emphasis_pct is a small fraction (~0.10).  accent_n = 4 (default) is
 *      neutral; >4 is more emphatic, <4 is reduced.
 *   5. Segments before the first AS rise from `tune->prehead_start` to AS_0.
 *   6. Segments between AS_i and AS_{i+1} interpolate between their peaks.
 *   7. Segments after the last AS fall to `tune->tail_end`.
 *
 * Heads-up: weak-word drops, schwa dips, and the old role-based bumps are NOT
 * preserved — declination is meant to capture the same effect more uniformly.
 * If a sample regresses on those grounds, the tune knobs above are the place
 * to retune. */

static double say_as_peak(
    const clause_prosody_t *clause,
    const prosody_tune_t *tune,
    int as_idx
)
{
    double head_off = tune->head_start;
    double final_off = (clause->last_vowel == clause->nucleus || clause->nucleus == clause->end)
                       ? tune->nucleus_no_tail_end
                       : tune->tail_end;
    double frac;
    double base_peak;
    double accent_mod;
    int n = clause->as_count;
    int level = clause->as_accent_n[as_idx];

    if (n <= 1) {
        base_peak = head_off;
    } else {
        frac = (double) as_idx / (double) (n - 1);
        base_peak = say_lerp(head_off, final_off, frac);
    }

    /* Per-AS accent modification. The reference modifies by 10% of "local room"
     * (peak above register base); we use the same proportion but scaled by
     * |peak| rather than (peak − BOR) since `base_peak` here is already an
     * offset from BOR (BOR is folded into `base_pitch` upstream). For accent
     * digit 4 (default), modification is zero. */
    {
        double local_room = base_peak >= 0.0 ? base_peak + 4.0 : 4.0;
        accent_mod = ((double)(level - 4)) * 0.10 * local_room;
    }
    return base_peak + accent_mod;
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
    int n;
    int i;
    double t;

    (void) segments;
    (void) segment_count;
    (void) language;

    if (clause->start == (size_t) -1 || clause->first_vowel == (size_t) -1) {
        return 0.0;
    }

    n = clause->as_count;
    if (n == 0) {
        /* Degenerate clause with no vowels — flat. */
        t = say_segment_progress(index, clause->start, clause->end, alpha);
        return say_lerp(tune->prehead_start, tune->tail_end, say_smoothstep01(t));
    }

    /* Locate which AS bracket holds `index`. */
    for (i = 0; i < n; ++i) {
        if (clause->as_indices[i] >= index) {
            break;
        }
    }

    if (i == 0) {
        /* Pre-head: rise from prehead_start to AS_0 peak. */
        double peak0 = say_as_peak(clause, tune, 0);
        t = say_segment_progress(index, clause->start, clause->as_indices[0], alpha);
        return say_lerp(tune->prehead_start, peak0, say_smoothstep01(t));
    }
    if (i == n) {
        /* Tail: fall from AS_{n-1} peak to tail_end. */
        double peak_last = say_as_peak(clause, tune, n - 1);
        t = say_segment_progress(index, clause->as_indices[n - 1], clause->end, alpha);
        return say_lerp(peak_last, tune->tail_end, say_smoothstep01(t));
    }
    if (clause->as_indices[i] == index) {
        /* Sitting on AS_i. */
        return say_as_peak(clause, tune, i);
    }
    /* Between AS_{i-1} and AS_i. */
    {
        double peak_prev = say_as_peak(clause, tune, i - 1);
        double peak_next = say_as_peak(clause, tune, i);
        t = say_segment_progress(index, clause->as_indices[i - 1], clause->as_indices[i], alpha);
        return say_lerp(peak_prev, peak_next, say_smoothstep01(t));
    }
}

int say_generate_frames(
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
        const phoneme_def_t *next_vowel; /* A1/A2 — burst color target for plosives/affricates */
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
        next_vowel = NULL;
        if (options->language == SAY_LANG_EN &&
            (say_is_plosive_phone(segments[i].phoneme) || say_is_affricate_phone(segments[i].phoneme))) {
            size_t k;
            for (k = i + 1; k < segment_count; ++k) {
                if (segments[k].phoneme == PH_PAUSE) {
                    break;
                }
                if (say_is_vowel_phone(segments[k].phoneme)) {
                    next_vowel = say_get_phoneme(segments[k].phoneme);
                    break;
                }
            }
        }
        dental_fricative = say_is_dental_fricative_phone(segments[i].phoneme);
        word_final_fricative = segments[i].word_end &&
            (say_is_fricative_phone(segments[i].phoneme) || say_is_affricate_phone(segments[i].phoneme));
        word_final_sibilant = segments[i].word_end && say_is_sibilant_phone(segments[i].phoneme);
        if (word_final_fricative) {
            target = current;
        }
        /* A3 — diphthong: override the formant target so the in-segment glide drives
         * toward the second element of the diphthong (J or W) rather than toward the
         * next phoneme. */
        if (segments[i].diphthong_target != 0) {
            target = say_get_phoneme((phoneme_id_t) segments[i].diphthong_target);
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

        /* B2 — PROSOD Rule 13: /S/+plosive cluster lengthens the sibilant.
         * Compensates for the perceptually short sibilant in onsets like "stay",
         * "school", "ski". EN only — French sibilant timing is already shorter. */
        if (options->language == SAY_LANG_EN && say_is_sibilant_phone(segments[i].phoneme) &&
            next_index >= 0 && say_is_plosive_phone(segments[next_index].phoneme)) {
            duration_ms *= 1.40;  /* Reference uses 1.75; scaled down because lib-say's sibilant
                                   * base_ms is already higher than the reference's INHDUR. */
        }

        /* B2 — PROSOD Rule 14: plosive after /S/ shortened.
         * The closure phase compresses sharply when preceded by a sibilant. */
        if (options->language == SAY_LANG_EN && say_is_plosive_phone(segments[i].phoneme)) {
            int prev_idx = say_find_prev_non_pause(segments, i);
            if (prev_idx >= 0 && say_is_sibilant_phone(segments[prev_idx].phoneme)) {
                duration_ms *= 0.60;
            }
        }

        /* B2 — PROSOD Rule 9: vowel before voiced fricative is lengthened.
         * Captures pre-voicing — e.g. the long /iː/ in "leave", "these", "is". */
        if (options->language == SAY_LANG_EN && say_is_vowel_phone(segments[i].phoneme) &&
            next_index >= 0 && say_is_voiced_fricative_phone(segments[next_index].phoneme)) {
            duration_ms *= 1.30;  /* Reference uses 1.60; scaled down to avoid stacking with
                                   * the existing word-final fricative bumps. */
        }

        /* B1 — two-tier duration formula (Reference PROSOD final step).
         *
         *   final_ms = (inh_ms - min_ms_eff) * pct + min_ms_eff
         *
         * The cascade above produced `duration_ms` as a pure scalar of `inh_ms`, i.e.
         * `pct = duration_ms / inh_ms`. Re-applying through the two-tier formula caps
         * compression at `min_ms_eff` (the floor) and dampens extreme lengthening,
         * mirroring the Reference MINDUR/INHDUR scheme. The floor is halved for
         * unstressed vowels (Rule 7) so weak vowels can compress further. */
        {
            double inh_ms = current->base_ms * segments[i].duration_scale;
            double min_ms_eff = current->min_ms * segments[i].duration_scale;
            double pct;
            int seg_unstressed_vowel = say_is_vowel_phone(segments[i].phoneme) &&
                (segments[i].weak_word ||
                 (segments[i].stress < 2 && !clause_anchor && !clause_nucleus));
            if (seg_unstressed_vowel) {
                min_ms_eff *= 0.5;
            }
            pct = (inh_ms > 1e-3) ? duration_ms / inh_ms : 1.0;
            duration_ms = (inh_ms - min_ms_eff) * pct + min_ms_eff;
            if (duration_ms < min_ms_eff) {
                duration_ms = min_ms_eff;
            }
        }

        /* B2 — PROSOD Rule 11: a sonorant after a voiceless plosive gets +25 ms.
         * Captures the aspiration burst length spilling into the following sonorant
         * onset (e.g. /pl/ in "play", /tr/ in "true"). Additive, not multiplicative. */
        if (options->language == SAY_LANG_EN && say_is_sonorant_phone(segments[i].phoneme)) {
            int prev_idx = say_find_prev_non_pause(segments, i);
            if (prev_idx >= 0 && say_is_voiceless_plosive_phone(segments[prev_idx].phoneme)) {
                duration_ms += 25.0;
            }
        }

        /* A1 — affricate additive duration (PROSOD Rule 12).
         * The closure-burst-fricative sequence is intrinsically longer than a single
         * phoneme. Reduced ~50% from the first-pass values (40/75 unvoiced,
         * 30/50 voiced) because MFA tended to align the closure phase as
         * silence, leaving only the burst+fricative tagged as the affricate —
         * which made our affricates measure too long against MaryTTS. The new
         * values match Mary's natural CH/JH durations more closely while still
         * giving the closure-burst sequence enough time to be perceptible.
         * The "followed by R" exception preserves the unstressed value
         * ("CHRome", "JREady"). */
        if (options->language == SAY_LANG_EN && say_is_affricate_phone(segments[i].phoneme)) {
            int affric_stressed = 0;
            int followed_by_r = 0;
            double additive_ms;
            size_t k;
            for (k = i + 1; k < segment_count; ++k) {
                if (segments[k].phoneme == PH_PAUSE) {
                    break;
                }
                if (segments[k].phoneme == PH_R) {
                    followed_by_r = 1;
                }
                if (say_is_vowel_phone(segments[k].phoneme)) {
                    if (segments[k].stress >= 2 && !segments[k].weak_word) {
                        affric_stressed = 1;
                    }
                    break;
                }
                if (segments[k].word_end) {
                    break;
                }
            }
            if (current->voiced) {
                /* JH (e.g. judge, changing): stressed +25 ms, unstressed +15 ms */
                additive_ms = (affric_stressed && !followed_by_r) ? 25.0 : 15.0;
            }
            else {
                /* CH (e.g. church, question): stressed +35 ms, unstressed +20 ms */
                additive_ms = (affric_stressed && !followed_by_r) ? 35.0 : 20.0;
            }
            duration_ms += additive_ms;
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
            if (segments[i].diphthong_target != 0) {
                /* A3 — diphthong glide takes ~58% of the segment so the F-trajectory
                 * is clearly perceptible. The first 42% is the steady vowel target. */
                steady_ratio = 0.42;
            }
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
            /* F2 — articulation. articulation_pct=100 leaves steady_ratio alone;
             * <100 shrinks the steady portion (more transition / lazier coarticulation),
             * >100 expands it (clipped / staccato). The "1 - steady_ratio" portion
             * is the transition zone, so multiplying steady_ratio by k effectively
             * scales transition by (1 - k * orig). Clamped to [0.10, 0.90]. */
            if (options->articulation_pct > 0 && options->articulation_pct != 100) {
                steady_ratio *= (double) options->articulation_pct / 100.0;
                if (steady_ratio < 0.10) steady_ratio = 0.10;
                if (steady_ratio > 0.90) steady_ratio = 0.90;
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

            /* A2 — plosive aspiration coloring. A1 — affricate burst coloring.
             * During the burst phase, bias F2/F3 toward the upcoming vowel so the
             * noise burst takes on a vowel-shaped character. The reference PlosAspID[37][5]
             * table picked a specific aspiration vowel-shaped noise per (plosive, vowel);
             * with the biquad bank we approximate it by lifting next-vowel's F2/F3 into
             * the burst formant filter. Active only in EN, between closure end and the
             * onset of the formant transition / fricative phase. */
            if (next_vowel != NULL) {
                double burst_w = 0.0;
                if (say_is_plosive_phone(segments[i].phoneme) && alpha >= 0.30 && alpha < 0.58) {
                    burst_w = sin((alpha - 0.30) / 0.28 * M_PI) * 0.55;
                }
                else if (say_is_affricate_phone(segments[i].phoneme) && alpha >= 0.30 && alpha < 0.48) {
                    burst_w = sin((alpha - 0.30) / 0.18 * M_PI) * 0.45;
                }
                if (burst_w > 0.0) {
                    frame.formant_freq[1] = frame.formant_freq[1] * (1.0 - burst_w) + next_vowel->formant_freq[1] * burst_w;
                    frame.formant_freq[2] = frame.formant_freq[2] * (1.0 - burst_w) + next_vowel->formant_freq[2] * burst_w;
                }
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

            /* F1 — centralization: blend a vowel's formants toward the schwa
             * targets. Applied last so the per-phoneme bandwidth/gain shaping
             * runs first; only formant frequencies get pulled toward schwa.
             * Skipped for sonorants and consonants because the rule is that
             * vowels reduce — consonants stay where they are. */
            if (options->centralization_pct > 0 &&
                say_is_vowel_phone(segments[i].phoneme) &&
                segments[i].phoneme != PH_SCHWA)
            {
                double t = (double) options->centralization_pct / 100.0;
                const phoneme_def_t *schwa = say_get_phoneme(PH_SCHWA);
                if (t > 1.0) t = 1.0;
                for (j = 0; j < SAY_MAX_FORMANTS; ++j) {
                    frame.formant_freq[j] = say_lerp(frame.formant_freq[j], schwa->formant_freq[j], t);
                }
            }

            /* F3 — voice tilt. Multiply all formant frequencies by
             * voice_formants_pct (≈117 for a higher / smaller-vocal-tract voice,
             * ≈92 for a deeper one) and pitch by voice_pitch_pct. Both default
             * to 100 (no change). */
            if (options->voice_formants_pct > 0 && options->voice_formants_pct != 100) {
                double k = (double) options->voice_formants_pct / 100.0;
                for (j = 0; j < SAY_MAX_FORMANTS; ++j) {
                    frame.formant_freq[j] *= k;
                }
            }
            if (options->voice_pitch_pct > 0 && options->voice_pitch_pct != 100) {
                frame.pitch_hz *= (double) options->voice_pitch_pct / 100.0;
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

/* === say.c lines 4259..4309 === */
/* A3 — diphthong fusion. Detects [stressed-vowel + glide] pairs in the same English
 * word and merges them into a single segment that carries an internal F-trajectory
 * (start at the vowel formants, end at the glide formants). The downstream frame
 * generator drives the trajectory using `diphthong_target`. SCHWA and the French
 * nasal vowels never form diphthongs. */
void say_fuse_english_diphthongs(segment_buffer_t *segments)
{
    size_t read;
    size_t write;

    if (segments == NULL || segments->count < 2) {
        return;
    }

    write = 0;
    read = 0;
    while (read < segments->count) {
        segment_t cur = segments->data[read];
        int fused = 0;

        if (read + 1 < segments->count) {
            segment_t nxt = segments->data[read + 1];
            if (say_is_vowel_phone(cur.phoneme) &&
                cur.phoneme != PH_SCHWA &&
                !say_is_nasal_vowel_phone(cur.phoneme) &&
                say_is_glide_phone(nxt.phoneme) &&
                !cur.word_end &&
                cur.diphthong_target == 0) {
                /* Collapse the glide into the vowel. The glide contributes ~55% of its
                 * duration to the diphthong (real diphthongs are ~1.4× a plain vowel,
                 * not 2× as we get from concatenating both segments). */
                cur.duration_scale += nxt.duration_scale * 0.55;
                cur.diphthong_target = (int) nxt.phoneme;
                cur.word_end = nxt.word_end;
                if (nxt.boundary_type > cur.boundary_type) {
                    cur.boundary_type = nxt.boundary_type;
                }
                read += 2;
                fused = 1;
            }
        }

        if (!fused) {
            ++read;
        }

        segments->data[write++] = cur;
    }

    segments->count = write;
}

