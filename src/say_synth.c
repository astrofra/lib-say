#define _CRT_SECURE_NO_WARNINGS

#include "say_internal.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---------------------------------------------------------------------------
 * Glottal pulse — a three-piece analytic shape used as the voiced excitation.
 * Phase is normalized to [0, 1] across one pitch period.
 * ------------------------------------------------------------------------- */

static double say_glottal_pulse(double phase)
{
    double flow;

    if (phase < 0.46) {
        double x = phase / 0.46;
        flow = sin(0.5 * M_PI * x);
        flow *= flow;
    }
    else if (phase < 0.74) {
        double x = (phase - 0.46) / 0.28;
        flow = cos(x * (M_PI * 0.5));
    }
    else {
        double x = (phase - 0.74) / 0.26;
        flow = -0.12 * sin(M_PI * x);
    }

    return flow;
}

/* ---------------------------------------------------------------------------
 * Biquad filters (RBJ cookbook) — bandpass for formant resonators, HP / LP for
 * the dedicated noise path.
 * ------------------------------------------------------------------------- */

typedef struct biquad_t {
    double b0;
    double b1;
    double b2;
    double a1;
    double a2;
    double z1;
    double z2;
} biquad_t;

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

static void say_biquad_set_highpass(biquad_t *filter, double sample_rate, double frequency)
{
    double w0, sin_w0, cos_w0, alpha, a0;

    if (frequency < 20.0)                       frequency = 20.0;
    if (frequency > sample_rate * 0.45)         frequency = sample_rate * 0.45;

    w0     = 2.0 * M_PI * frequency / sample_rate;
    sin_w0 = sin(w0);
    cos_w0 = cos(w0);
    alpha  = sin_w0 / (2.0 * 0.7071);
    a0     = 1.0 + alpha;

    filter->b0 =  (1.0 + cos_w0) / (2.0 * a0);
    filter->b1 = -(1.0 + cos_w0) / a0;
    filter->b2 =  (1.0 + cos_w0) / (2.0 * a0);
    filter->a1 = -2.0 * cos_w0 / a0;
    filter->a2 = (1.0 - alpha) / a0;
}

static void say_biquad_set_lowpass(biquad_t *filter, double sample_rate, double frequency)
{
    double w0, sin_w0, cos_w0, alpha, a0;

    if (frequency < 20.0)                       frequency = 20.0;
    if (frequency > sample_rate * 0.45)         frequency = sample_rate * 0.45;

    w0     = 2.0 * M_PI * frequency / sample_rate;
    sin_w0 = sin(w0);
    cos_w0 = cos(w0);
    alpha  = sin_w0 / (2.0 * 0.7071);
    a0     = 1.0 + alpha;

    filter->b0 = (1.0 - cos_w0) / (2.0 * a0);
    filter->b1 = (1.0 - cos_w0) / a0;
    filter->b2 = (1.0 - cos_w0) / (2.0 * a0);
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

/* ---------------------------------------------------------------------------
 * Sample-level synthesis loop. Consumes a stream of frames and produces 16-bit
 * PCM. Per frame: smooth state interpolation, pitch jitter, voiced + noise
 * excitation, parallel formant biquad bank, optional dedicated HP+LP noise
 * path, voicing-bar low-pass, soft-clip and DC-blocking high pass, 0.34
 * threshold limiter, peak-normalize, tanh shaper, write int16.
 * ------------------------------------------------------------------------- */

int say_synthesize_frames(
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
    biquad_t voiced_filters[SAY_MAX_FORMANTS];
    biquad_t noise_filters[SAY_MAX_FORMANTS];
    size_t i;
    int64_t frame_sample_accum;
    size_t total_samples;
    double *mix;
    int16_t *samples;
    size_t sample_index;
    double phase;
    unsigned int rng;
    double amplitude_state;
    double pitch_state;
    double noise_mix_state;
    double voicing_state;
    double formant_freq_state[SAY_MAX_FORMANTS];
    double bandwidth_state[SAY_MAX_FORMANTS];
    double gain_state[SAY_MAX_FORMANTS];
    double source_state;
    double source_hp_x1;
    double source_hp_y1;
    double source_tilt_state;     /* Spectral tilt LP — emulates the natural −6 dB/oct
                                   * roll-off of the glottal source above ~600 Hz, before
                                   * the signal enters the formant bank. Reduces the
                                   * "buzzy" overtone energy on high formants. */
    double noise_hp1_x1;
    double noise_hp1_y1;
    double noise_hp2_x1;
    double noise_hp2_y1;
    double noise_soft_state;
    double hp_x1;
    double hp_y1;
    double limiter_env;
    double peak;
    double jitter_state;
    double jitter_target;
    int jitter_countdown;
    int state_ready;
    biquad_t noise_path_hp;
    biquad_t noise_path_lp;
    double np_mix_state;
    double np_f_low_state;
    double np_f_high_state;
    double np_gain_state;
    double vbar_lp_state;
    double vbar_amp_state;

    memset(voiced_filters, 0, sizeof(voiced_filters));
    memset(noise_filters, 0, sizeof(noise_filters));
    frame_sample_accum = 0;
    total_samples = 0;
    for (i = 0; i < frame_count; ++i) {
        int frame_samples;
        frame_sample_accum += (int64_t) sample_rate * (int64_t) frame_ms;
        frame_samples = (int) (frame_sample_accum / 1000);
        frame_sample_accum %= 1000;
        total_samples += (size_t) frame_samples;
    }

    mix = (double *) malloc(total_samples * sizeof(*mix));
    if (mix == NULL) {
        say_set_error(error, error_size, "out of memory while allocating %zu synthesis samples", total_samples);
        return 0;
    }

    phase = 0.0;
    rng = 0x12345678u;
    sample_index = 0;
    frame_sample_accum = 0;
    amplitude_state = 0.0;
    pitch_state = 120.0;
    noise_mix_state = 0.0;
    voicing_state = 0.0;
    source_state = 0.0;
    source_hp_x1 = 0.0;
    source_hp_y1 = 0.0;
    source_tilt_state = 0.0;
    noise_hp1_x1 = 0.0;
    noise_hp1_y1 = 0.0;
    noise_hp2_x1 = 0.0;
    noise_hp2_y1 = 0.0;
    noise_soft_state = 0.0;
    hp_x1 = 0.0;
    hp_y1 = 0.0;
    limiter_env = 0.0;
    peak = 0.0;
    jitter_state = 0.0;
    jitter_target = 0.0;
    jitter_countdown = 0;
    state_ready = 0;
    memset(formant_freq_state, 0, sizeof(formant_freq_state));
    memset(bandwidth_state, 0, sizeof(bandwidth_state));
    memset(gain_state, 0, sizeof(gain_state));
    memset(&noise_path_hp, 0, sizeof(noise_path_hp));
    memset(&noise_path_lp, 0, sizeof(noise_path_lp));
    np_mix_state  = 0.0;
    np_f_low_state  = 1000.0;
    np_f_high_state = 4000.0;
    np_gain_state = 0.0;
    vbar_lp_state = 0.0;
    vbar_amp_state = 0.0;

    for (i = 0; i < frame_count; ++i) {
        size_t j;
        int frame_samples;
        double target_amplitude;
        double target_pitch;
        double target_noise_mix;
        double target_voicing;

        target_amplitude = frames[i].is_pause ? 0.0 : frames[i].amplitude;
        target_pitch = frames[i].pitch_hz > 1.0 ? frames[i].pitch_hz : pitch_state;
        target_noise_mix = frames[i].is_pause ? 0.0 : frames[i].noise_mix;
        target_voicing = frames[i].voiced ? 1.0 : 0.0;

        if (!state_ready) {
            for (j = 0; j < SAY_MAX_FORMANTS; ++j) {
                formant_freq_state[j] = frames[i].formant_freq[j];
                bandwidth_state[j] = frames[i].bandwidth[j];
                gain_state[j] = frames[i].gain[j];
                say_biquad_set_bandpass(&voiced_filters[j], (double) sample_rate, formant_freq_state[j], bandwidth_state[j], gain_state[j]);
                say_biquad_set_bandpass(&noise_filters[j], (double) sample_rate, formant_freq_state[j], bandwidth_state[j], gain_state[j]);
            }
            amplitude_state = target_amplitude;
            pitch_state = target_pitch;
            noise_mix_state = target_noise_mix;
            voicing_state = target_voicing;
            np_mix_state  = frames[i].noise_path_mix;
            if (frames[i].noise_path_f_low  > 0.0) np_f_low_state  = frames[i].noise_path_f_low;
            if (frames[i].noise_path_f_high > np_f_low_state) np_f_high_state = frames[i].noise_path_f_high;
            np_gain_state = frames[i].noise_path_gain;
            vbar_amp_state = frames[i].voicing_bar_amp;
            state_ready = 1;
        }

        frame_sample_accum += (int64_t) sample_rate * (int64_t) frame_ms;
        frame_samples = (int) (frame_sample_accum / 1000);
        frame_sample_accum %= 1000;

        for (j = 0; j < (size_t) frame_samples; ++j) {
            double glottal;
            double noise;
            double excitation;
            double voiced_excitation;
            double noise_excitation;
            double noise_raw;
            double voiced_mix;
            double voiced_output;
            double noise_output;
            double noise_path_out;
            double voicing_bar_out;
            double output;
            double source_hp;
            double noise_hp1;
            double noise_hp2;
            double noise_soft;
            double amplitude_rate;
            double noise_rate;
            double voicing_rate;
            size_t k;

            amplitude_rate = target_amplitude > amplitude_state ? 0.007 : 0.030;
            noise_rate = target_noise_mix > noise_mix_state ? 0.010 : 0.040;
            voicing_rate = target_voicing > voicing_state ? 0.014 : 0.050;

            amplitude_state += amplitude_rate * (target_amplitude - amplitude_state);
            pitch_state += 0.012 * (target_pitch - pitch_state);
            noise_mix_state += noise_rate * (target_noise_mix - noise_mix_state);
            voicing_state += voicing_rate * (target_voicing - voicing_state);

            np_mix_state += 0.012 * (frames[i].noise_path_mix - np_mix_state);
            np_gain_state += 0.020 * (frames[i].noise_path_gain - np_gain_state);
            vbar_amp_state += 0.020 * (frames[i].voicing_bar_amp - vbar_amp_state);
            if (frames[i].noise_path_f_low > 0.0) {
                np_f_low_state  += 0.020 * (frames[i].noise_path_f_low  - np_f_low_state);
                np_f_high_state += 0.020 * (frames[i].noise_path_f_high - np_f_high_state);
            }

            for (k = 0; k < SAY_MAX_FORMANTS; ++k) {
                double noise_focus;
                double sibilant_focus;
                double noise_gain_mul;
                double noise_bandwidth;

                formant_freq_state[k] += 0.025 * (frames[i].formant_freq[k] - formant_freq_state[k]);
                bandwidth_state[k] += 0.025 * (frames[i].bandwidth[k] - bandwidth_state[k]);
                gain_state[k] += 0.025 * (frames[i].gain[k] - gain_state[k]);

                noise_focus = say_clamp01((frames[i].noise_mix - 0.14) / 0.26);
                sibilant_focus = say_clamp01((frames[i].noise_mix - 0.24) / 0.18);
                switch (k) {
                    case 0:
                        noise_gain_mul = 1.0 - 0.86 * noise_focus;
                        break;
                    case 1:
                        noise_gain_mul = 1.0 - 0.64 * noise_focus;
                        break;
                    case 2:
                        noise_gain_mul = 1.0 - 0.12 * noise_focus;
                        break;
                    case 3:
                        noise_gain_mul = 1.0 + 0.16 * noise_focus;
                        break;
                    default:
                        noise_gain_mul = 1.0 + 0.28 * noise_focus;
                        break;
                }
                if (k < 2) {
                    noise_gain_mul *= 1.0 - 0.16 * sibilant_focus;
                }
                else if (k >= 3) {
                    noise_gain_mul *= 1.0 + 0.12 * sibilant_focus;
                }

                noise_bandwidth = bandwidth_state[k] * (1.10 + 0.26 * noise_focus);
                if (k < 2) {
                    noise_bandwidth *= 1.12 + 0.10 * sibilant_focus;
                }
                else if (k >= 3) {
                    noise_bandwidth *= 1.04 + 0.08 * sibilant_focus;
                }

                say_biquad_set_bandpass(&voiced_filters[k], (double) sample_rate, formant_freq_state[k], bandwidth_state[k], gain_state[k]);
                say_biquad_set_bandpass(&noise_filters[k], (double) sample_rate, formant_freq_state[k], noise_bandwidth, gain_state[k] * noise_gain_mul);
            }
            if (np_mix_state > 0.001) {
                say_biquad_set_highpass(&noise_path_hp, (double) sample_rate, np_f_low_state);
                say_biquad_set_lowpass (&noise_path_lp, (double) sample_rate, np_f_high_state);
            }

            if (voicing_state > 0.02 && pitch_state > 1.0) {
                if (jitter_countdown <= 0) {
                    rng = rng * 1664525u + 1013904223u;
                    jitter_target = ((((double) ((rng >> 8) & 0xFFFFu) / 32768.0) - 1.0) * 0.0045);
                    jitter_countdown = sample_rate / 180;
                    if (jitter_countdown < 1) {
                        jitter_countdown = 1;
                    }
                }
                --jitter_countdown;
                jitter_state += 0.010 * (jitter_target - jitter_state);
                phase += (pitch_state * (1.0 + jitter_state)) / (double) sample_rate;
                if (phase >= 1.0) {
                    phase -= floor(phase);
                }
                glottal = say_glottal_pulse(phase);
            }
            else {
                glottal = 0.0;
                jitter_state *= 0.98;
            }

            rng = rng * 1664525u + 1013904223u;
            noise = ((double) ((rng >> 8) & 0x00FFFFFFu) / 8388608.0) - 1.0;

            voiced_mix = voicing_state * (1.0 - noise_mix_state);
            voiced_excitation = voiced_mix * glottal;
            source_hp = voiced_excitation - source_hp_x1 + 0.972 * source_hp_y1;
            source_hp_x1 = voiced_excitation;
            source_hp_y1 = source_hp;
            source_state += 0.16 * (source_hp - source_state);
            voiced_excitation = 0.80 * source_state + 0.20 * source_hp;
            /* Spectral tilt — one-pole LP at fc = 0.085 × fs / 2π ≈ 596 Hz.
             * Stacks with source_state's existing −6 dB/oct shaping above ~1124 Hz
             * to give roughly −12 dB/oct above 1.1 kHz, matching the natural
             * glottal source roll-off and damping the high-formant buzz. */
            source_tilt_state += 0.085 * (voiced_excitation - source_tilt_state);
            voiced_excitation = source_tilt_state;

            noise_hp1 = noise - noise_hp1_x1 + 0.985 * noise_hp1_y1;
            noise_hp1_x1 = noise;
            noise_hp1_y1 = noise_hp1;
            noise_hp2 = noise_hp1 - noise_hp2_x1 + 0.940 * noise_hp2_y1;
            noise_hp2_x1 = noise_hp1;
            noise_hp2_y1 = noise_hp2;
            noise_soft_state += 0.18 * (noise_hp2 - noise_soft_state);
            noise_soft = noise_soft_state;
            noise_raw = 0.44 * noise_hp1 + 0.26 * noise_hp2 + 0.30 * noise_soft;
            noise_excitation = noise_mix_state * noise_raw;

            voiced_output = 0.0;
            noise_output = 0.0;
            for (k = 0; k < SAY_MAX_FORMANTS; ++k) {
                voiced_output += say_biquad_process(&voiced_filters[k], voiced_excitation);
                noise_output += say_biquad_process(&noise_filters[k], noise_excitation);
            }

            /* Dedicated noise path: bandpass-shaped white noise bypassing formant filters */
            if (np_mix_state > 0.001) {
                double hp_out = say_biquad_process(&noise_path_hp, noise_raw);
                noise_path_out = say_biquad_process(&noise_path_lp, hp_out) * np_gain_state;
            } else {
                noise_path_out = 0.0;
            }

            /* Voicing bar: LP-filtered glottal source below ~250 Hz */
            vbar_lp_state += 0.072 * (glottal * voicing_state - vbar_lp_state);
            voicing_bar_out = vbar_lp_state * vbar_amp_state;

            excitation = voiced_excitation + noise_excitation;
            output = voiced_output
                   + (1.0 - np_mix_state) * noise_output
                   + np_mix_state * noise_path_out
                   + voicing_bar_out;

            if (frames[i].is_pause) {
                output *= 0.08;
                for (k = 0; k < SAY_MAX_FORMANTS; ++k) {
                    voiced_filters[k].z1 *= 0.84;
                    voiced_filters[k].z2 *= 0.84;
                    noise_filters[k].z1 *= 0.84;
                    noise_filters[k].z2 *= 0.84;
                }
                noise_path_hp.z1 *= 0.84;
                noise_path_hp.z2 *= 0.84;
                noise_path_lp.z1 *= 0.84;
                noise_path_lp.z2 *= 0.84;
                vbar_lp_state *= 0.84;
                source_tilt_state *= 0.84;
            }

            output *= amplitude_state * 0.86;
            {
                double hp_output = output - hp_x1 + 0.995 * hp_y1;
                hp_x1 = output;
                hp_y1 = hp_output;
                output = hp_output;
            }
            {
                double abs_output = fabs(output);
                double threshold = 0.34;
                double release = 0.9985;

                if (abs_output > limiter_env) {
                    limiter_env = abs_output;
                }
                else {
                    limiter_env *= release;
                }

                if (limiter_env > threshold) {
                    output *= threshold / limiter_env;
                }
            }
            (void) excitation;
            mix[sample_index++] = output;
            if (fabs(output) > peak) {
                peak = fabs(output);
            }
        }
    }

    samples = (int16_t *) malloc(total_samples * sizeof(*samples));
    if (samples == NULL) {
        free(mix);
        say_set_error(error, error_size, "out of memory while allocating %zu pcm samples", total_samples);
        return 0;
    }

    if (peak < 1e-6) {
        peak = 1.0;
    }

    for (i = 0; i < sample_index; ++i) {
        double scaled = mix[i] * (0.82 / peak);
        double shaped = tanh(scaled * 1.15) * 0.96;
        if (shaped > 0.98) {
            shaped = 0.98;
        }
        if (shaped < -0.98) {
            shaped = -0.98;
        }
        samples[i] = (int16_t) lrint(shaped * 32767.0);
    }

    free(mix);
    *out_samples = samples;
    *out_sample_count = sample_index;
    return 1;
}
