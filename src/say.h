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
