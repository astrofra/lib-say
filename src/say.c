#define _CRT_SECURE_NO_WARNINGS

#include "say.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SAY_MAX_FORMANTS 5

typedef enum phoneme_id_t {
    PH_PAUSE = 0,
    PH_A,
    PH_AE,
    PH_AH,
    PH_E,
    PH_EH,
    PH_I,
    PH_IH,
    PH_O,
    PH_OH,
    PH_U,
    PH_Y,
    PH_EU,
    PH_SCHWA,
    PH_AN,
    PH_ON,
    PH_IN,
    PH_W,
    PH_J,
    PH_R,
    PH_L,
    PH_M,
    PH_N,
    PH_NY,
    PH_NG,
    PH_P,
    PH_B,
    PH_T,
    PH_D,
    PH_K,
    PH_G,
    PH_F,
    PH_V,
    PH_S,
    PH_Z,
    PH_SH,
    PH_ZH,
    PH_H,
    PH_TH,
    PH_DH,
    PH_CH,
    PH_JH,
    PH_TS,
    PH_DZ,
    PH_COUNT
} phoneme_id_t;

typedef struct phoneme_def_t {
    phoneme_id_t id;
    const char *symbol;
    int is_vowel;
    int voiced;
    double base_ms;
    double amplitude;
    double noise_mix;
    double formant_freq[SAY_MAX_FORMANTS];
    double bandwidth[SAY_MAX_FORMANTS];
    double gain[SAY_MAX_FORMANTS];
} phoneme_def_t;

typedef struct segment_t {
    phoneme_id_t phoneme;
    double duration_scale;
    int word_start;
    int word_end;
    int boundary_type;
} segment_t;

typedef struct segment_buffer_t {
    segment_t *data;
    size_t count;
    size_t capacity;
} segment_buffer_t;

typedef struct frame_t {
    double pitch_hz;
    double amplitude;
    double noise_mix;
    int voiced;
    int is_pause;
    double formant_freq[SAY_MAX_FORMANTS];
    double bandwidth[SAY_MAX_FORMANTS];
    double gain[SAY_MAX_FORMANTS];
} frame_t;

typedef struct frame_buffer_t {
    frame_t *data;
    size_t count;
    size_t capacity;
} frame_buffer_t;

typedef struct biquad_t {
    double b0;
    double b1;
    double b2;
    double a1;
    double a2;
    double z1;
    double z2;
} biquad_t;

static const phoneme_def_t g_phonemes[PH_COUNT] = {
    { PH_PAUSE, "PAUSE", 0, 0, 90.0, 0.0, 0.0, { 0 }, { 0 }, { 0 } },
    { PH_A, "A", 1, 1, 115.0, 1.00, 0.02, { 800, 1200, 2800, 3600, 4500 }, { 90, 100, 150, 200, 260 }, { 1.00, 0.85, 0.35, 0.16, 0.08 } },
    { PH_AE, "AE", 1, 1, 110.0, 1.00, 0.02, { 700, 1700, 2500, 3600, 4500 }, { 90, 110, 150, 200, 260 }, { 1.00, 0.82, 0.34, 0.15, 0.07 } },
    { PH_AH, "AH", 1, 1, 105.0, 0.96, 0.03, { 650, 1200, 2500, 3400, 4400 }, { 100, 120, 160, 200, 260 }, { 0.98, 0.78, 0.30, 0.14, 0.07 } },
    { PH_E, "E", 1, 1, 108.0, 0.98, 0.02, { 500, 1900, 2600, 3400, 4300 }, { 90, 100, 130, 180, 240 }, { 0.98, 0.84, 0.34, 0.14, 0.07 } },
    { PH_EH, "EH", 1, 1, 104.0, 0.98, 0.02, { 600, 1700, 2500, 3400, 4300 }, { 95, 105, 135, 185, 240 }, { 0.98, 0.83, 0.34, 0.15, 0.07 } },
    { PH_I, "I", 1, 1, 108.0, 0.98, 0.02, { 300, 2200, 3000, 3800, 4700 }, { 70, 95, 120, 180, 240 }, { 0.96, 0.82, 0.32, 0.14, 0.06 } },
    { PH_IH, "IH", 1, 1, 96.0, 0.92, 0.02, { 400, 2000, 2800, 3600, 4500 }, { 80, 100, 130, 185, 240 }, { 0.92, 0.78, 0.30, 0.13, 0.06 } },
    { PH_O, "O", 1, 1, 110.0, 1.00, 0.02, { 450, 900, 2400, 3400, 4300 }, { 90, 100, 140, 200, 240 }, { 1.00, 0.84, 0.34, 0.14, 0.06 } },
    { PH_OH, "OH", 1, 1, 108.0, 0.98, 0.02, { 550, 900, 2400, 3400, 4300 }, { 95, 105, 145, 200, 240 }, { 0.98, 0.84, 0.34, 0.14, 0.06 } },
    { PH_U, "U", 1, 1, 112.0, 0.96, 0.02, { 350, 800, 2200, 3200, 4200 }, { 70, 95, 130, 190, 240 }, { 0.96, 0.80, 0.30, 0.13, 0.06 } },
    { PH_Y, "Y", 1, 1, 110.0, 0.96, 0.02, { 320, 1750, 2400, 3300, 4200 }, { 75, 100, 130, 180, 240 }, { 0.96, 0.80, 0.30, 0.13, 0.06 } },
    { PH_EU, "EU", 1, 1, 106.0, 0.95, 0.02, { 400, 1500, 2400, 3300, 4200 }, { 80, 100, 130, 180, 240 }, { 0.95, 0.80, 0.30, 0.13, 0.06 } },
    { PH_SCHWA, "SCHWA", 1, 1, 85.0, 0.88, 0.03, { 500, 1500, 2500, 3400, 4300 }, { 100, 120, 160, 210, 260 }, { 0.88, 0.74, 0.28, 0.12, 0.06 } },
    { PH_AN, "AN", 1, 1, 118.0, 0.96, 0.04, { 650, 1100, 2300, 3200, 4100 }, { 110, 130, 170, 210, 260 }, { 0.94, 0.76, 0.30, 0.12, 0.06 } },
    { PH_ON, "ON", 1, 1, 116.0, 0.96, 0.04, { 500, 900, 2200, 3200, 4100 }, { 110, 130, 170, 210, 260 }, { 0.94, 0.76, 0.30, 0.12, 0.06 } },
    { PH_IN, "IN", 1, 1, 114.0, 0.95, 0.04, { 450, 1800, 2500, 3400, 4300 }, { 110, 125, 160, 210, 260 }, { 0.94, 0.80, 0.30, 0.12, 0.06 } },
    { PH_W, "W", 0, 1, 72.0, 0.70, 0.03, { 320, 800, 2200, 3000, 3900 }, { 110, 130, 180, 220, 280 }, { 0.72, 0.56, 0.18, 0.07, 0.03 } },
    { PH_J, "J", 0, 1, 68.0, 0.68, 0.04, { 320, 2100, 2900, 3600, 4400 }, { 110, 130, 180, 220, 280 }, { 0.70, 0.58, 0.18, 0.07, 0.03 } },
    { PH_R, "R", 0, 1, 76.0, 0.72, 0.08, { 300, 1300, 1800, 2600, 3500 }, { 110, 140, 190, 240, 300 }, { 0.74, 0.56, 0.20, 0.08, 0.03 } },
    { PH_L, "L", 0, 1, 76.0, 0.70, 0.04, { 350, 1500, 2600, 3400, 4300 }, { 110, 130, 180, 220, 280 }, { 0.72, 0.56, 0.18, 0.07, 0.03 } },
    { PH_M, "M", 0, 1, 78.0, 0.72, 0.05, { 250, 1200, 2100, 2900, 3800 }, { 130, 150, 200, 240, 300 }, { 0.74, 0.54, 0.18, 0.07, 0.03 } },
    { PH_N, "N", 0, 1, 76.0, 0.72, 0.05, { 300, 1500, 2500, 3400, 4300 }, { 130, 150, 200, 240, 300 }, { 0.74, 0.54, 0.18, 0.07, 0.03 } },
    { PH_NY, "NY", 0, 1, 78.0, 0.72, 0.06, { 300, 1800, 2600, 3500, 4300 }, { 130, 150, 200, 240, 300 }, { 0.74, 0.54, 0.18, 0.07, 0.03 } },
    { PH_NG, "NG", 0, 1, 78.0, 0.72, 0.06, { 300, 1400, 2200, 3000, 3800 }, { 130, 150, 200, 240, 300 }, { 0.74, 0.54, 0.18, 0.07, 0.03 } },
    { PH_P, "P", 0, 0, 68.0, 0.62, 1.00, { 500, 1400, 2400, 3500, 4500 }, { 300, 340, 380, 420, 500 }, { 0.40, 0.28, 0.24, 0.20, 0.14 } },
    { PH_B, "B", 0, 1, 70.0, 0.64, 0.50, { 500, 1400, 2400, 3500, 4500 }, { 300, 340, 380, 420, 500 }, { 0.42, 0.30, 0.24, 0.20, 0.14 } },
    { PH_T, "T", 0, 0, 66.0, 0.62, 1.00, { 600, 1700, 2800, 3800, 4800 }, { 280, 330, 380, 420, 500 }, { 0.40, 0.28, 0.24, 0.20, 0.14 } },
    { PH_D, "D", 0, 1, 68.0, 0.64, 0.45, { 600, 1700, 2800, 3800, 4800 }, { 280, 330, 380, 420, 500 }, { 0.42, 0.30, 0.24, 0.20, 0.14 } },
    { PH_K, "K", 0, 0, 70.0, 0.62, 1.00, { 700, 1800, 3000, 3900, 4900 }, { 280, 330, 380, 420, 500 }, { 0.40, 0.28, 0.24, 0.20, 0.14 } },
    { PH_G, "G", 0, 1, 72.0, 0.64, 0.50, { 700, 1800, 3000, 3900, 4900 }, { 280, 330, 380, 420, 500 }, { 0.42, 0.30, 0.24, 0.20, 0.14 } },
    { PH_F, "F", 0, 0, 82.0, 0.66, 1.00, { 900, 1800, 2900, 3900, 5000 }, { 250, 290, 340, 390, 450 }, { 0.42, 0.34, 0.28, 0.22, 0.16 } },
    { PH_V, "V", 0, 1, 84.0, 0.68, 0.60, { 900, 1800, 2900, 3900, 5000 }, { 250, 290, 340, 390, 450 }, { 0.44, 0.34, 0.28, 0.22, 0.16 } },
    { PH_S, "S", 0, 0, 88.0, 0.66, 1.00, { 1000, 2300, 3500, 4500, 5600 }, { 230, 280, 330, 380, 430 }, { 0.44, 0.36, 0.30, 0.24, 0.18 } },
    { PH_Z, "Z", 0, 1, 88.0, 0.68, 0.65, { 1000, 2300, 3500, 4500, 5600 }, { 230, 280, 330, 380, 430 }, { 0.46, 0.36, 0.30, 0.24, 0.18 } },
    { PH_SH, "SH", 0, 0, 90.0, 0.68, 1.00, { 1200, 2500, 3600, 4700, 5800 }, { 220, 270, 320, 370, 420 }, { 0.46, 0.38, 0.30, 0.24, 0.18 } },
    { PH_ZH, "ZH", 0, 1, 90.0, 0.70, 0.70, { 1200, 2500, 3600, 4700, 5800 }, { 220, 270, 320, 370, 420 }, { 0.48, 0.38, 0.30, 0.24, 0.18 } },
    { PH_H, "H", 0, 0, 68.0, 0.48, 0.85, { 1000, 1700, 2600, 3500, 4400 }, { 260, 310, 360, 410, 470 }, { 0.34, 0.26, 0.20, 0.16, 0.12 } },
    { PH_TH, "TH", 0, 0, 86.0, 0.64, 1.00, { 1200, 2400, 3400, 4500, 5600 }, { 220, 270, 320, 370, 420 }, { 0.42, 0.34, 0.28, 0.22, 0.16 } },
    { PH_DH, "DH", 0, 1, 86.0, 0.66, 0.65, { 1200, 2400, 3400, 4500, 5600 }, { 220, 270, 320, 370, 420 }, { 0.44, 0.34, 0.28, 0.22, 0.16 } },
    { PH_CH, "CH", 0, 0, 92.0, 0.68, 1.00, { 1300, 2600, 3700, 4800, 5900 }, { 220, 270, 320, 370, 420 }, { 0.46, 0.38, 0.30, 0.24, 0.18 } },
    { PH_JH, "JH", 0, 1, 92.0, 0.70, 0.70, { 1300, 2600, 3700, 4800, 5900 }, { 220, 270, 320, 370, 420 }, { 0.48, 0.38, 0.30, 0.24, 0.18 } },
    { PH_TS, "TS", 0, 0, 90.0, 0.66, 1.00, { 1200, 2500, 3600, 4700, 5800 }, { 220, 270, 320, 370, 420 }, { 0.46, 0.38, 0.30, 0.24, 0.18 } },
    { PH_DZ, "DZ", 0, 1, 90.0, 0.68, 0.70, { 1200, 2500, 3600, 4700, 5800 }, { 220, 270, 320, 370, 420 }, { 0.48, 0.38, 0.30, 0.24, 0.18 } }
};

static void say_set_error(char *error, size_t error_size, const char *fmt, ...)
{
    va_list args;

    if (error == NULL || error_size == 0) {
        return;
    }

    va_start(args, fmt);
    vsnprintf(error, error_size, fmt, args);
    va_end(args);
}

static const phoneme_def_t *say_get_phoneme(phoneme_id_t id)
{
    if ((int) id < 0 || id >= PH_COUNT) {
        return &g_phonemes[PH_PAUSE];
    }
    return &g_phonemes[id];
}

static int say_is_vowel_phone(phoneme_id_t id)
{
    return say_get_phoneme(id)->is_vowel;
}

static int say_is_boundary_char(char c)
{
    return c == '.' || c == ',' || c == '!' || c == '?' || c == ';' || c == ':';
}

static int say_is_word_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

static int say_equals_icase(const char *a, const char *b)
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

static int say_segment_buffer_reserve(segment_buffer_t *buffer, size_t extra)
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

static int say_segment_buffer_push(
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
    return 1;
}

static int say_frame_buffer_reserve(frame_buffer_t *buffer, size_t extra)
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

static int say_frame_buffer_push(frame_buffer_t *buffer, const frame_t *frame)
{
    if (!say_frame_buffer_reserve(buffer, 1)) {
        return 0;
    }
    buffer->data[buffer->count++] = *frame;
    return 1;
}

static size_t say_utf8_decode(const unsigned char *text, unsigned int *codepoint)
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

static char *say_normalize_text(const char *input, char *error, size_t error_size)
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
            if (ascii == '\'' || ascii == '-' || ascii == '\n' || ascii == '\r' || ascii == '\t') {
                ascii = ' ';
            }

            if (say_is_word_char(ascii) || say_is_boundary_char(ascii) || ascii == ' ') {
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
                    if (ascii == '\'' || ascii == '-') {
                        ascii = ' ';
                    }
                    if (!say_is_word_char(ascii) && ascii != ' ' && !say_is_boundary_char(ascii)) {
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

static int say_append_phone(segment_buffer_t *segments, phoneme_id_t phoneme)
{
    return say_segment_buffer_push(segments, phoneme, 1.0, 0);
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

    start = segments->count;
    i = 0;
    while (word[i] != '\0') {
        if (say_match_at(word, i, "tion")) {
            if (!say_append_phone(segments, PH_SH) || !say_append_phone(segments, PH_SCHWA) || !say_append_phone(segments, PH_N)) {
                return 0;
            }
            i += 4;
            continue;
        }
        if (say_match_at(word, i, "sion")) {
            if (!say_append_phone(segments, PH_ZH) || !say_append_phone(segments, PH_SCHWA) || !say_append_phone(segments, PH_N)) {
                return 0;
            }
            i += 4;
            continue;
        }
        if (say_match_at(word, i, "tch")) {
            if (!say_append_phone(segments, PH_CH)) {
                return 0;
            }
            i += 3;
            continue;
        }
        if (say_match_at(word, i, "dge")) {
            if (!say_append_phone(segments, PH_JH)) {
                return 0;
            }
            i += 3;
            continue;
        }
        if (say_match_at(word, i, "igh")) {
            if (!say_append_phone(segments, PH_AH) || !say_append_phone(segments, PH_I)) {
                return 0;
            }
            i += 3;
            continue;
        }
        if (say_match_at(word, i, "ee") || say_match_at(word, i, "ea")) {
            if (!say_append_phone(segments, PH_I)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "oo")) {
            if (!say_append_phone(segments, PH_U)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "ow") || say_match_at(word, i, "ou")) {
            if (!say_append_phone(segments, PH_AH) || !say_append_phone(segments, PH_U)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "oi") || say_match_at(word, i, "oy")) {
            if (!say_append_phone(segments, PH_OH) || !say_append_phone(segments, PH_I)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "ai") || say_match_at(word, i, "ay") || say_match_at(word, i, "ei") || say_match_at(word, i, "ey")) {
            if (!say_append_phone(segments, PH_E) || !say_append_phone(segments, PH_I)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "ph")) {
            if (!say_append_phone(segments, PH_F)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "sh")) {
            if (!say_append_phone(segments, PH_SH)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "ch")) {
            if (!say_append_phone(segments, PH_CH)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "th")) {
            phoneme_id_t phone = (i == 0 && say_has_vowel_after(word, i + 2)) ? PH_DH : PH_TH;
            if (!say_append_phone(segments, phone)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "ng")) {
            if (!say_append_phone(segments, PH_NG)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "qu")) {
            if (!say_append_phone(segments, PH_K) || !say_append_phone(segments, PH_W)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "ck")) {
            if (!say_append_phone(segments, PH_K)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "wr")) {
            if (!say_append_phone(segments, PH_R)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "wh")) {
            if (!say_append_phone(segments, PH_W)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (i == 0 && say_match_at(word, i, "kn")) {
            if (!say_append_phone(segments, PH_N)) {
                return 0;
            }
            i += 2;
            continue;
        }

        switch (word[i]) {
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
                if (!say_append_phone(segments, PH_S)) {
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
        segments->data[start].word_start = 1;
        segments->data[segments->count - 1].word_end = 1;
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

    start = segments->count;
    len = strlen(word);
    i = 0;
    while (i < len) {
        if (i + 2 == len && say_match_at(word, i, "es")) {
            break;
        }
        if (i + 3 == len && say_match_at(word, i, "ent")) {
            break;
        }
        if (i + 1 == len && word[i] == 'e') {
            break;
        }
        if (say_match_at(word, i, "eaux") || say_match_at(word, i, "eau")) {
            if (!say_append_phone(segments, PH_O)) {
                return 0;
            }
            i += say_match_at(word, i, "eaux") ? 4 : 3;
            continue;
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
        if (say_match_at(word, i, "ou")) {
            if (!say_append_phone(segments, PH_U)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "oi")) {
            if (!say_append_phone(segments, PH_W) || !say_append_phone(segments, PH_A)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "oy")) {
            if (!say_append_phone(segments, PH_W) || !say_append_phone(segments, PH_A) || !say_append_phone(segments, PH_I)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "eu") || say_match_at(word, i, "oeu")) {
            if (!say_append_phone(segments, PH_EU)) {
                return 0;
            }
            i += say_match_at(word, i, "oeu") ? 3 : 2;
            continue;
        }
        if (say_match_at(word, i, "au")) {
            if (!say_append_phone(segments, PH_O)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "ai") || say_match_at(word, i, "ei") || say_match_at(word, i, "er") || say_match_at(word, i, "ez")) {
            if (!say_append_phone(segments, PH_E)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "gn")) {
            if (!say_append_phone(segments, PH_NY)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "ill") && i > 0 && say_is_vowel_char(word[i - 1])) {
            if (!say_append_phone(segments, PH_J)) {
                return 0;
            }
            i += 3;
            continue;
        }
        if (say_match_at(word, i, "ch")) {
            if (!say_append_phone(segments, PH_SH)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "ph")) {
            if (!say_append_phone(segments, PH_F)) {
                return 0;
            }
            i += 2;
            continue;
        }
        if (say_match_at(word, i, "qu")) {
            if (!say_append_phone(segments, PH_K)) {
                return 0;
            }
            i += 2;
            continue;
        }

        switch (word[i]) {
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
                if (word[i + 1] != '\0' && say_is_vowel_char(word[i - (i > 0 ? 1 : 0)]) && say_is_vowel_char(word[i + 1])) {
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
        segments->data[start].word_start = 1;
        segments->data[segments->count - 1].word_end = 1;
    }
    return 1;
}

static int say_append_text_word(const char *word, say_language_t language, segment_buffer_t *segments)
{
    if (word[0] == '\0') {
        return 1;
    }
    if (language == SAY_LANG_FR) {
        return say_phonemize_french_word(word, segments);
    }
    return say_phonemize_english_word(word, segments);
}

static int say_append_digit_word(char digit, say_language_t language, segment_buffer_t *segments)
{
    static const char *g_en_digits[] = {
        "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine"
    };
    static const char *g_fr_digits[] = {
        "zero", "un", "deux", "trois", "quatre", "cinq", "six", "sept", "huit", "neuf"
    };
    const char *word;

    word = language == SAY_LANG_FR ? g_fr_digits[digit - '0'] : g_en_digits[digit - '0'];
    return say_append_text_word(word, language, segments);
}

static int say_phonemize_text(
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
        if (say_is_word_char(normalized_text[i])) {
            size_t start;
            char word[128];
            size_t count;

            start = i;
            while (normalized_text[i] != '\0' && say_is_word_char(normalized_text[i])) {
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

static const phoneme_def_t *say_lookup_symbol(const char *symbol)
{
    size_t i;

    for (i = 0; i < PH_COUNT; ++i) {
        if (say_equals_icase(g_phonemes[i].symbol, symbol)) {
            return &g_phonemes[i];
        }
    }
    return NULL;
}

static int say_parse_phoneme_input(
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

static int say_find_next_non_pause(const segment_t *segments, size_t count, size_t start_index)
{
    size_t i;

    for (i = start_index; i < count; ++i) {
        if (segments[i].phoneme != PH_PAUSE) {
            return (int) i;
        }
    }
    return -1;
}

static double say_pause_duration_ms(int boundary_type)
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

static int say_generate_frames(
    const segment_t *segments,
    size_t segment_count,
    const say_options_t *options,
    frame_buffer_t *frames,
    char *error,
    size_t error_size
)
{
    size_t i;
    size_t total_voiced_slots;
    size_t voiced_slot;
    int question_like;

    total_voiced_slots = 0;
    question_like = 0;
    for (i = 0; i < segment_count; ++i) {
        if (segments[i].phoneme != PH_PAUSE) {
            ++total_voiced_slots;
        }
        if (segments[i].boundary_type == 3) {
            question_like = 1;
        }
    }

    voiced_slot = 0;
    for (i = 0; i < segment_count; ++i) {
        const phoneme_def_t *current;
        const phoneme_def_t *target;
        double duration_ms;
        int frame_count;
        size_t frame_index;
        frame_t frame;
        double progress;
        double base_pitch;
        double stress_boost;
        int next_index;
        int vowel_count_in_word;
        int this_vowel_rank;
        size_t j;

        current = say_get_phoneme(segments[i].phoneme);
        if (segments[i].phoneme == PH_PAUSE) {
            duration_ms = say_pause_duration_ms(segments[i].boundary_type) * segments[i].duration_scale;
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

        next_index = say_find_next_non_pause(segments, segment_count, i + 1);
        target = next_index >= 0 ? say_get_phoneme(segments[next_index].phoneme) : current;
        duration_ms = current->base_ms * segments[i].duration_scale;

        vowel_count_in_word = 0;
        this_vowel_rank = 0;
        if (say_is_vowel_phone(segments[i].phoneme)) {
            for (j = i; j < segment_count && !segments[j].word_end; ++j) {
                if (say_is_vowel_phone(segments[j].phoneme)) {
                    ++vowel_count_in_word;
                    if (j == i) {
                        this_vowel_rank = vowel_count_in_word;
                    }
                }
            }
            if (segments[j].word_end && say_is_vowel_phone(segments[j].phoneme)) {
                ++vowel_count_in_word;
                if (j == i) {
                    this_vowel_rank = vowel_count_in_word;
                }
            }
        }

        base_pitch = options->language == SAY_LANG_FR ? 118.0 : 126.0;
        progress = total_voiced_slots > 1 ? (double) voiced_slot / (double) (total_voiced_slots - 1) : 0.0;
        base_pitch += (options->language == SAY_LANG_FR ? -12.0 : -16.0) * progress;
        if (question_like && progress > 0.72) {
            base_pitch += 32.0 * (progress - 0.72) / 0.28;
        }
        if (segments[i].boundary_type == 4) {
            base_pitch += 12.0;
        }

        stress_boost = 0.0;
        if (say_is_vowel_phone(segments[i].phoneme)) {
            if (options->language == SAY_LANG_EN) {
                if (this_vowel_rank == 1) {
                    stress_boost = 1.0;
                }
                if (vowel_count_in_word == 1) {
                    stress_boost = 0.8;
                }
            }
            else {
                if (segments[i].word_end) {
                    stress_boost = 0.9;
                }
                else {
                    stress_boost = 0.35;
                }
            }
        }
        else if (current->voiced) {
            stress_boost = 0.1;
        }

        duration_ms *= 1.0 + 0.18 * stress_boost;
        if (segments[i].word_end && say_is_vowel_phone(segments[i].phoneme) && options->language == SAY_LANG_FR) {
            duration_ms *= 1.10;
        }

        frame_count = (int) ceil(duration_ms / options->frame_ms);
        if (frame_count < 1) {
            frame_count = 1;
        }

        for (frame_index = 0; frame_index < (size_t) frame_count; ++frame_index) {
            double alpha;

            alpha = frame_count == 1 ? 0.0 : (double) frame_index / (double) (frame_count - 1);
            memset(&frame, 0, sizeof(frame));
            frame.voiced = current->voiced;
            frame.is_pause = 0;
            frame.pitch_hz = base_pitch + 12.0 * stress_boost;
            frame.amplitude = current->amplitude * (0.88 + 0.14 * stress_boost);
            frame.noise_mix = current->noise_mix;

            for (j = 0; j < SAY_MAX_FORMANTS; ++j) {
                frame.formant_freq[j] = current->formant_freq[j] + (target->formant_freq[j] - current->formant_freq[j]) * alpha;
                frame.bandwidth[j] = current->bandwidth[j] + (target->bandwidth[j] - current->bandwidth[j]) * alpha;
                frame.gain[j] = current->gain[j] + (target->gain[j] - current->gain[j]) * alpha;
            }

            if (!say_frame_buffer_push(frames, &frame)) {
                say_set_error(error, error_size, "out of memory while generating speech frames");
                return 0;
            }
        }

        ++voiced_slot;
    }

    if (frames->count == 0) {
        say_set_error(error, error_size, "frame generator produced no output");
        return 0;
    }
    return 1;
}

static void say_biquad_set_bandpass(biquad_t *filter, double sample_rate, double frequency, double bandwidth, double gain)
{
    double q;
    double w0;
    double sin_w0;
    double cos_w0;
    double alpha;
    double a0;

    if (frequency < 80.0) {
        frequency = 80.0;
    }
    if (frequency > sample_rate * 0.45) {
        frequency = sample_rate * 0.45;
    }
    if (bandwidth < 30.0) {
        bandwidth = 30.0;
    }

    q = frequency / bandwidth;
    if (q < 0.25) {
        q = 0.25;
    }

    w0 = 2.0 * M_PI * frequency / sample_rate;
    sin_w0 = sin(w0);
    cos_w0 = cos(w0);
    alpha = sin_w0 / (2.0 * q);
    a0 = 1.0 + alpha;

    filter->b0 = gain * (alpha / a0);
    filter->b1 = 0.0;
    filter->b2 = gain * (-alpha / a0);
    filter->a1 = -2.0 * cos_w0 / a0;
    filter->a2 = (1.0 - alpha) / a0;
}

static double say_biquad_process(biquad_t *filter, double input)
{
    double output;

    output = filter->b0 * input + filter->z1;
    filter->z1 = filter->b1 * input - filter->a1 * output + filter->z2;
    filter->z2 = filter->b2 * input - filter->a2 * output;
    return output;
}

static int say_synthesize_frames(
    const frame_t *frames,
    size_t frame_count,
    int sample_rate,
    int frame_ms,
    int16_t **out_samples,
    size_t *out_sample_count,
    char *error,
    size_t error_size
)
{
    biquad_t filters[SAY_MAX_FORMANTS];
    size_t i;
    int64_t frame_sample_accum;
    size_t total_samples;
    int16_t *samples;
    size_t sample_index;
    double phase;
    unsigned int rng;

    memset(filters, 0, sizeof(filters));
    frame_sample_accum = 0;
    total_samples = 0;
    for (i = 0; i < frame_count; ++i) {
        int frame_samples;
        frame_sample_accum += (int64_t) sample_rate * (int64_t) frame_ms;
        frame_samples = (int) (frame_sample_accum / 1000);
        frame_sample_accum %= 1000;
        total_samples += (size_t) frame_samples;
    }

    samples = (int16_t *) malloc(total_samples * sizeof(*samples));
    if (samples == NULL) {
        say_set_error(error, error_size, "out of memory while allocating %zu samples", total_samples);
        return 0;
    }

    phase = 0.0;
    rng = 0x12345678u;
    sample_index = 0;
    frame_sample_accum = 0;
    for (i = 0; i < frame_count; ++i) {
        size_t j;
        int frame_samples;

        for (j = 0; j < SAY_MAX_FORMANTS; ++j) {
            say_biquad_set_bandpass(&filters[j], (double) sample_rate, frames[i].formant_freq[j], frames[i].bandwidth[j], frames[i].gain[j]);
        }

        frame_sample_accum += (int64_t) sample_rate * (int64_t) frame_ms;
        frame_samples = (int) (frame_sample_accum / 1000);
        frame_sample_accum %= 1000;

        for (j = 0; j < (size_t) frame_samples; ++j) {
            double glottal;
            double noise;
            double excitation;
            double voiced_mix;
            double output;
            size_t k;

            if (frames[i].voiced && frames[i].pitch_hz > 1.0) {
                phase += frames[i].pitch_hz / (double) sample_rate;
                if (phase >= 1.0) {
                    phase -= floor(phase);
                }
                glottal = 2.0 * phase - 1.0;
                glottal = tanh(glottal * 1.4) * 0.9;
            }
            else {
                glottal = 0.0;
            }

            rng = rng * 1664525u + 1013904223u;
            noise = ((double) ((rng >> 8) & 0x00FFFFFFu) / 8388608.0) - 1.0;

            voiced_mix = frames[i].voiced ? (1.0 - frames[i].noise_mix) : 0.0;
            excitation = voiced_mix * glottal + frames[i].noise_mix * noise;

            output = 0.0;
            for (k = 0; k < SAY_MAX_FORMANTS; ++k) {
                output += say_biquad_process(&filters[k], excitation);
            }

            if (frames[i].is_pause) {
                output *= 0.1;
                for (k = 0; k < SAY_MAX_FORMANTS; ++k) {
                    filters[k].z1 *= 0.86;
                    filters[k].z2 *= 0.86;
                }
            }

            output *= frames[i].amplitude * 0.46;
            output = tanh(output);

            if (output > 1.0) {
                output = 1.0;
            }
            if (output < -1.0) {
                output = -1.0;
            }

            samples[sample_index++] = (int16_t) lrint(output * 32767.0);
        }
    }

    *out_samples = samples;
    *out_sample_count = sample_index;
    return 1;
}

static void say_write_u16_be(FILE *file, unsigned int value)
{
    unsigned char bytes[2];

    bytes[0] = (unsigned char) ((value >> 8) & 0xFFu);
    bytes[1] = (unsigned char) (value & 0xFFu);
    fwrite(bytes, sizeof(bytes), 1, file);
}

static void say_write_u32_be(FILE *file, unsigned int value)
{
    unsigned char bytes[4];

    bytes[0] = (unsigned char) ((value >> 24) & 0xFFu);
    bytes[1] = (unsigned char) ((value >> 16) & 0xFFu);
    bytes[2] = (unsigned char) ((value >> 8) & 0xFFu);
    bytes[3] = (unsigned char) (value & 0xFFu);
    fwrite(bytes, sizeof(bytes), 1, file);
}

static void say_write_ieee_extended(FILE *file, double value)
{
    unsigned char bytes[10];
    unsigned int exponent;
    unsigned long long significand;
    int exp2;
    double fraction;
    double mantissa;
    long double scaled;
    size_t i;

    memset(bytes, 0, sizeof(bytes));

    if (value > 0.0) {
        fraction = frexp(value, &exp2);
        exponent = (unsigned int) (exp2 - 1 + 16383);
        mantissa = ldexp(fraction, 1);
        scaled = (long double) mantissa * (long double) (1ull << 63);
        significand = (unsigned long long) (scaled + 0.5L);

        if (significand == 0ull) {
            exponent = 0;
        }

        bytes[0] = (unsigned char) ((exponent >> 8) & 0x7F);
        bytes[1] = (unsigned char) (exponent & 0xFFu);
        for (i = 0; i < 8; ++i) {
            bytes[2 + i] = (unsigned char) ((significand >> (56 - 8 * i)) & 0xFFu);
        }
    }

    fwrite(bytes, sizeof(bytes), 1, file);
}

static int say_write_raw(
    const char *path,
    const int16_t *samples,
    size_t sample_count,
    char *error,
    size_t error_size
)
{
    FILE *file;

    file = fopen(path, "wb");
    if (file == NULL) {
        say_set_error(error, error_size, "unable to open %s: %s", path, strerror(errno));
        return 0;
    }

    if (sample_count > 0 && fwrite(samples, sizeof(*samples), sample_count, file) != sample_count) {
        fclose(file);
        say_set_error(error, error_size, "failed to write raw output to %s", path);
        return 0;
    }

    fclose(file);
    return 1;
}

static int say_write_aiff(
    const char *path,
    int sample_rate,
    const int16_t *samples,
    size_t sample_count,
    char *error,
    size_t error_size
)
{
    FILE *file;
    unsigned int sound_bytes;
    size_t i;

    if (sample_count > 0x7FFFFFFFu / 2u) {
        say_set_error(error, error_size, "sample buffer is too large for AIFF");
        return 0;
    }

    sound_bytes = (unsigned int) (sample_count * 2u);
    file = fopen(path, "wb");
    if (file == NULL) {
        say_set_error(error, error_size, "unable to open %s: %s", path, strerror(errno));
        return 0;
    }

    fwrite("FORM", 4, 1, file);
    say_write_u32_be(file, 46u + sound_bytes);
    fwrite("AIFF", 4, 1, file);

    fwrite("COMM", 4, 1, file);
    say_write_u32_be(file, 18u);
    say_write_u16_be(file, 1u);
    say_write_u32_be(file, (unsigned int) sample_count);
    say_write_u16_be(file, 16u);
    say_write_ieee_extended(file, (double) sample_rate);

    fwrite("SSND", 4, 1, file);
    say_write_u32_be(file, 8u + sound_bytes);
    say_write_u32_be(file, 0u);
    say_write_u32_be(file, 0u);

    for (i = 0; i < sample_count; ++i) {
        unsigned short value = (unsigned short) samples[i];
        unsigned char bytes[2];
        bytes[0] = (unsigned char) ((value >> 8) & 0xFFu);
        bytes[1] = (unsigned char) (value & 0xFFu);
        fwrite(bytes, sizeof(bytes), 1, file);
    }

    fclose(file);
    return 1;
}

void say_default_options(say_options_t *options)
{
    if (options == NULL) {
        return;
    }
    options->sample_rate = 22050;
    options->frame_ms = 10;
    options->language = SAY_LANG_EN;
    options->phoneme_input = 0;
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
    }
    return SAY_FORMAT_RAW;
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

    resolved_options = *options;
    if (resolved_options.sample_rate != 22050 && resolved_options.sample_rate != 44100) {
        say_set_error(error, error_size, "sample rate must be 22050 or 44100 Hz");
        return 0;
    }
    if (resolved_options.frame_ms < 5 || resolved_options.frame_ms > 10) {
        say_set_error(error, error_size, "frame size must be between 5 and 10 ms");
        return 0;
    }

    memset(&segments, 0, sizeof(segments));
    memset(&frames, 0, sizeof(frames));

    normalized = NULL;
    ok = 0;
    if (resolved_options.phoneme_input) {
        if (!say_parse_phoneme_input(input, &segments, error, error_size)) {
            goto cleanup;
        }
    }
    else {
        normalized = say_normalize_text(input, error, error_size);
        if (normalized == NULL) {
            goto cleanup;
        }
        if (!say_phonemize_text(normalized, resolved_options.language, &segments, error, error_size)) {
            goto cleanup;
        }
    }

    if (!say_generate_frames(segments.data, segments.count, &resolved_options, &frames, error, error_size)) {
        goto cleanup;
    }
    if (!say_synthesize_frames(frames.data, frames.count, resolved_options.sample_rate, resolved_options.frame_ms, out_samples, out_sample_count, error, error_size)) {
        goto cleanup;
    }

    ok = 1;

cleanup:
    free(normalized);
    free(segments.data);
    free(frames.data);
    return ok;
}

int say_write_audio_file(
    const char *path,
    say_audio_format_t format,
    int sample_rate,
    const int16_t *samples,
    size_t sample_count,
    char *error,
    size_t error_size
)
{
    if (path == NULL || path[0] == '\0') {
        say_set_error(error, error_size, "output path is required");
        return 0;
    }
    if (samples == NULL && sample_count != 0) {
        say_set_error(error, error_size, "sample buffer is null");
        return 0;
    }

    if (format == SAY_FORMAT_AIFF) {
        return say_write_aiff(path, sample_rate, samples, sample_count, error, error_size);
    }
    return say_write_raw(path, samples, sample_count, error, error_size);
}

void say_free(void *ptr)
{
    free(ptr);
}
