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
        "  tts <text-or-input-file> -o <output.{raw|aiff|wav}> [--lang en|fr] [--rate 44100]\n"
        "  tts --phonemes \"HH EH L O\" -o out.aiff\n"
        "  tts <text-or-input-file> --debug-report report.txt --dry-run [--lang en|fr]\n"
        "\n"
        "Options:\n"
        "  -o, --output <path>   Output audio file (.raw, .aiff, or .wav)\n"
        "  --lang <en|fr>        Language rules for text input (default: en)\n"
        "  --rate <hz>           Sample rate, 44100 only (default: 44100)\n"
        "  --frame-ms <5-10>     Frame size in milliseconds (default: 5)\n"
        "  --centralize <0-100>  Vowel reduction toward schwa (default: 0)\n"
        "  --articulate <50-200> Steady-portion scale; <100 lazier, >100 clipped (default: 100)\n"
        "  --voice-formants <80-130>  Formant frequency multiplier (default: 100)\n"
        "  --voice-pitch <50-200>     Pitch multiplier (default: 100)\n"
        "  --phonemes            Treat the input as phoneme symbols instead of plain text\n"
        "  --debug-report <p>    Write a debug report to a file, or use - for stdout\n"
        "  --dry-run             Build the debug pipeline but skip audio rendering/output\n"
        "  --amiga               Render through the ported Amiga substrate (22050 Hz LUTs, 44100 Hz output)\n"
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

static int tts_write_text_file(const char *path, const char *text, char *error, size_t error_size)
{
    FILE *file;
    size_t length;

    file = fopen(path, "wb");
    if (file == NULL) {
        snprintf(error, error_size, "unable to open %s: %s", path, strerror(errno));
        return 0;
    }

    length = strlen(text);
    if (length > 0 && fwrite(text, 1, length, file) != length) {
        fclose(file);
        snprintf(error, error_size, "failed to write %s", path);
        return 0;
    }

    fclose(file);
    return 1;
}

static const char *tts_format_label(say_audio_format_t format)
{
    switch (format) {
        case SAY_FORMAT_AIFF:
            return "AIFF";
        case SAY_FORMAT_WAV:
            return "WAV";
        case SAY_FORMAT_RAW:
        default:
            return "RAW";
    }
}

int main(int argc, char **argv)
{
    say_options_t options;
    const char *input_arg;
    const char *output_path;
    const char *debug_report_path;
    say_audio_format_t format;
    char *input_text;
    char *debug_report;
    int16_t *samples;
    size_t sample_count;
    char error[256];
    int dry_run;
    int use_amiga;
    int i;

    say_default_options(&options);
    input_arg = NULL;
    output_path = NULL;
    debug_report_path = NULL;
    input_text = NULL;
    debug_report = NULL;
    samples = NULL;
    sample_count = 0;
    dry_run = 0;
    use_amiga = 0;
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
        if (strcmp(argv[i], "--centralize") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "missing value after --centralize\n");
                return 1;
            }
            options.centralization_pct = atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "--articulate") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "missing value after --articulate\n");
                return 1;
            }
            options.articulation_pct = atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "--voice-formants") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "missing value after --voice-formants\n");
                return 1;
            }
            options.voice_formants_pct = atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "--voice-pitch") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "missing value after --voice-pitch\n");
                return 1;
            }
            options.voice_pitch_pct = atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "--phonemes") == 0) {
            options.phoneme_input = 1;
            continue;
        }
        if (strcmp(argv[i], "--debug-report") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "missing value after --debug-report\n");
                return 1;
            }
            debug_report_path = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--dry-run") == 0) {
            dry_run = 1;
            continue;
        }
        if (strcmp(argv[i], "--amiga") == 0) {
            use_amiga = 1;
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
    if (output_path == NULL && !dry_run) {
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

    if (debug_report_path != NULL) {
        if (!say_build_debug_report(input_text, &options, &debug_report, error, sizeof(error))) {
            free(input_text);
            fprintf(stderr, "%s\n", error);
            return 1;
        }
        if (strcmp(debug_report_path, "-") == 0) {
            fputs(debug_report, stdout);
            if (debug_report[strlen(debug_report) - 1] != '\n') {
                fputc('\n', stdout);
            }
        }
        else if (!tts_write_text_file(debug_report_path, debug_report, error, sizeof(error))) {
            free(input_text);
            say_free(debug_report);
            fprintf(stderr, "%s\n", error);
            return 1;
        }
    }

    if (dry_run) {
        if (debug_report_path != NULL && strcmp(debug_report_path, "-") != 0) {
            fprintf(stdout, "Wrote debug report to %s\n", debug_report_path);
        }
        free(input_text);
        say_free(debug_report);
        return 0;
    }

    int output_sample_rate = options.sample_rate;
    if (use_amiga) {
        if (!say_synthesize_amiga(input_text, &options, &samples, &sample_count,
                                  &output_sample_rate, error, sizeof(error))) {
            free(input_text);
            say_free(debug_report);
            fprintf(stderr, "%s\n", error);
            return 1;
        }
    } else {
        if (!say_synthesize(input_text, &options, &samples, &sample_count, error, sizeof(error))) {
            free(input_text);
            say_free(debug_report);
            fprintf(stderr, "%s\n", error);
            return 1;
        }
    }

    format = say_guess_audio_format(output_path);
    if (!say_write_audio_file(output_path, format, output_sample_rate, samples, sample_count, error, sizeof(error))) {
        free(input_text);
        say_free(debug_report);
        say_free(samples);
        fprintf(stderr, "%s\n", error);
        return 1;
    }

    if (debug_report_path != NULL && strcmp(debug_report_path, "-") != 0) {
        fprintf(stdout, "Wrote debug report to %s\n", debug_report_path);
    }
    fprintf(stdout, "Wrote %zu samples to %s (%s, %d Hz)\n",
        sample_count,
        output_path,
        tts_format_label(format),
        output_sample_rate);

    free(input_text);
    say_free(debug_report);
    say_free(samples);
    return 0;
}
