from __future__ import annotations

import collections
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
    {"id": "01-demo", "focus": "baseline", "text": "Hello from lib-say. This is an English demo sentence."},
    {"id": "02-hamlet", "focus": "function-words", "text": "To be or not to be, that is the question."},
    {"id": "03-sibilants", "focus": "sibilants", "text": "She sells seashells by the seashore."},
    {"id": "04-dentals", "focus": "th-dh", "text": "This thing is worth the effort."},
    {"id": "05-voiced-dentals", "focus": "dh-there", "text": "Those feathers gather there."},
    {"id": "06-affricates", "focus": "affricates", "text": "Judge the changing church bells."},
    {"id": "07-stress", "focus": "stress", "text": "English fricatives shift sharply."},
    {"id": "08-finals", "focus": "word-final-fricatives", "text": "Leave these clothes and bags."},
]

MARY_PHONE_MAP = {
    "@": ["SCHWA"],
    "@U": ["OH", "W"],
    "A": ["A"],
    "AI": ["AH", "J"],
    "D": ["DH"],
    "E": ["EH"],
    "EI": ["E", "J"],
    "I": ["IH"],
    "N": ["NG"],
    "O": ["OH"],
    "OI": ["OH", "J"],
    "S": ["SH"],
    "T": ["TH"],
    "U": ["U"],
    "V": ["AH"],
    "Z": ["ZH"],
    "aU": ["AH", "W"],
    "b": ["B"],
    "d": ["D"],
    "dZ": ["JH"],
    "f": ["F"],
    "g": ["G"],
    "h": ["H"],
    "i": ["I"],
    "j": ["J"],
    "k": ["K"],
    "l": ["L"],
    "m": ["M"],
    "n": ["N"],
    "p": ["P"],
    "r": ["R"],
    "r=": ["R"],
    "s": ["S"],
    "t": ["T"],
    "tS": ["CH"],
    "u": ["U"],
    "v": ["V"],
    "w": ["W"],
    "z": ["Z"],
    "{": ["AE"],
}

VOWEL_TOKENS = {"A", "AE", "AH", "E", "EH", "I", "IH", "O", "OH", "U", "SCHWA"}


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


def normalize_word(word: str) -> str:
    return "".join(char for char in word.lower() if char.isalpha())


def normalize_our_phone_string(raw: str) -> tuple[list[str], list[int]]:
    tokens: list[str] = []
    stress_positions: list[int] = []
    for raw_token in raw.split():
        token = raw_token.strip()
        if not token:
            continue
        stressed = token.startswith("'")
        if stressed:
            token = token[1:]
        if not token:
            continue
        tokens.append(token)
        if stressed:
            stress_positions.append(len(tokens) - 1)
    return tokens, stress_positions


def normalize_mary_phone_string(raw: str) -> tuple[list[str], list[int], list[str]]:
    tokens: list[str] = []
    stress_positions: list[int] = []
    unknown_tokens: list[str] = []
    stress_next = False

    for raw_token in raw.split():
        token = raw_token.strip()
        if not token or token == "-":
            continue
        if token == "'":
            stress_next = True
            continue
        mapped = MARY_PHONE_MAP.get(token)
        if mapped is None:
            unknown_tokens.append(token)
            mapped = [f"?{token}"]
        for mapped_token in mapped:
            tokens.append(mapped_token)
            if stress_next:
                stress_positions.append(len(tokens) - 1)
                stress_next = False

    return tokens, stress_positions, unknown_tokens


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


def parse_mary_phonemes(path: pathlib.Path) -> list[dict[str, object]]:
    ns = {"mary": "http://mary.dfki.de/2002/MaryXML"}
    root = ET.fromstring(path.read_text(encoding="utf-8"))
    items: list[dict[str, object]] = []
    for token in root.findall(".//mary:t", ns):
        word = "".join(token.itertext()).strip()
        normalized_word = normalize_word(word)
        if not normalized_word:
            continue
        raw = token.attrib.get("ph", "").strip()
        normalized_tokens, stress_positions, unknown_tokens = normalize_mary_phone_string(raw)
        items.append(
            {
                "display_word": word,
                "word": normalized_word,
                "raw": raw,
                "tokens": normalized_tokens,
                "stress": stress_positions,
                "unknown": unknown_tokens,
            }
        )
    return items


def parse_our_debug_report(path: pathlib.Path) -> list[dict[str, object]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    items: list[dict[str, object]] = []
    in_word_section = False

    for line in lines:
        stripped = line.strip()
        if stripped == "word_to_phoneme:":
            in_word_section = True
            continue
        if in_word_section and not stripped:
            break
        if not in_word_section:
            continue
        if stripped.startswith("[pause:") or "->" not in stripped:
            continue

        left, right = stripped.split("->", 1)
        word = left.strip()
        raw = right.strip()
        tokens, stress_positions = normalize_our_phone_string(raw)
        items.append(
            {
                "display_word": word,
                "word": normalize_word(word),
                "raw": raw,
                "tokens": tokens,
                "stress": stress_positions,
            }
        )

    return items


def levenshtein_distance(left: list[str], right: list[str]) -> int:
    if not left:
        return len(right)
    if not right:
        return len(left)

    previous = list(range(len(right) + 1))
    for i, left_token in enumerate(left, start=1):
        current = [i]
        for j, right_token in enumerate(right, start=1):
            insertion = current[j - 1] + 1
            deletion = previous[j] + 1
            substitution = previous[j - 1] + (0 if left_token == right_token else 1)
            current.append(min(insertion, deletion, substitution))
        previous = current
    return previous[-1]


def count_vowels(tokens: list[str]) -> int:
    return sum(1 for token in tokens if token in VOWEL_TOKENS)


def classify_difference(ours: dict[str, object], mary: dict[str, object]) -> str:
    our_tokens = ours["tokens"]
    mary_tokens = mary["tokens"]
    our_stress = ours["stress"]
    mary_stress = mary["stress"]

    if our_tokens == mary_tokens:
        vowel_count = max(count_vowels(our_tokens), count_vowels(mary_tokens))
        if our_stress == mary_stress or vowel_count <= 1:
            return "exact"
        return "stress_mismatch"

    our_vowels = [token for token in our_tokens if token in VOWEL_TOKENS]
    mary_vowels = [token for token in mary_tokens if token in VOWEL_TOKENS]
    our_consonants = [token for token in our_tokens if token not in VOWEL_TOKENS]
    mary_consonants = [token for token in mary_tokens if token not in VOWEL_TOKENS]

    if our_vowels == mary_vowels and our_consonants != mary_consonants:
        return "consonant_mismatch"
    if our_consonants == mary_consonants and our_vowels != mary_vowels:
        return "vowel_mismatch"
    if len(our_tokens) != len(mary_tokens):
        distance = levenshtein_distance(our_tokens, mary_tokens)
        if distance <= 1:
            return "near_mismatch"
        return "length_mismatch"
    if levenshtein_distance(our_tokens, mary_tokens) <= 1:
        return "near_mismatch"
    return "mixed_mismatch"


def compare_word_lists(
    ours: list[dict[str, object]], mary: list[dict[str, object]]
) -> tuple[list[dict[str, object]], collections.Counter[str], list[str]]:
    comparisons: list[dict[str, object]] = []
    counts: collections.Counter[str] = collections.Counter()
    issues: list[str] = []

    if len(ours) != len(mary):
        issues.append(f"word_count_mismatch ours={len(ours)} mary={len(mary)}")

    for index in range(min(len(ours), len(mary))):
        our_item = ours[index]
        mary_item = mary[index]
        if our_item["word"] != mary_item["word"]:
            issues.append(
                f"word_alignment_mismatch index={index} ours={our_item['display_word']} mary={mary_item['display_word']}"
            )
            label = "word_alignment_mismatch"
        else:
            label = classify_difference(our_item, mary_item)
        counts[label] += 1
        comparisons.append(
            {
                "word": mary_item["display_word"],
                "label": label,
                "mary_raw": mary_item["raw"],
                "mary_norm": " ".join(mary_item["tokens"]),
                "our_raw": our_item["raw"],
                "our_norm": " ".join(our_item["tokens"]),
            }
        )

    if len(ours) > len(mary):
        for item in ours[len(mary) :]:
            counts["extra_ours_word"] += 1
            issues.append(f"extra_ours_word {item['display_word']}")
    elif len(mary) > len(ours):
        for item in mary[len(ours) :]:
            counts["extra_mary_word"] += 1
            issues.append(f"extra_mary_word {item['display_word']}")

    return comparisons, counts, issues


def append_metric_line(lines: list[str], name: str, path: pathlib.Path, metrics: dict[str, float]) -> None:
    lines.append(
        f"  {name:<8}: {path.name} "
        f"duration={metrics['duration_s']:.3f}s "
        f"mean_rms={metrics['mean_rms']:.4f} "
        f"zcr={metrics['zcr_mean']:.4f} "
        f"diff_mean={metrics['diff_mean']:.4f} "
        f"diff_max={metrics['diff_max']:.4f}"
    )


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
        "English Oracle Comparison",
        "=========================",
        f"MaryTTS: {MARY_BASE} voice={MARY_VOICE}",
        "",
        "Corpus:",
    ]
    for item in PHRASES:
        report_lines.append(f"  {item['id']} [{item['focus']}] {item['text']}")
    report_lines.append("")

    aggregate_counts: collections.Counter[str] = collections.Counter()
    mismatch_examples: dict[str, list[str]] = collections.defaultdict(list)
    duration_ratios: dict[str, float] = {}

    for item in PHRASES:
        sample_id = item["id"]
        text = item["text"]

        sam_path = out_dir / f"sam-{sample_id}.wav"
        mary_audio_path = out_dir / f"mary-{sample_id}.wav"
        mary_phoneme_path = out_dir / f"mary-{sample_id}.phonemes.xml"
        mary_feature_path = out_dir / f"mary-{sample_id}.features.txt"
        our_path = out_dir / f"ours-{sample_id}.aiff"
        our_debug_path = out_dir / f"ours-{sample_id}.debug.txt"

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
            [str(tts_exe), text, "--lang", "en", "--debug-report", str(our_debug_path), "--dry-run"],
            f"lib-say debug for {sample_id}",
        )
        run_command(
            [str(tts_exe), text, "-o", str(our_path), "--lang", "en"],
            f"lib-say synthesis for {sample_id}",
        )
        ensure_file(our_path, "lib-say reference")
        ensure_file(our_debug_path, "lib-say debug")

        sam_metrics = compute_metrics(*load_audio(sam_path)[:2])
        mary_metrics = compute_metrics(*load_audio(mary_audio_path)[:2])
        our_metrics = compute_metrics(*load_audio(our_path)[:2])
        mary_words = parse_mary_phonemes(mary_phoneme_path)
        our_words = parse_our_debug_report(our_debug_path)
        comparisons, counts, issues = compare_word_lists(our_words, mary_words)

        aggregate_counts.update(counts)
        for comparison in comparisons:
            if comparison["label"] != "exact":
                mismatch_examples[comparison["label"]].append(
                    f"{comparison['word']}: ours={comparison['our_norm']} | mary={comparison['mary_norm']}"
                )

        report_lines.append(f"{sample_id} [{item['focus']}]: {text}")
        append_metric_line(report_lines, "SAM", sam_path, sam_metrics)
        append_metric_line(report_lines, "MaryTTS", mary_audio_path, mary_metrics)
        append_metric_line(report_lines, "lib-say", our_path, our_metrics)
        report_lines.append(
            "  delta vs Mary: "
            f"duration x{(our_metrics['duration_s'] / mary_metrics['duration_s']) if mary_metrics['duration_s'] else 0.0:.2f}, "
            f"rms x{(our_metrics['mean_rms'] / mary_metrics['mean_rms']) if mary_metrics['mean_rms'] else 0.0:.2f}, "
            f"zcr x{(our_metrics['zcr_mean'] / mary_metrics['zcr_mean']) if mary_metrics['zcr_mean'] else 0.0:.2f}"
        )
        if mary_metrics["duration_s"]:
            duration_ratios[sample_id] = our_metrics["duration_s"] / mary_metrics["duration_s"]
        report_lines.append("  word_diffs:")
        for comparison in comparisons:
            report_lines.append(
                f"    {comparison['label']:<22} {comparison['word']}: "
                f"ours={comparison['our_norm']} | mary={comparison['mary_norm']}"
            )
        if issues:
            report_lines.append("  issues:")
            for issue in issues:
                report_lines.append(f"    {issue}")
        unknown_mary = sorted({token for word in mary_words for token in word["unknown"]})
        if unknown_mary:
            report_lines.append(f"  unknown_mary_tokens: {' '.join(unknown_mary)}")
        report_lines.append("")

    report_lines.append("Summary")
    report_lines.append("-------")
    for label, count in sorted(aggregate_counts.items()):
        report_lines.append(f"  {label}: {count}")

    report_lines.append("")
    report_lines.append("Actionable Summary")
    report_lines.append("------------------")
    for label in ["consonant_mismatch", "length_mismatch", "vowel_mismatch", "mixed_mismatch", "near_mismatch"]:
        report_lines.append(f"  {label}: {aggregate_counts.get(label, 0)}")

    tracked_ids = ["01-demo", "04-dentals", "08-finals"]
    if any(sample_id in duration_ratios for sample_id in tracked_ids):
        report_lines.append("")
        report_lines.append("Duration Targets")
        report_lines.append("----------------")
        for sample_id in tracked_ids:
            ratio = duration_ratios.get(sample_id)
            if ratio is not None:
                report_lines.append(f"  {sample_id}: duration x{ratio:.2f}")

    if mismatch_examples:
        report_lines.append("")
        report_lines.append("Mismatch Samples")
        report_lines.append("----------------")
        for label in sorted(mismatch_examples):
            report_lines.append(f"  {label}:")
            for sample in mismatch_examples[label][:8]:
                report_lines.append(f"    {sample}")

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
