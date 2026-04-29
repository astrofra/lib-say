#define _CRT_SECURE_NO_WARNINGS

#include "say_internal.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * say.c — pipeline orchestration, debug-report formatting, and the public C API.
 *
 * The big stages now live in dedicated modules :
 *   - say_phonemes.c      phoneme table + feature predicates
 *   - say_text.c          UTF-8 normalization + lexicon + en/fr phonemizers
 *   - say_prosody.c       clause analysis + duration cascade + frame generation
 *   - say_synth.c         biquad bank + sample-level synthesis
 *   - say_audio_io.c      RAW / AIFF / WAV writers
 *
 * say.c is the thin glue that wires those stages together for the public API
 * declared in say.h.
 * ------------------------------------------------------------------------- */

/* === say.c lines 616..627 === */
void say_set_error(char *error, size_t error_size, const char *fmt, ...)
{
    va_list args;

    if (error == NULL || error_size == 0) {
        return;
    }

    va_start(args, fmt);
    vsnprintf(error, error_size, fmt, args);
    va_end(args);
}

/* === say.c lines 4242..4263 === */
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

/* A3 — diphthong fusion. Detects [stressed-vowel + glide] pairs in the same English
 * word and merges them into a single segment that carries an internal F-trajectory
 * (start at the vowel formants, end at the glide formants). The downstream frame
 * generator drives the trajectory using `diphthong_target`. SCHWA and the French
 * nasal vowels never form diphthongs. */

/* === say.c lines 4311..4377 === */
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

    /* C2 — feature-driven phonological rule pass: cluster simplifications,
     * allophonic substitutions, etc. Currently English-only. Runs before
     * diphthong fusion so the rule context is the raw NRL/lexicon output. */
    if (resolved_options.language == SAY_LANG_EN) {
        say_apply_phonological_rules(segments);
    }

    if (resolved_options.language == SAY_LANG_EN) {
        say_fuse_english_diphthongs(segments);
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

/* === say.c lines 4379..4691 === */
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
        if (segments[i].diphthong_target != 0) {
            const phoneme_def_t *target_phoneme = say_get_phoneme((phoneme_id_t) segments[i].diphthong_target);
            if (!say_text_buffer_appendf(buffer, "->%s", target_phoneme->symbol)) {
                return 0;
            }
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

/* === say.c lines 4693..4905 === */
void say_default_options(say_options_t *options)
{
    if (options == NULL) {
        return;
    }
    options->sample_rate = 44100;
    options->frame_ms = 5;
    options->language = SAY_LANG_EN;
    options->phoneme_input = 0;
    options->centralization_pct = 0;     /* F1 — neutral (no schwa blend) */
    options->articulation_pct  = 100;    /* F2 — neutral (steady_ratio unchanged) */
    options->voice_formants_pct = 100;   /* F3 — neutral (no formant tilt) */
    options->voice_pitch_pct   = 100;    /* F3 — neutral (no pitch shift) */
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


/* === say.c lines 5075..5078 === */
void say_free(void *ptr)
{
    free(ptr);
}

