# Architecture Rewrite: Fricative and Affricate Synthesis

## Problem Statement

The phoneme extractor score for English averages **24.48%** across nine test sentences, compared to a MaryTTS reference of **94–96%**. The gap is not uniform. Vowels and nasals are largely detected (90–100% family coverage). The failure concentrates in three phoneme classes:

| Family | Current coverage | MaryTTS coverage |
|---|---:|---:|
| Affricates (CH, JH) | 0% | 100% |
| Dental fricatives (TH, DH) | 0–28% | 100% |
| Voiced fricatives (V, DH, ZH) | 0–33% | 100% |
| Sibilants (S, Z, SH, ZH) | 50–100% | 100% |

The three lowest-scoring sentences — `06-affricates` (15%), `08-finals` (19%), `07-stress` (19%) — are dominated by these failing classes. Improving them is the highest-leverage architectural change available.

---

## Current Architecture and Its Limits

The synthesis engine in `src/say.c` uses a **five-formant biquad bandpass filter bank** shared between the voiced source and the noise source. A single scalar `noise_mix` controls the voiced/noise blend. Two derived values modulate gain per formant band:

```
noise_focus   = clamp01((noise_mix - 0.14) / 0.26)
sibilant_focus = clamp01((noise_mix - 0.24) / 0.18)
```

This design works well for vowels (noise_mix ≈ 0) and clean unvoiced sibilants like S (noise_mix ≈ 0.44, sibilant_focus = 1). It fails for the three problem classes for different but related reasons:

### Why affricates fail (CH, JH)

CH is acoustically a rapid stop closure followed by a SH-like fricative burst. JH is the voiced equivalent. The current model synthesizes CH/JH as **steady-state** phonemes — the same formants and noise mix throughout the entire alpha envelope. There is no closure phase, no burst transient, and no transition into a fricative. The extractor hears a flat, sustained frication that resembles neither T nor SH, and emits nothing recognizable.

Score in `06-affricates`: 15%. Affricate family coverage: 0 out of 6 expected tokens detected.

### Why dental fricatives fail (TH, DH)

In natural speech, TH/DH produce **diffuse, wideband noise** caused by turbulence at the tooth gap. There are no strong formant resonances — the spectrum is essentially flat from 1–8 kHz, with a gentle roll-off. The current model drives TH through the formant filter bank at frequencies 900, 2000, 3800, 6000, 8000 Hz with strong gain at F4/F5. This produces a noise pattern with resonant peaks that the extractor does not recognize as TH. The phoneme is either decoded as silence, or as the most acoustically similar resonant fricative (often labeled `f`, `s`, or `ax`).

Additionally, the `noise_mix = 0.22` used for TH sits just below the `sibilant_focus` threshold at 0.24. Raising it above 0.24 activates sibilant boost on TH's 8000 Hz formant, shifting the spectrum toward S-like character — verified experimentally: noise_mix 0.22→0.26 caused a −11% regression in `04-dentals`.

TH family coverage in `09-th-only`: 2 out of 7 expected tokens detected (28.57%). The extractor stays in blank state through the first 1.25 seconds of a TH-dense sentence.

### Why voiced fricatives fail (V, DH, ZH)

Voiced fricatives need two simultaneous acoustic components:
1. A **low-frequency voicing bar** — periodic energy from glottal vibration at F0 (~130 Hz), present below 300 Hz.
2. **Mid-to-high frequency friction** — aperiodic noise energy in the fricative's characteristic range.

The current formant bank routes both the voiced source and the noise source through the same filters. For V (F1 = 900 Hz, gain 0.44), the voiced component dominates the low formants and makes the sound vowel-like. Noise_mix = 0.28 gives only a modest noise contribution that cannot overcome the tonal F1. The extractor labels V as a vowel variant.

Voiced fricative coverage: 0% in `01-demo`, `02-hamlet`, `04-dentals`, `06-affricates`, `08-finals`; 16–33% in samples where V or Z appears in favorable phonetic context.

---

## Proposed Architecture Changes

### 1. Affricate Time-Variant Model

**Priority: Highest.** Fixes 0% affricate coverage without risk to other phoneme classes.

Replace the steady-state synthesis of CH/JH with a **three-phase alpha envelope**:

| Phase | Alpha range | Model |
|---|---|---|
| Closure | 0.00 → 0.30 | Silence (voiced = 0, noise at near-zero level), like T/D closure |
| Burst | 0.30 → 0.48 | High-amplitude broadband noise burst; voiced = 0 for CH, voiced = 1 for JH |
| Fricative | 0.48 → 1.00 | SH formants + noise for CH; ZH formants + noise for JH |

Implementation approach: in the synthesis loop, when the current phoneme is CH or JH, remap the alpha value to select among three internal states. The closure state uses the existing plosive closure logic. The burst state uses a flat broadband noise at high amplitude (noise_mix ≈ 0.60, gain ≈ 1.4). The fricative state adopts SH or ZH parameters from the phoneme table.

The phoneme table entries for CH and JH should be redesigned as **composite phonemes** pointing to their target fricative (SH for CH, ZH for JH) for the steady-state phase. A new boolean flag `is_affricate` in the phoneme table would activate the three-phase synthesis.

This requires approximately 30–50 lines of new code in the synthesis loop and two new phoneme table fields. No changes to the filter architecture are required.

### 2. Dedicated Parallel Noise Path

**Priority: High.** Decouples noise spectral shaping from the formant filter bank. Required for dental fricatives and useful for all fricatives.

Add a **secondary noise synthesis path** that runs in parallel with the formant filter bank. The primary path (current) handles voiced source + formant-filtered noise. The secondary path synthesizes noise through a **dedicated spectral shaper** — a simple one- or two-pole Butterworth filter applied to white noise, characterized by four parameters per phoneme:

```c
struct phoneme_noise_spec {
    double gain;       // overall noise level (0..1)
    double f_low;      // lower -3dB frequency (Hz)
    double f_high;     // upper -3dB frequency (Hz)
    double slope;      // spectral tilt (dB/octave, 0 = flat, negative = roll-off)
};
```

Add this structure to the phoneme table alongside the existing formant entries. The synthesis loop computes the secondary noise signal and mixes it with the formant output at a blend factor controlled by a new `noise_path_mix` parameter (0 = all primary, 1 = all secondary).

For vowels and nasals: `noise_path_mix = 0` (no change, backwards compatible).

For fricatives: `noise_path_mix = 0.5..1.0`, moving noise generation from the formant filters into the dedicated path.

This replaces the `noise_focus`/`sibilant_focus` chain for fricative phonemes. The per-phoneme `{gain, f_low, f_high, slope}` gives direct, acoustic-linguistically meaningful control over each fricative's noise spectrum.

**Recommended noise spectra** for key phonemes (based on acoustic phonetics):

| Phoneme | f_low | f_high | slope | Notes |
|---|---:|---:|---:|---|
| TH | 1000 | 8000 | −3 | Wideband, no peaks, gentle HF roll-off |
| DH | 800 | 6000 | −4 | Wider than DH, lower cut due to voicing |
| S | 4000 | 10000 | 0 | Sharp onset, high-frequency concentrated |
| SH | 2500 | 8000 | −2 | Broader than S, lower center |
| F | 1500 | 6000 | −5 | Labiodental, moderate frequency range |
| V | 1200 | 5000 | −6 | Similar to F but voiced, lower energy |

### 3. Dental Fricative Wideband Noise

**Priority: High.** Specific subcase of change 2, addressing the most severe single-family failure.

For TH (unvoiced) and DH (voiced), set `noise_path_mix = 1.0` — all noise from the dedicated path, none from the formant filters. The formant filter bank still shapes the voiced component of DH (providing the voicing bar), but the noise component is synthesized independently using `f_low=1000, f_high=8000, slope=-3, gain=0.85` for TH.

This removes the sensitivity to the `sibilant_focus` threshold entirely for TH. There is no longer a threshold cliff at noise_mix=0.24 because TH's noise does not go through the formant filters.

The existing unvoiced TH gain modifier block (lines ~3132–3142 in `say.c`, which widens bandwidths and reduces F3) becomes irrelevant and should be removed once this path is implemented.

### 4. Voiced Fricative Voicing Bar Separation

**Priority: Medium.** Addresses the 0% voiced fricative coverage. Depends on change 2.

For V, DH, ZH: add a **dedicated voiced bar generator** — a low-pass filtered pitch source passed through a single wide-bandwidth filter centered at 80–200 Hz. This creates the characteristic low-frequency voiced buzz that distinguishes voiced fricatives from their unvoiced counterparts and from vowels.

Implementation: inject a separate signal into the output mixer alongside the formant bank output and the secondary noise path. The voicing bar amplitude is fixed at `0.15..0.25` of the overall frame amplitude, regardless of `noise_mix`. Its low-pass cutoff is set to 250 Hz.

For unvoiced fricatives: voicing bar amplitude = 0. No behavioral change.

This separates the acoustic function of the voiced source into two roles:
- **Formant excitation** (pitch harmonics at vowel formant frequencies) — existing path
- **Voicing bar** (low-frequency buzz below the fricative noise) — new path

---

## Implementation Order

| Step | Change | Estimated impact | Risk |
|---|---|---|---|
| 1 | Affricate three-phase alpha model | +3..5% average | Low — only CH/JH phonemes |
| 2 | Dedicated noise path (infrastructure) | Neutral (refactor) | Medium — touches filter loop |
| 3 | TH/DH wideband noise via new path | +1..3% average | Low once step 2 is done |
| 4 | Voiced fricative voicing bar | +0.5..2% average | Low — additive signal |
| 5 | Remove sibilant_focus/noise_focus chain | Cleanup | Low — after all above |

Steps 1 can be done independently of 2–4 and should be done first because it targets the worst-scoring sample class (0%) with no risk of cross-regression.

Steps 2–4 are best done together as a single refactor of the noise synthesis path.

---

## What This Spec Does Not Address

- **Language model decoding failures** (`07-stress`): "English fricatives shift sharply" is decoded as "from the fruit from the" by the phoneme extractor's language model. This is not a synthesis quality issue — it is a structural mismatch between the acoustic output and the extractor's LM context model. The only fix is a different extractor, or a simpler sentence that avoids dense homophonic sequences.
- **Voiced plosive closure murmur** (B, D, G): Adding voiced energy during plosive closures was attempted and caused cascading regressions (−7% to −13%). The current `alpha < 0.30 → voiced = 0` is load-bearing for the overall acoustic quality balance.
- **Weak function word detection**: Short-duration vowels in unstressed function words (scale 0.72–0.82) are below the extractor's detection threshold. Fixing this requires either a prosody model that maintains minimum perceptual energy for unstressed syllables, or a different scoring framework that penalizes function-word deletions less.
