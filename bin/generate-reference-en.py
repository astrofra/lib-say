from __future__ import annotations

import math
import os
import pathlib
import struct
import subprocess
import sys
import urllib.parse
import urllib.request
import warnings
import wave
import xml.etree.ElementTree as ET

warnings.filterwarnings("ignore", category=DeprecationWarning)
import aifc


MARY_BASE = os.environ.get("MARY_BASE", "http://localhost:59125").rstrip("/")
MARY_VOICE = os.environ.get("MARY_VOICE", "cmu-slt-hsmm")

PHRASES = [
    ("01-demo", "Hello from lib-say. This is an English demo sentence."),
    ("02-hamlet", "To be or not to be, that is the question."),
    ("03-sibilants", "She sells seashells by the seashore."),
]


def request_mary(params: dict[str, str]) -> bytes:
    data = urllib.parse.urlencode(params).encode("utf-8")
    request = urllib.request.Request(f"{MARY_BASE}/process", data=data)
    with urllib.request.urlopen(request, timeout=30) as response:
        return response.read()


def ensure_file(path: pathlib.Path, description: str) -> None:
    if not path.exists():
        raise RuntimeError(f"{description} was not created: {path}")


def run_command(argv: list[str], description: str) -> None:
    result = subprocess.run(argv, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        raise RuntimeError(
            f"{description} failed with exit code {result.returncode}\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )


def write_bytes(path: pathlib.Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def load_audio(path: pathlib.Path) -> tuple[int, list[float], int]:
    suffix = path.suffix.lower()
    if suffix == ".wav":
        with wave.open(str(path), "rb") as handle:
            frame_count = handle.getnframes()
            sample_rate = handle.getframerate()
            sample_width = handle.getsampwidth()
            channels = handle.getnchannels()
            raw = handle.readframes(frame_count)
        if channels != 1:
            raise RuntimeError(f"{path} must be mono, got {channels} channels")
        if sample_width == 1:
            samples = [(byte - 128) / 128.0 for byte in raw]
        elif sample_width == 2:
            samples = [
                value / 32768.0
                for value in struct.unpack("<" + "h" * (len(raw) // 2), raw)
            ]
        else:
            raise RuntimeError(f"{path} has unsupported WAV sample width {sample_width}")
        return sample_rate, samples, sample_width

    if suffix == ".aiff":
        with aifc.open(str(path), "rb") as handle:
            frame_count = handle.getnframes()
            sample_rate = handle.getframerate()
            sample_width = handle.getsampwidth()
            channels = handle.getnchannels()
            raw = handle.readframes(frame_count)
        if channels != 1 or sample_width != 2:
            raise RuntimeError(f"{path} must be mono 16-bit AIFF")
        samples = [
            value / 32768.0
            for value in struct.unpack(">" + "h" * (len(raw) // 2), raw)
        ]
        return sample_rate, samples, sample_width

    raise RuntimeError(f"Unsupported audio type: {path}")


def compute_metrics(sample_rate: int, samples: list[float]) -> dict[str, float]:
    diffs = [abs(samples[i] - samples[i - 1]) for i in range(1, len(samples))]
    window = 1024
    hop = 512
    rms_values: list[float] = []
    zcr_values: list[float] = []

    for start in range(0, max(0, len(samples) - window + 1), hop):
        frame = samples[start : start + window]
        rms_values.append(math.sqrt(sum(value * value for value in frame) / len(frame)))
        zero_crossings = 0
        for left, right in zip(frame, frame[1:]):
            if (left >= 0.0 > right) or (left < 0.0 <= right):
                zero_crossings += 1
        zcr_values.append(zero_crossings / len(frame))

    return {
        "duration_s": len(samples) / sample_rate if sample_rate else 0.0,
        "peak": max((abs(value) for value in samples), default=0.0),
        "mean_rms": sum(rms_values) / len(rms_values) if rms_values else 0.0,
        "zcr_mean": sum(zcr_values) / len(zcr_values) if zcr_values else 0.0,
        "diff_mean": sum(diffs) / len(diffs) if diffs else 0.0,
        "diff_max": max(diffs, default=0.0),
    }


def parse_mary_phonemes(path: pathlib.Path) -> list[str]:
    ns = {"mary": "http://mary.dfki.de/2002/MaryXML"}
    root = ET.fromstring(path.read_text(encoding="utf-8"))
    lines: list[str] = []
    for token in root.findall(".//mary:t", ns):
        word = "".join(token.itertext()).strip()
        phonemes = token.attrib.get("ph", "").strip()
        if word:
            lines.append(f"{word} -> {phonemes}")
    return lines


def main() -> int:
    script_dir = pathlib.Path(__file__).resolve().parent
    repo_dir = script_dir.parent
    out_dir = script_dir / "reference-en"
    tts_exe = script_dir / "tts.exe"
    sam_exe = repo_dir / "reference" / "sam.exe"

    if not tts_exe.exists():
        raise RuntimeError(f"Missing executable: {tts_exe}")
    if not sam_exe.exists():
        raise RuntimeError(f"Missing executable: {sam_exe}")

    out_dir.mkdir(parents=True, exist_ok=True)

    report_lines = [
        "English Reference Comparison",
        "============================",
        f"MaryTTS: {MARY_BASE} voice={MARY_VOICE}",
        "",
    ]

    for sample_id, text in PHRASES:
        sam_path = out_dir / f"sam-{sample_id}.wav"
        mary_audio_path = out_dir / f"mary-{sample_id}.wav"
        mary_phoneme_path = out_dir / f"mary-{sample_id}.phonemes.xml"
        mary_feature_path = out_dir / f"mary-{sample_id}.features.txt"
        our_path = out_dir / f"ours-{sample_id}.aiff"

        run_command([str(sam_exe), "-wav", str(sam_path), text], f"SAM synthesis for {sample_id}")
        ensure_file(sam_path, "SAM reference")

        write_bytes(
            mary_audio_path,
            request_mary(
                {
                    "INPUT_TEXT": text,
                    "INPUT_TYPE": "TEXT",
                    "OUTPUT_TYPE": "AUDIO",
                    "AUDIO": "WAVE_FILE",
                    "LOCALE": "en_US",
                    "VOICE": MARY_VOICE,
                }
            ),
        )
        write_bytes(
            mary_phoneme_path,
            request_mary(
                {
                    "INPUT_TEXT": text,
                    "INPUT_TYPE": "TEXT",
                    "OUTPUT_TYPE": "PHONEMES",
                    "LOCALE": "en_US",
                    "VOICE": MARY_VOICE,
                }
            ),
        )
        write_bytes(
            mary_feature_path,
            request_mary(
                {
                    "INPUT_TEXT": text,
                    "INPUT_TYPE": "TEXT",
                    "OUTPUT_TYPE": "TARGETFEATURES",
                    "OUTPUT_TYPE_PARAMS": "phone stressed accented",
                    "LOCALE": "en_US",
                    "VOICE": MARY_VOICE,
                }
            ),
        )

        run_command(
            [str(tts_exe), text, "-o", str(our_path), "--lang", "en"],
            f"lib-say synthesis for {sample_id}",
        )
        ensure_file(our_path, "lib-say reference")

        sam_metrics = compute_metrics(*load_audio(sam_path)[:2])
        mary_metrics = compute_metrics(*load_audio(mary_audio_path)[:2])
        our_metrics = compute_metrics(*load_audio(our_path)[:2])
        mary_phonemes = parse_mary_phonemes(mary_phoneme_path)

        report_lines.append(f"{sample_id}: {text}")
        report_lines.append("  mary_phonemes:")
        for line in mary_phonemes:
            report_lines.append(f"    {line}")
        report_lines.append(
            "  SAM     : "
            f"{sam_path.name} duration={sam_metrics['duration_s']:.3f}s "
            f"mean_rms={sam_metrics['mean_rms']:.4f} zcr={sam_metrics['zcr_mean']:.4f} "
            f"diff_mean={sam_metrics['diff_mean']:.4f} diff_max={sam_metrics['diff_max']:.4f}"
        )
        report_lines.append(
            "  MaryTTS : "
            f"{mary_audio_path.name} duration={mary_metrics['duration_s']:.3f}s "
            f"mean_rms={mary_metrics['mean_rms']:.4f} zcr={mary_metrics['zcr_mean']:.4f} "
            f"diff_mean={mary_metrics['diff_mean']:.4f} diff_max={mary_metrics['diff_max']:.4f}"
        )
        report_lines.append(
            "  lib-say : "
            f"{our_path.name} duration={our_metrics['duration_s']:.3f}s "
            f"mean_rms={our_metrics['mean_rms']:.4f} zcr={our_metrics['zcr_mean']:.4f} "
            f"diff_mean={our_metrics['diff_mean']:.4f} diff_max={our_metrics['diff_max']:.4f}"
        )
        report_lines.append(
            "  delta vs Mary: "
            f"duration x{(our_metrics['duration_s'] / mary_metrics['duration_s']) if mary_metrics['duration_s'] else 0.0:.2f}, "
            f"rms x{(our_metrics['mean_rms'] / mary_metrics['mean_rms']) if mary_metrics['mean_rms'] else 0.0:.2f}, "
            f"zcr x{(our_metrics['zcr_mean'] / mary_metrics['zcr_mean']) if mary_metrics['zcr_mean'] else 0.0:.2f}"
        )
        report_lines.append(
            "  delta vs SAM : "
            f"duration x{(our_metrics['duration_s'] / sam_metrics['duration_s']) if sam_metrics['duration_s'] else 0.0:.2f}, "
            f"rms x{(our_metrics['mean_rms'] / sam_metrics['mean_rms']) if sam_metrics['mean_rms'] else 0.0:.2f}, "
            f"zcr x{(our_metrics['zcr_mean'] / sam_metrics['zcr_mean']) if sam_metrics['zcr_mean'] else 0.0:.2f}"
        )
        report_lines.append("")

    comparison_path = out_dir / "comparison.txt"
    comparison_path.write_text("\n".join(report_lines), encoding="utf-8")
    print(f"Wrote {comparison_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:  # pragma: no cover - CLI failure path
        print(exc, file=sys.stderr)
        raise SystemExit(1)
