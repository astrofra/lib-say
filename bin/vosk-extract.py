"""Vosk-based phoneme extractor: word recognition + CMUdict expansion.

Usage:
  vosk-extract.py <wav_file> [output_file] [--grammar TEXT]

  If output_file is omitted, writes to stdout.
  --grammar constrains Vosk to the expected text (improves accuracy on synthetic
  speech by preventing the LM from wandering too far).
  Output format is compatible with phonemes.exe (header + one ARPAbet token per line).
"""
from __future__ import annotations

import argparse
import json
import logging
import os
import pathlib
import re
import sys
import wave

import pronouncing
import vosk

logging.getLogger("vosk").setLevel(logging.WARNING)
os.environ.setdefault("VOSK_LOG_LEVEL", "-1")

_REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
MODEL_PATH = _REPO_ROOT / "reference" / "vosk-model"

# ARPAbet (CMUdict stress-stripped) → lowercase Kaldi label matching EXTRACTOR_PHONE_MAP
_ARPABET_REMAP: dict[str, str] = {
    "HH": "h",  # CMUdict uses HH; extractor map has `h` → ["H"]
}


def _arpabet_to_extractor(phone: str) -> str:
    clean = phone.rstrip("012")
    return _ARPABET_REMAP.get(clean, clean).lower()


def _word_phonemes(word: str) -> list[str]:
    pronunciations = pronouncing.phones_for_word(word.lower())
    if not pronunciations:
        return []
    return [_arpabet_to_extractor(p) for p in pronunciations[0].split()]


def _normalize_grammar_text(text: str) -> str:
    """Strip punctuation and lowercase so Vosk grammar matches vocabulary."""
    text = text.lower()
    text = re.sub(r"[^a-z\s]", " ", text)
    return re.sub(r"\s+", " ", text).strip()


def _load_model() -> vosk.Model:
    if not MODEL_PATH.exists():
        raise RuntimeError(
            f"Vosk model not found at {MODEL_PATH}.\n"
            "Download vosk-model-small-en-us-0.15 from alphacephei.com/vosk and "
            f"extract it to {MODEL_PATH}."
        )
    vosk.SetLogLevel(-1)
    return vosk.Model(str(MODEL_PATH))


def extract(wav_path: pathlib.Path, grammar_text: str | None = None) -> list[str]:
    """Recognize words from a WAV then expand to phonemes via CMUdict.

    If grammar_text is given, Vosk is constrained to that phrase (plus [unk]).
    This significantly reduces LM drift on synthetic speech.
    """
    model = _load_model()
    with wave.open(str(wav_path), "rb") as wf:
        channels = wf.getnchannels()
        sample_width = wf.getsampwidth()
        sample_rate = wf.getframerate()
        if channels != 1 or sample_width != 2:
            raise RuntimeError(
                f"{wav_path}: need mono 16-bit PCM WAV (got {channels}ch {sample_width * 8}bit)"
            )
        rec = vosk.KaldiRecognizer(model, sample_rate)
        rec.SetWords(True)
        if grammar_text is not None:
            normalized = _normalize_grammar_text(grammar_text)
            rec.SetGrammar(json.dumps([normalized, "[unk]"]))
        while True:
            data = wf.readframes(4000)
            if not data:
                break
            rec.AcceptWaveform(data)
        result = json.loads(rec.FinalResult())

    words = [
        w["word"]
        for w in result.get("result", [])
        if w["word"] != "[unk]"
    ]
    phonemes: list[str] = []
    for word in words:
        phonemes.extend(_word_phonemes(word))
    return phonemes


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("wav", help="Input WAV file (mono 16-bit PCM)")
    parser.add_argument("output", nargs="?", help="Output file (default: stdout)")
    parser.add_argument("--grammar", default=None, help="Expected sentence text (constrains ASR)")
    args = parser.parse_args()

    wav_path = pathlib.Path(args.wav)
    out_path = pathlib.Path(args.output) if args.output else None

    try:
        phonemes = extract(wav_path, grammar_text=args.grammar)
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    lines = ["phone\tstart\tend"] + phonemes
    output = "\n".join(lines) + "\n"

    if out_path is not None:
        out_path.write_text(output, encoding="utf-8")
    else:
        sys.stdout.write(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
