# Real-Time Formant-Based TTS (C) — Specification

## Overview
This project defines a lightweight, real-time formant-based Text-To-Speech (TTS) engine written in C.

Goals:
- Deterministic, low-latency synthesis
- No external dependencies
- Command-line interface
- Output to RAW, AIFF, or WAV audio files
- Support for English and French
- Improved quality over classic 8-bit systems (target: Amiga-level or better)

---

## Functional Requirements

### Input
- Plain text (UTF-8)
- Optional phoneme input mode

### Output
- 16-bit PCM audio
- Formats:
  - RAW (headerless)
  - AIFF
  - WAV

### CLI Usage
```
tts input.txt -o output.aiff --lang en
tts input.txt -o output.wav --lang en
tts "Hello world" -o out.raw --lang fr
```

---

## Architecture

### Pipeline
```
Text → Phonemizer → Prosody → Frame Generator → DSP Synth → Audio Output
```

---

## Modules

### 1. Text Processing
- Normalize input (lowercase, punctuation handling)
- Tokenization

### 2. Phonemizer
- Rule-based grapheme-to-phoneme conversion
- Separate rule sets:
  - English
  - French
- Optional IPA-like intermediate representation

### 3. Prosody Engine
- Duration per phoneme
- Pitch contour (basic intonation)
- Stress handling (EN) / syllabic rhythm (FR)

### 4. Frame Generator
- Convert phoneme stream into time frames (~5–10 ms)
- Interpolate parameters between phonemes

### 5. DSP Synthesizer

#### Source
- Voiced: glottal waveform (saw / LF model)
- Unvoiced: white noise

#### Filter Bank
- 3 to 5 formants (F1–F5)
- Biquad IIR filters

#### Parameters per frame
- Formant frequencies
- Bandwidths
- Amplitude
- Voicing flag

---

## Phoneme System

### Requirements
- Unified phoneme set for EN + FR
- Support:
  - Vowels (oral + nasal for FR)
  - Consonants (voiced/unvoiced)
  - Diphthongs (EN)

### Example
```
/a/ → F1=800Hz, F2=1200Hz
/i/ → F1=300Hz, F2=2200Hz
```

---

## Language Support

### English
- Stress-based timing
- Diphthongs
- Reduced vowels (schwa)

### French
- Syllable-timed rhythm
- Nasal vowels
- Less stress variation

---

## Audio Engine

### Requirements
- Sample rate: 44100 Hz
- Real-time capable (buffer-based processing)
- No dynamic allocation in DSP loop

---

## Performance Constraints
- Target: <5% CPU on modern machine
- Memory: <10 MB
- Binary size: ideally <200 KB

---

## Extensibility
- Add new languages via phoneme tables
- Optional:
  - Vibrato
  - Breath noise
  - Voice presets

---

## Non-Goals
- Neural TTS
- Studio-quality realism
- Large linguistic databases

---

## Deliverables
- Single C codebase
- CLI executable
- Minimal documentation
- Example phoneme tables (EN + FR)

---

## Testing
- Unit tests for:
  - Phoneme parsing
  - Filter stability
- Audio regression tests (checksum or waveform diff)

---

## License
- Must be fully original or use permissive licensed components (MIT/BSD/Apache)
