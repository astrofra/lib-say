"""Generate the curated English demo set.

For each phrase in ../tests/corpus.tsv this writes two WAVs into
./output/{biquad,amiga}/<id>.wav using the lib-say tts.exe in both rendering
paths. Run after `cmake --build build --config Release` so the binary is
fresh.

Usage:
    python samples/generate.py             # render all phrases
    python samples/generate.py 03 05       # render only phrases whose id starts
                                           # with "03" or "05"
"""
from __future__ import annotations

import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
TTS = ROOT / "bin" / "tts.exe"
CORPUS = ROOT / "tests" / "corpus.tsv"
OUTPUT = pathlib.Path(__file__).resolve().parent / "output"


def load_corpus() -> list[tuple[str, str]]:
    rows: list[tuple[str, str]] = []
    for line in CORPUS.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        sample_id, text = line.split("\t", 1)
        rows.append((sample_id, text))
    return rows


def render(sample_id: str, text: str, path: pathlib.Path, amiga: bool) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    cmd = [str(TTS), text, "-o", str(path), "--lang", "en"]
    if amiga:
        cmd.append("--amiga")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(
            f"tts.exe failed for {sample_id} ({'amiga' if amiga else 'biquad'}):\n"
            f"{result.stderr}"
        )


def main(filters: list[str]) -> int:
    if not TTS.exists():
        print(f"Missing {TTS} — run cmake build first", file=sys.stderr)
        return 1

    corpus = load_corpus()
    if filters:
        corpus = [r for r in corpus if any(r[0].startswith(p) for p in filters)]

    print(f"Rendering {len(corpus)} phrases x 2 paths -> {OUTPUT}")
    for sample_id, text in corpus:
        for path_label, amiga_flag in (("biquad", False), ("amiga", True)):
            wav = OUTPUT / path_label / f"{sample_id}.wav"
            render(sample_id, text, wav, amiga_flag)
            print(f"  {path_label:6}/{sample_id}.wav  ({len(text)} chars)")
    print("Done.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
