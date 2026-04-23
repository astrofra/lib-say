#define _CRT_SECURE_NO_WARNINGS

#include "say.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void tts_print_usage(FILE *stream)
{
    fprintf(stream,
        "Usage:\n"
        "  tts <text-or-input-file> -o <output.{raw|aiff}> [--lang en|fr] [--rate 22050|44100]\n"
        "  tts --phonemes \"HH EH L O\" -o out.aiff\n"
        "\n"
        "Options:\n"
        "  -o, --output <path>   Output audio file (.raw or .aiff)\n"
        "  --lang <en|fr>        Language rules for text input (default: en)\n"
        "  --rate <hz>           Sample rate, 22050 or 44100 (default: 22050)\n"
        "  --frame-ms <5-10>     Frame size in milliseconds (default: 10)\n"
        "  --phonemes            Treat the input as phoneme symbols instead of plain text\n"
        "  -h, --help            Show this message\n"
        "\n"
        "Phoneme mode accepts symbols like:\n"
        "  A AE AH E EH I IH O OH U Y EU SCHWA AN ON IN\n"
        "  W J R L M N NY NG P B T D K G F V S Z SH ZH H TH DH CH JH TS DZ PAUSE\n");
}

static int tts_read_file(const char *path, char **out_text, char *error, size_t error_size)
{
    FILE *file;
    long length;
    char *buffer;
    size_t read_count;

    *out_text = NULL;
    file = fopen(path, "rb");
    if (file == NULL) {
        snprintf(error, error_size, "unable to open %s: %s", path, strerror(errno));
        return 0;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        snprintf(error, error_size, "unable to seek in %s", path);
        return 0;
    }
    length = ftell(file);
    if (length < 0) {
        fclose(file);
        snprintf(error, error_size, "unable to determine size of %s", path);
        return 0;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        snprintf(error, error_size, "unable to rewind %s", path);
        return 0;
    }

    buffer = (char *) malloc((size_t) length + 1);
    if (buffer == NULL) {
        fclose(file);
        snprintf(error, error_size, "out of memory while reading %s", path);
        return 0;
    }

    read_count = fread(buffer, 1, (size_t) length, file);
    fclose(file);
    if (read_count != (size_t) length) {
        free(buffer);
        snprintf(error, error_size, "failed to read %s", path);
        return 0;
    }

    buffer[length] = '\0';
    *out_text = buffer;
    return 1;
}

int main(int argc, char **argv)
{
    say_options_t options;
    const char *input_arg;
    const char *output_path;
    say_audio_format_t format;
    char *input_text;
    int16_t *samples;
    size_t sample_count;
    char error[256];
    int i;

    say_default_options(&options);
    input_arg = NULL;
    output_path = NULL;
    input_text = NULL;
    samples = NULL;
    sample_count = 0;
    error[0] = '\0';

    if (argc <= 1) {
        tts_print_usage(stderr);
        return 1;
    }

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            tts_print_usage(stdout);
            return 0;
        }
        if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "missing value after %s\n", argv[i]);
                return 1;
            }
            output_path = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--lang") == 0) {
            if (i + 1 >= argc || !say_parse_language(argv[i + 1], &options.language)) {
                fprintf(stderr, "invalid language after --lang\n");
                return 1;
            }
            ++i;
            continue;
        }
        if (strcmp(argv[i], "--rate") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "missing value after --rate\n");
                return 1;
            }
            options.sample_rate = atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "--frame-ms") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "missing value after --frame-ms\n");
                return 1;
            }
            options.frame_ms = atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "--phonemes") == 0) {
            options.phoneme_input = 1;
            continue;
        }
        if (input_arg == NULL) {
            input_arg = argv[i];
            continue;
        }

        fprintf(stderr, "unexpected argument: %s\n", argv[i]);
        return 1;
    }

    if (input_arg == NULL) {
        fprintf(stderr, "missing input text or file path\n");
        return 1;
    }
    if (output_path == NULL) {
        fprintf(stderr, "missing -o / --output path\n");
        return 1;
    }

    if (!options.phoneme_input) {
        FILE *probe = fopen(input_arg, "rb");
        if (probe != NULL) {
            fclose(probe);
            if (!tts_read_file(input_arg, &input_text, error, sizeof(error))) {
                fprintf(stderr, "%s\n", error);
                return 1;
            }
        }
        else {
            input_text = _strdup(input_arg);
            if (input_text == NULL) {
                fprintf(stderr, "out of memory while copying input text\n");
                return 1;
            }
        }
    }
    else {
        input_text = _strdup(input_arg);
        if (input_text == NULL) {
            fprintf(stderr, "out of memory while copying phoneme input\n");
            return 1;
        }
    }

    if (!say_synthesize(input_text, &options, &samples, &sample_count, error, sizeof(error))) {
        free(input_text);
        fprintf(stderr, "%s\n", error);
        return 1;
    }

    format = say_guess_audio_format(output_path);
    if (!say_write_audio_file(output_path, format, options.sample_rate, samples, sample_count, error, sizeof(error))) {
        free(input_text);
        say_free(samples);
        fprintf(stderr, "%s\n", error);
        return 1;
    }

    fprintf(stdout, "Wrote %zu samples to %s (%s, %d Hz)\n",
        sample_count,
        output_path,
        format == SAY_FORMAT_AIFF ? "AIFF" : "RAW",
        options.sample_rate);

    free(input_text);
    say_free(samples);
    return 0;
}
