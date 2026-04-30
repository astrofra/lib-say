/*
 * say_synth_amiga.c — C port of the Amiga narrator's `synth.asm + syn.i`.
 *
 * The Amiga real-time formant synthesizer reads a coefficient buffer (8 bytes
 * per frame: F1_idx, F2_idx, F3_idx, AMP1, AMP2, AMP3, FLAGS, F0) and emits
 * 8-bit signed mono audio.  Each frame is subdivided into "microframes"
 * (= one inner-loop iteration per pitch period or fricative period — see
 * SAMPERFRAME calc).  Per pitch period, a 4-bit V/F/A/N steering value
 * (Voiced / Fricative / Aspirant / Nasal) selects one of 8 synthesis paths:
 *
 *   F123  — fully voiced (vowels) : F1 - F2 + F3 with reflecting period
 *   AN    — voicebar / syllabic-nasal : F1 only
 *   FF    — unvoiced fricative : ping-pong over a 512-sample noise record
 *   AH    — aspirant : like FF but the F3 (aspiration) record, attenuated 18 dB
 *   FFAH  — voiceless fric + asp : sum of FF + AH
 *   F1FF  — voiced fricative : F1 + FF, with -6 dB amplitude swap mid-period
 *   F1AH  — voiced aspirant : F1 + AH (attenuated)
 *   SIL   — silence
 *
 * Sample synthesis multiplies an LUT byte (5-bit signed sample) by a 5-bit
 * amplitude via the precomputed MULT[] table — `MULT[(amp << 5) | sample]`
 * returns the byte product, with two's-complement signed values folded into
 * the 32×32 grid.
 *
 * The original asm runs against AmigaOS audio DMA buffers; this port writes
 * 8-bit signed values into a host-grown int16_t PCM buffer (sample is
 * promoted via `(int8_t) byte << 8`). The natural rate is 22 200 Hz to match
 * the Amiga; the caller can request 22 050 Hz directly or upsample later.
 *
 * The terminator in the input buffer is the same as the Amiga's: a frame
 * whose first byte (F1 index) has its high bit set (0x80..0xFF). The real
 * narrator emits 0xFF.
 *
 * Phase 3 of the Amiga substrate port plan
 * (documentation/amiga-substrate-port-plan.md). Phase 5 will add the bridge
 * layer that fills the coefficient buffer from lib-say segments + F0 contour.
 */

#define _CRT_SECURE_NO_WARNINGS

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "say_amiga_compat.h"

/* The shared LUTs live in say_synth_tables.c (linked via say_amiga_substrate).
 * We declare the underlying arrays here as extern via accessors, but for tight
 * inner-loop code we want them at file scope.  Easiest: include the .inc
 * directly here so the compiler sees the static arrays inline. */
#include "say_synth_tables.inc"

/* Coefficient frame layout — matches synth.asm's offsets into A6. */
#define COEF_F1     0
#define COEF_F2     1
#define COEF_F3     2
#define COEF_AMP1   3
#define COEF_AMP2   4
#define COEF_AMP3   5
#define COEF_FLAGS  6
#define COEF_F0     7
#define COEF_BYTES  8

/* Bits in coef[FLAGS] — see synth.asm "Bits in the hi word of d4". */
#define FLAG_NASAL  (1u << 4)   /* bit 20 in d4's hi word, post-LSR #2 in steering */
#define FLAG_ASPIR  (1u << 5)   /* bit 21 */
#define FLAG_FRIC   (1u << 6)   /* bit 22 */
#define FLAG_VOICED (1u << 7)   /* bit 23 */

/* Internal record-end marker bit, lives only inside the synthesizer. */
#define REC_END_BIT 0x80000000u

typedef struct {
    /* Output. */
    int16_t *out;
    size_t   out_pos;
    size_t   out_capacity;

    /* Input. */
    const uint8_t *coef;
    size_t   frame_idx;

    /* Per-frame derived state. The substrate keeps a fixed "base" pointer per
     * formant (set when the frame is loaded — this is the start of the
     * impulse response in the LUT) and a working pointer that advances during
     * synthesis. At each pitch boundary the working pointer is restored to
     * the base, retriggering the formant burst — that's what gives voiced
     * speech its periodic structure (the glottal-pulse rate). The original
     * asm holds these in F1REC/F2REC/F3REC (save) vs a1/a2/a3 (working);
     * the FUbypass branch at synth.asm:367-372 restores them. */
    const uint8_t *f1_rec_base;
    const uint8_t *f2_rec_base;
    const uint8_t *f3_rec_base;
    const uint8_t *f1_rec;
    const uint8_t *f2_rec;
    const uint8_t *f3_rec;
    int      amp1;        /* low 5 bits = amp1, shifted up by 5 (matches asm `lsl.w #5,d1`) */
    int      amp2;        /* current half (gets swapped at half-pitch in F1FF) */
    int      amp2_alt;    /* the other half (-6 dB if voiced fricative, else equal) */
    int      amp3;
    int      pitch;       /* full pitch period in samples (low byte of coef[F0]) */
    int      half_pitch;  /* pitch / 2 */
    unsigned flags;       /* VOICED|FRIC|ASPIR|NASAL bits (FLAG_* above) */

    /* Reflecting indices for fricative / aspirant records. */
    int      fric_idx;    /* 0..511 */
    int      friclen;     /* counts from 511 down */
    int      frinc;       /* +1 or -1 */

    /* Formant record progress. */
    int      reclen;      /* counts from 127 down */
    unsigned rec_end;     /* nonzero once reclen hits 0 */

    /* Microframe tracking. */
    int      samperframe; /* samples per microframe (frame-rate / pitch granularity) */
    int      microframe;  /* counts from samperframe down */
    int      update;      /* set when microframe expires; triggers frame update */
} synth_state_t;

/* ---- Output writer ----------------------------------------------------- */

static int synth_emit_sample(synth_state_t *s, int sample_byte)
{
    int s8;
    int16_t value;
    if (s->out_pos >= s->out_capacity) {
        size_t new_cap = s->out_capacity ? s->out_capacity * 2 : 4096;
        int16_t *grown = (int16_t *) realloc(s->out, new_cap * sizeof(int16_t));
        if (grown == NULL) return 0;
        s->out = grown;
        s->out_capacity = new_cap;
    }
    /* sample_byte is two's-complement 8-bit (MULT-table products are signed
     * bytes plus a `(int8_t) sum` wraparound). Clamp to [-127, +127] before
     * promoting so the output range is symmetric — leaving -128 in place would
     * map to int16 -32768 with no positive counterpart, audible as a one-sided
     * spike on the loudest voiced fricative samples. */
    s8 = (int8_t) sample_byte;
    if (s8 < -127) s8 = -127;
    value = (int16_t) (s8 * 256);
    s->out[s->out_pos++] = value;
    return 1;
}

/* Pull the next 5-bit MULT-table product. Index = (amp_shifted) | sample.
 * MULT[index] is an int8 represented as uint8 (two's complement). */
static inline int mult_lookup(int amp_shifted, int sample_byte)
{
    int idx = amp_shifted | (sample_byte & 0xFF);
    /* amp_shifted is at most (31 << 5) = 0x3E0; sample is 0..0xFF.
     * Combined max is 0x3FF = 1023, exactly MULT[1024] - 1. */
    if (idx < 0) idx = 0;
    if (idx >= (int) sizeof(MULT)) idx = (int) sizeof(MULT) - 1;
    return (int8_t) MULT[idx];
}

/* ---- Per-sample inner-loop primitives ---------------------------------- */
/* Each handler synthesizes ONE sample and writes it. After `pitch` of these
 * for a routine, the outer loop advances to NewPulse for the next pitch
 * period (or frame). Returns 0 on out-of-memory. */

/* F123: voiced (vowels). F1 - F2 + F3, where the formant records are
 * 128-sample impulse responses; once exhausted (rec_end set) the residual
 * is silent. */
static int synth_F123(synth_state_t *s)
{
    int sample = 0;
    if (!s->rec_end) {
        int b1 = *s->f1_rec++;
        int b2 = *s->f2_rec++;
        int b3 = *s->f3_rec++;
        sample = mult_lookup(s->amp1, b1);
        sample -= mult_lookup(s->amp2, b2);
        sample += mult_lookup(s->amp3, b3);
        sample = (int8_t) sample;     /* emulate 68k byte-arithmetic wraparound */
    }
    return synth_emit_sample(s, sample);
}

/* AN: nasal / voicebar — F1 only. */
static int synth_AN(synth_state_t *s)
{
    int sample = 0;
    if (!s->rec_end) {
        int b1 = *s->f1_rec++;
        sample = mult_lookup(s->amp1, b1);
    }
    return synth_emit_sample(s, sample);
}

/* FF: unvoiced fricative — ping-pong index over the FRICtable record. */
static int synth_FF(synth_state_t *s)
{
    /* The original asm uses `0(a2,d5)` where a2 = base of FRICtable record
     * (already adjusted for which fric ID at frame load time) and d5 is the
     * reflecting index. We mirror that with f2_rec + fric_idx. */
    int b = s->f2_rec[s->fric_idx & 0x1FF];
    int sample = mult_lookup(s->amp2, b);
    return synth_emit_sample(s, sample);
}

/* AH: aspirant — like FF, but the f3 (aspiration) record, attenuated 18 dB
 * via arithmetic right-shift by 3. */
static int synth_AH(synth_state_t *s)
{
    int b = s->f3_rec[s->fric_idx & 0x1FF];
    int product = mult_lookup(s->amp3, b);
    product = product >> 3;            /* asr.b #3 — preserves sign of int8 */
    return synth_emit_sample(s, product);
}

/* FFAH: voiceless fricative + aspirant. */
static int synth_FFAH(synth_state_t *s)
{
    int b3 = s->f3_rec[s->fric_idx & 0x1FF];
    int b2 = s->f2_rec[s->fric_idx & 0x1FF];
    int sample = mult_lookup(s->amp3, b3) >> 3;   /* aspirant attenuated 18 dB */
    sample += mult_lookup(s->amp2, b2);            /* + fricative */
    sample = (int8_t) sample;
    return synth_emit_sample(s, sample);
}

/* F1FF: voiced fricative — F1 + FF. The amp2 swap at half-pitch is handled
 * in the pitch-period loop (see run_pitch_period). */
static int synth_F1FF(synth_state_t *s)
{
    int b2 = s->f2_rec[s->fric_idx & 0x1FF];
    int sample = mult_lookup(s->amp2, b2);
    if (!s->rec_end) {
        int b1 = *s->f1_rec++;
        sample += mult_lookup(s->amp1, b1);
    }
    sample = (int8_t) sample;
    return synth_emit_sample(s, sample);
}

/* F1AH: voiced aspirant — F1 + AH (aspirant attenuated 18 dB). */
static int synth_F1AH(synth_state_t *s)
{
    int b3 = s->f3_rec[s->fric_idx & 0x1FF];
    int sample = mult_lookup(s->amp3, b3) >> 3;
    if (!s->rec_end) {
        int b1 = *s->f1_rec++;
        sample += mult_lookup(s->amp1, b1);
    }
    sample = (int8_t) sample;
    return synth_emit_sample(s, sample);
}

/* SIL: silence. */
static int synth_SIL(synth_state_t *s)
{
    return synth_emit_sample(s, 0);
}

/* ---- Per-sample bookkeeping common to all routines --------------------- */

/* Decrement microframe / record counters after every emitted sample. */
static void synth_tick(synth_state_t *s, int uses_formant_rec, int uses_fric_idx)
{
    /* Microframe / frame-update counter. */
    if (--s->microframe == 0) {
        s->microframe = s->samperframe;
        ++s->update;
    }

    /* Formant record progress (only the routines that read F1 use this). */
    if (uses_formant_rec && !s->rec_end) {
        if (--s->reclen == 0) {
            s->rec_end = 1;
        }
    }

    /* Fricative / aspirant ping-pong. */
    if (uses_fric_idx) {
        s->fric_idx += s->frinc;
        if (--s->friclen == 0) {
            s->friclen = 511;
            s->frinc = -s->frinc;
        }
    }
}

/* Run one pitch period (`s->pitch` samples). Returns 0 on OOM. */
static int run_pitch_period(synth_state_t *s)
{
    int branch;     /* 0..15 = VFAN composite */
    int half_count;
    int amp2_save;

    /* Reset per-pitch-period state — equivalent to FUbypass in synth.asm:
     * working pointers restored to the saved start-of-record so the formant
     * impulse response retriggers, reclen/rec_end reset, fric/aspir indices
     * keep their reflecting state across periods (those use fric_idx). */
    s->f1_rec = s->f1_rec_base;
    s->f2_rec = s->f2_rec_base;
    s->f3_rec = s->f3_rec_base;
    s->reclen = 127;
    s->rec_end = 0;

    /* Decode V/F/A/N steering. The asm computes the index from d4's hi word:
     *   bit  7 = VOICED, 6 = FRIC, 5 = ASPIR, 4 = NASAL.
     * Branch table values (from synth.asm comments):
     *   1000  V          → F123
     *   1001  V    N     → AN          (voiced nasal — F1 only with voicebar)
     *   1010  V  A       → F1AH
     *   1011  V  AN      → SIL
     *   1100  VF         → F1FF
     *   1101  VF N       → F1FF        (voiced fric, ignore nasal flag)
     *   1110  VFA        → SIL
     *   1111  VFAN       → SIL
     *   0010    A        → AH          (aspirant, no voice)
     *   0100   F         → FF          (fricative, no voice)
     *   0110   FA        → FFAH
     *   anything else    → SIL
     */
    branch = ((s->flags & FLAG_VOICED) ? 8 : 0)
           | ((s->flags & FLAG_FRIC)   ? 4 : 0)
           | ((s->flags & FLAG_ASPIR)  ? 2 : 0)
           | ((s->flags & FLAG_NASAL)  ? 1 : 0);

    /* For F1FF the asm pre-swaps amp2 with its alt (the -6 dB version) and
     * swaps back at half-pitch. We replicate by tracking a save and a
     * countdown half_count. */
    half_count = s->half_pitch;
    amp2_save = s->amp2;

    {
        int p;
        for (p = 0; p < s->pitch; ++p) {
            int ok;
            switch (branch) {
                case 0x8:  ok = synth_F123(s);  synth_tick(s, 1, 0); break;
                case 0x9:  ok = synth_AN(s);    synth_tick(s, 1, 0); break;
                case 0xA:  ok = synth_F1AH(s);  synth_tick(s, 1, 1); break;
                case 0xC:  ok = synth_F1FF(s);  synth_tick(s, 1, 1); break;
                case 0xD:  ok = synth_F1FF(s);  synth_tick(s, 1, 1); break;
                case 0x2:  ok = synth_AH(s);    synth_tick(s, 0, 1); break;
                case 0x4:  ok = synth_FF(s);    synth_tick(s, 0, 1); break;
                case 0x6:  ok = synth_FFAH(s);  synth_tick(s, 0, 1); break;
                default:   ok = synth_SIL(s);   synth_tick(s, 0, 0); break;
            }
            if (!ok) return 0;

            /* Voiced-fricative amp2 swap at half-pitch. The asm does
             * `swap d2; subq.w #1,d0; bne.s ...; swap d2; ...`. */
            if (branch == 0xC || branch == 0xD) {
                if (--half_count == 0) {
                    int tmp = s->amp2;
                    s->amp2 = s->amp2_alt;
                    s->amp2_alt = tmp;
                }
            }
        }
    }

    /* Restore amp2 (in case we swapped). */
    s->amp2 = amp2_save;
    return 1;
}

/* ---- Frame load -------------------------------------------------------- */

/* Returns 1 if a frame was loaded (synthesis continues), 0 if EOF reached. */
static int load_frame(synth_state_t *s)
{
    const uint8_t *frame;
    int f1_idx, f2_idx, f3_idx, a1, a2, a3, flags, f0;

    frame = s->coef + s->frame_idx * COEF_BYTES;
    f1_idx = frame[COEF_F1];

    /* Terminator: F1 index with high bit set (0xFF in the original). */
    if (f1_idx & 0x80) return 0;

    f2_idx = frame[COEF_F2];
    f3_idx = frame[COEF_F3];
    a1     = frame[COEF_AMP1];
    a2     = frame[COEF_AMP2];
    a3     = frame[COEF_AMP3];
    flags  = frame[COEF_FLAGS];
    f0     = frame[COEF_F0];

    /* F1 record: 128-sample impulse response, base = F1table + idx*128. */
    s->f1_rec_base = F1table + (f1_idx * 128);

    /* F2: voiced uses F2table[idx*128]; fricative uses FRICtable[idx*64]
     * (the asm shifts left by 6, since CONVERT pre-shifted by 3). */
    if (flags & FLAG_FRIC) {
        s->f2_rec_base = FRICtable + (f2_idx * 64);
    } else {
        s->f2_rec_base = F2table + (f2_idx * 128);
    }

    /* F3: same pattern with ASPIR. */
    if (flags & FLAG_ASPIR) {
        s->f3_rec_base = FRICtable + (f3_idx * 64);
    } else {
        s->f3_rec_base = F3table + (f3_idx * 128);
    }
    s->f1_rec = s->f1_rec_base;
    s->f2_rec = s->f2_rec_base;
    s->f3_rec = s->f3_rec_base;

    /* Amplitudes: shifted up by 5 to form the high bits of the MULT-table
     * index. Voiced fricatives also halve amp2 for the second half of the
     * pitch period (-6 dB) — store both. */
    s->amp1 = (a1 & 0x1F) << 5;
    s->amp2 = (a2 & 0x1F) << 5;
    s->amp2_alt = ((a2 >> 1) & 0x1F) << 5;   /* -6 dB version */
    if (!((flags & FLAG_FRIC) && (flags & FLAG_VOICED))) {
        /* Non-voiced-fric: keep amp2 constant across the pitch period. */
        s->amp2_alt = s->amp2;
    }
    s->amp3 = (a3 & 0x1F) << 5;

    s->flags = (unsigned) flags;
    s->pitch = f0 ? f0 : 1;       /* avoid zero-period infinite loop */
    s->half_pitch = s->pitch / 2;
    if (s->half_pitch == 0) s->half_pitch = 1;

    s->frame_idx++;
    return 1;
}

/* Compute samples-per-microframe at a given sample rate and speaking rate. */
static int compute_samperframe(int sample_rate, int rate_wpm)
{
    /* Original: samperframe = (sampfreq * 75) / rate / 60.
     * The asm then `lsr.w #1,d1` to halve (because the buffer is in words);
     * we synthesize one sample per inner iteration so we don't divide by 2. */
    long n = (long) sample_rate * 75L;
    n /= (long) rate_wpm;
    n /= 60L;
    if (n < 1) n = 1;
    if (n > 0x7FFF) n = 0x7FFF;
    return (int) n;
}

/* ---- Public entry ------------------------------------------------------ */

/* P5 — bridge entry point. Lets the caller pin samperframe explicitly so it
 * can match an external frame cadence (lib-say emits parameter frames every
 * 5..10 ms). When samperframe_override <= 0 we keep the legacy WPM-derived
 * value. The original say_synth_amiga_run is now a thin wrapper. */
int say_synth_amiga_run_ex(
    const uint8_t *coef,
    int sample_rate,
    int rate_wpm,
    int samperframe_override,
    int16_t **out_samples,
    size_t *out_sample_count
)
{
    synth_state_t s;
    memset(&s, 0, sizeof(s));

    if (coef == NULL || out_samples == NULL || out_sample_count == NULL) return 0;
    if (sample_rate <= 0) return 0;
    if (samperframe_override <= 0 && rate_wpm <= 0) return 0;

    s.coef = coef;
    s.samperframe = samperframe_override > 0
                  ? samperframe_override
                  : compute_samperframe(sample_rate, rate_wpm);
    s.microframe  = s.samperframe;
    s.friclen     = 511;
    s.frinc       = +1;
    s.fric_idx    = 0;
    s.update      = 1;            /* force frame load on first iteration */
    s.reclen      = 127;
    s.rec_end     = 0;
    /* Initial f1/f2/f3 pointers — replaced as soon as load_frame runs. */
    s.f1_rec = F1table;
    s.f2_rec = F2table;
    s.f3_rec = F3table;

    /* Outer pitch-period loop. `update` counts how many microframes (each
     * SAMPERFRAME samples long) elapsed during the previous pitch period.
     * The original asm at NewPulse (synth.asm:293-312) advances the coef
     * pointer once per microframe wrap and only writes the latched F1/F2/F3
     * records on the LAST iteration — i.e. it skips past intermediate frames
     * to keep the audio clock locked to samperframe rather than pitch period.
     * We replicate by calling load_frame `update` times before synthesizing
     * the next period. */
    {
    int frames_consumed = 0;
    int pitch_periods = 0;
    int loaded_once = 0;
    for (;;) {
        /* synth.asm:293-312 — at each pitch boundary, if UPDATE>0 then advance
         * UPDATE coef entries past the buffer pointer (skipping intermediate
         * frames) and load the last one as the new IR set. UPDATE=0 means
         * "reuse the latched IR" — FUbypass falls straight to synthesis. The
         * very first iteration is forced (`UPDATE=1` initial seed) so we load
         * the opening frame. */
        int reload_count = loaded_once ? s.update : 1;
        int loaded_any = 0;
        s.update = 0;
        while (reload_count-- > 0) {
            if (!load_frame(&s)) {
                if (loaded_any) goto render;
                goto done;
            }
            loaded_any = 1;
            loaded_once = 1;
            ++frames_consumed;
        }
render:
        ++pitch_periods;
        if (!run_pitch_period(&s)) {
            free(s.out);
            return 0;
        }
    }
done:
    if (getenv("SAY_AMIGA_DEBUG")) {
        fprintf(stderr, "[amiga-synth] frames_consumed=%d pitch_periods=%d emitted=%zu samperframe=%d\n",
                frames_consumed, pitch_periods, s.out_pos, s.samperframe);
    }
    }

    *out_samples = s.out;
    *out_sample_count = s.out_pos;
    return 1;
}

int say_synth_amiga_run(
    const uint8_t *coef,
    int sample_rate,
    int rate_wpm,
    int16_t **out_samples,
    size_t *out_sample_count
)
{
    return say_synth_amiga_run_ex(coef, sample_rate, rate_wpm, /*samperframe_override*/ 0,
                                  out_samples, out_sample_count);
}
