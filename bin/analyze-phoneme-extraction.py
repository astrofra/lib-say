from __future__ import annotations

import argparse
import collections
import datetime as dt
import math
import pathlib
import re
import shutil
import struct
import subprocess
import sys
import time
import wave
import xml.etree.ElementTree as ET


PHRASES = [
    {"id": "01-demo", "focus": "baseline", "text": "Hello from lib-say. This is an English demo sentence."},
    {"id": "02-hamlet", "focus": "function-words", "text": "To be or not to be, that is the question."},
    {"id": "03-sibilants", "focus": "sibilants", "text": "She sells seashells by the seashore."},
    {"id": "04-dentals", "focus": "th-dh", "text": "This thing is worth the effort."},
    {"id": "05-voiced-dentals", "focus": "dh-there", "text": "Those feathers gather there."},
    {"id": "06-affricates", "focus": "affricates", "text": "Judge the changing church bells."},
    {"id": "07-stress", "focus": "stress", "text": "English fricatives shift sharply."},
    {"id": "08-finals", "focus": "word-final-fricatives", "text": "Leave these clothes and bags."},
    {"id": "09-th-only", "focus": "th-unvoiced-only", "text": "Three thick thin things worth both teeth."},
]

EXTRACTOR_PHONE_MAP = {
    "_": ["PAUSE"],
    "aa": ["A"],
    "ae": ["AE"],
    "ah": ["AH"],
    "ao": ["OH"],
    "aw": ["AH", "W"],
    "ax": ["SCHWA"],
    "ay": ["AH", "J"],
    "b": ["B"],
    "ch": ["CH"],
    "d": ["D"],
    "dh": ["DH"],
    "eh": ["EH"],
    "er": ["R"],
    "ey": ["E", "J"],
    "f": ["F"],
    "g": ["G"],
    "h": ["H"],
    "hh": ["H"],
    "ih": ["IH"],
    "iy": ["I"],
    "jh": ["JH"],
    "k": ["K"],
    "l": ["L"],
    "m": ["M"],
    "n": ["N"],
    "ng": ["NG"],
    "ow": ["OH", "W"],
    "oy": ["OH", "J"],
    "p": ["P"],
    "r": ["R"],
    "s": ["S"],
    "sh": ["SH"],
    "t": ["T"],
    "th": ["TH"],
    "uh": ["U"],
    "uw": ["U"],
    "v": ["V"],
    "w": ["W"],
    "y": ["J"],
    "z": ["Z"],
    "zh": ["ZH"],
}

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

VOWELS = {"A", "AE", "AH", "E", "EH", "I", "IH", "O", "OH", "U", "SCHWA"}
PHONE_FAMILIES = [
    ("sibilants", "Sibilantes", {"S", "Z", "SH", "ZH"}),
    ("dentals", "Dentales TH/DH", {"TH", "DH"}),
    ("affricates", "Affriquées", {"CH", "JH", "TS", "DZ"}),
    ("voiced_fricatives", "Fricatives voisées", {"V", "DH", "Z", "ZH"}),
    ("vowels", "Voyelles", VOWELS),
]


def repo_root() -> pathlib.Path:
    return pathlib.Path(__file__).resolve().parent.parent


def run_command(argv: list[str], description: str, cwd: pathlib.Path | None = None) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(argv, cwd=str(cwd) if cwd else None, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        raise RuntimeError(
            f"{description} failed with exit code {result.returncode}\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )
    return result


def git_value(root: pathlib.Path, *args: str) -> str:
    result = subprocess.run(["git", *args], cwd=str(root), capture_output=True, text=True, check=False)
    if result.returncode != 0:
        return "unknown"
    return result.stdout.strip() or "unknown"


def parse_expected_debug_report(path: pathlib.Path) -> list[str]:
    text = path.read_text(encoding="utf-8", errors="replace")
    match = re.search(r"phoneme_stream:\s*\n\s*(.*?)\n\nsegments:", text, re.S)
    if not match:
        return []
    raw = match.group(1)
    raw = re.sub(r"\[pause[^\]]*\]", " PAUSE ", raw)
    raw = raw.replace("/", " ")
    return [token.lstrip("'") for token in raw.split()]


def parse_extractor_output(path: pathlib.Path) -> tuple[list[str], list[str]]:
    tokens: list[str] = []
    unknown: list[str] = []
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    for line in lines[1:]:
        parts = line.split()
        if not parts:
            continue
        raw_phone = parts[0].lower()
        mapped = EXTRACTOR_PHONE_MAP.get(raw_phone)
        if mapped is None:
            tokens.append(f"?{parts[0]}")
            unknown.append(parts[0])
        else:
            tokens.extend(mapped)
    return tokens, unknown


def parse_mary_phonemes(path: pathlib.Path) -> tuple[list[str], list[str]]:
    ns = {"mary": "http://mary.dfki.de/2002/MaryXML"}
    root = ET.fromstring(path.read_text(encoding="utf-8", errors="replace"))
    tokens: list[str] = []
    unknown: list[str] = []
    for token_node in root.findall(".//mary:t", ns):
        raw = token_node.attrib.get("ph", "").strip()
        stress_next = False
        for raw_token in raw.split():
            if raw_token == "-":
                continue
            if raw_token == "'":
                stress_next = True
                continue
            mapped = MARY_PHONE_MAP.get(raw_token)
            if mapped is None:
                tokens.append(f"?{raw_token}")
                unknown.append(raw_token)
            else:
                tokens.extend(mapped)
            stress_next = False
    return tokens, unknown


def levenshtein(left: list[str], right: list[str]) -> int:
    if not left:
        return len(right)
    if not right:
        return len(left)

    previous = list(range(len(right) + 1))
    for i, left_token in enumerate(left, start=1):
        current = [i]
        for j, right_token in enumerate(right, start=1):
            current.append(
                min(
                    current[j - 1] + 1,
                    previous[j] + 1,
                    previous[j - 1] + (0 if left_token == right_token else 1),
                )
            )
        previous = current
    return previous[-1]


def score_alignment(expected: list[str], actual: list[str]) -> tuple[int, float]:
    distance = levenshtein(expected, actual)
    score = 1.0 - distance / max(len(expected), len(actual), 1)
    return distance, max(0.0, score)


def classify_tokens(tokens: list[str]) -> collections.Counter[str]:
    counts: collections.Counter[str] = collections.Counter()
    for token in tokens:
        for family_id, _label, family_tokens in PHONE_FAMILIES:
            if token in family_tokens:
                counts[family_id] += 1
    return counts


def classify_missing(tokens: list[str]) -> collections.Counter[str]:
    counts: collections.Counter[str] = collections.Counter()
    for token in tokens:
        if token in {"S", "Z", "SH", "ZH"}:
            counts["sibilants"] += 1
        elif token in {"TH", "DH"}:
            counts["dentals"] += 1
        elif token in {"CH", "JH", "TS", "DZ"}:
            counts["affricates"] += 1
        elif token in VOWELS:
            counts["vowels"] += 1
        elif token != "PAUSE":
            counts["consonants"] += 1
    return counts


def family_ratio(expected_counts: collections.Counter[str], observed_counts: collections.Counter[str], family_id: str) -> float | None:
    expected_count = expected_counts[family_id]
    if expected_count == 0:
        return None
    return min(observed_counts[family_id], expected_count) / expected_count


def extract_family_rows(rows: list[dict[str, object]]) -> list[dict[str, object]]:
    diagnostics: list[dict[str, object]] = []
    for row in rows:
        expected_counts = row["expected_family_counts"]
        extracted_counts = row["extracted_family_counts"]
        mary_counts = row["mary_family_counts"]
        for family_id, label, _tokens in PHONE_FAMILIES:
            expected_count = expected_counts[family_id]
            if expected_count == 0:
                continue
            diagnostics.append(
                {
                    "id": row["id"],
                    "focus": row["focus"],
                    "family_id": family_id,
                    "label": label,
                    "expected": expected_count,
                    "extracted": extracted_counts[family_id],
                    "coverage": family_ratio(expected_counts, extracted_counts, family_id),
                    "mary": mary_counts[family_id] if row["mary_tokens"] else None,
                    "mary_coverage": family_ratio(expected_counts, mary_counts, family_id) if row["mary_tokens"] else None,
                }
            )
    return diagnostics


def recommendation_for_row(row: dict[str, object]) -> str:
    focus = str(row["focus"])
    score = float(row["score"])
    expected_counts = row["expected_family_counts"]
    extracted_counts = row["extracted_family_counts"]

    if focus == "sibilants":
        coverage = family_ratio(expected_counts, extracted_counts, "sibilants")
        if coverage is not None and coverage < 0.5:
            return "Renforcer l'identité spectrale S/SH/Z avant d'augmenter encore la durée : le corpus contient beaucoup de sibilantes, mais l'extracteur en retrouve peu."
        return "Comparer surtout S vs SH/Z : la famille est perçue, mais l'identité exacte reste instable."
    if focus in {"th-dh", "dh-there"}:
        coverage = family_ratio(expected_counts, extracted_counts, "dentals")
        voiced_coverage = family_ratio(expected_counts, extracted_counts, "voiced_fricatives")
        if voiced_coverage is not None and voiced_coverage < 0.5:
            return "Travailler les fricatives voisées séparément de TH : DH/Z/V doivent rester voisées sans devenir de simples occlusives."
        if coverage is not None and coverage < 0.5:
            return "Isoler TH/DH dans un micro-corpus : le score bas vient probablement d'une confusion de lieu d'articulation, pas seulement de voisement."
    if focus == "affricates":
        return "Éviter les changements acoustiques agressifs sur CH/JH tant que le diagnostic famille ne progresse pas ; le dernier essai a dégradé le score strict."
    if focus == "word-final-fricatives":
        return "Tester les fins de mot avec paires minimales courtes : les fricatives finales semblent se mélanger avec des consonnes non voisées."
    if score < 0.25:
        return "Créer une phrase de test plus courte pour séparer erreur de reconnaissance lexicale et erreur phonétique."
    return "Surveiller en régression ; ce cas sert surtout de contrôle de stabilité."


def load_wav(path: pathlib.Path) -> tuple[int, list[float]]:
    with wave.open(str(path), "rb") as handle:
        frame_count = handle.getnframes()
        sample_rate = handle.getframerate()
        sample_width = handle.getsampwidth()
        channels = handle.getnchannels()
        raw = handle.readframes(frame_count)
    if channels != 1:
        raise RuntimeError(f"{path} must be mono, got {channels} channels")
    if sample_width != 2:
        raise RuntimeError(f"{path} must be 16-bit PCM, got sample width {sample_width}")
    samples = [value / 32768.0 for value in struct.unpack("<" + "h" * (len(raw) // 2), raw)]
    return sample_rate, samples


def audio_metrics(path: pathlib.Path) -> dict[str, float]:
    sample_rate, samples = load_wav(path)
    diffs = [abs(samples[i] - samples[i - 1]) for i in range(1, len(samples))]
    window = 1024
    hop = 512
    rms_values: list[float] = []
    zcr_values: list[float] = []

    for start in range(0, max(0, len(samples) - window + 1), hop):
        frame = samples[start : start + window]
        rms_values.append(math.sqrt(sum(value * value for value in frame) / len(frame)))
        zcr_values.append(
            sum(1 for left, right in zip(frame, frame[1:]) if (left >= 0.0 > right) or (left < 0.0 <= right))
            / len(frame)
        )

    return {
        "duration_s": len(samples) / sample_rate if sample_rate else 0.0,
        "mean_rms": sum(rms_values) / len(rms_values) if rms_values else 0.0,
        "zcr_mean": sum(zcr_values) / len(zcr_values) if zcr_values else 0.0,
        "diff_mean": sum(diffs) / len(diffs) if diffs else 0.0,
        "diff_max": max(diffs, default=0.0),
    }


def format_tokens(tokens: list[str], limit: int = 80) -> str:
    if len(tokens) <= limit:
        return " ".join(tokens)
    return " ".join(tokens[:limit]) + " ..."


def percent(value: float) -> str:
    return f"{value * 100.0:.2f}%"


def metric(value: float) -> str:
    return f"{value:.4f}"


def analyze(args: argparse.Namespace) -> tuple[pathlib.Path, pathlib.Path | None]:
    root = repo_root()
    out_dir = (root / args.out_dir).resolve() if not pathlib.Path(args.out_dir).is_absolute() else pathlib.Path(args.out_dir)
    latest_dir = out_dir / "latest"
    history_dir = out_dir / "history"
    reference_dir = root / "bin" / "reference-en"
    tts_exe = root / "bin" / "tts.exe"
    extractor_dir = root / "reference" / "phonemes_extractor"
    extractor_exe = extractor_dir / "phonemes.exe"
    vosk_script = root / "bin" / "vosk-extract.py"

    use_vosk = getattr(args, "extractor", "legacy") == "vosk"

    if not tts_exe.exists():
        raise RuntimeError(f"Missing executable: {tts_exe}")
    if not use_vosk and not extractor_exe.exists():
        raise RuntimeError(f"Missing executable: {extractor_exe}")
    if use_vosk and not vosk_script.exists():
        raise RuntimeError(f"Missing script: {vosk_script}")

    latest_dir.mkdir(parents=True, exist_ok=True)
    if args.history:
        history_dir.mkdir(parents=True, exist_ok=True)

    git_commit = git_value(root, "rev-parse", "--short", "HEAD")
    git_dirty = bool(git_value(root, "status", "--porcelain"))
    generated_at = dt.datetime.now().astimezone()
    rows: list[dict[str, object]] = []
    aggregate_missing: collections.Counter[str] = collections.Counter()

    for item in PHRASES:
        sample_id = item["id"]
        text = item["text"]
        wav_path = latest_dir / f"ours-{sample_id}.wav"
        debug_path = latest_dir / f"ours-{sample_id}.debug.txt"
        extractor_output_path = latest_dir / f"ours-{sample_id}.phonemes.txt"

        run_command(
            [
                str(tts_exe),
                text,
                "-o",
                str(wav_path),
                "--lang",
                args.lang,
                "--debug-report",
                str(debug_path),
            ],
            f"lib-say synthesis for {sample_id}",
        )

        if use_vosk:
            vosk_cmd = [sys.executable, str(vosk_script), str(wav_path), str(extractor_output_path),
                        "--grammar", text]
            run_command(vosk_cmd, f"vosk phoneme extraction for {sample_id}")
        else:
            run_command([str(extractor_exe), str(wav_path)], f"phoneme extraction for {sample_id}", cwd=latest_dir)
            fixed_output = latest_dir / "phonemes.txt"
            for _ in range(10):
                try:
                    shutil.copy2(fixed_output, extractor_output_path)
                    break
                except OSError:
                    time.sleep(0.1)
            else:
                shutil.copy2(fixed_output, extractor_output_path)
            fixed_output.unlink(missing_ok=True)

        expected = parse_expected_debug_report(debug_path)
        extracted, extractor_unknown = parse_extractor_output(extractor_output_path)
        distance, extraction_score = score_alignment(expected, extracted)
        missing_counts = classify_missing(expected)
        aggregate_missing.update(missing_counts)
        expected_family_counts = classify_tokens(expected)
        extracted_family_counts = classify_tokens(extracted)
        our_metrics = audio_metrics(wav_path)

        mary_tokens: list[str] = []
        mary_score: float | None = None
        mary_distance: int | None = None
        mary_unknown: list[str] = []
        mary_metrics: dict[str, float] | None = None

        mary_xml = reference_dir / f"mary-{sample_id}.phonemes.xml"
        mary_wav = reference_dir / f"mary-{sample_id}.wav"
        if mary_xml.exists():
            mary_tokens, mary_unknown = parse_mary_phonemes(mary_xml)
            mary_distance, mary_score = score_alignment(expected, mary_tokens)
        mary_family_counts = classify_tokens(mary_tokens)
        if mary_wav.exists():
            mary_metrics = audio_metrics(mary_wav)

        rows.append(
            {
                "id": sample_id,
                "focus": item["focus"],
                "text": text,
                "expected": expected,
                "extracted": extracted,
                "expected_family_counts": expected_family_counts,
                "extracted_family_counts": extracted_family_counts,
                "distance": distance,
                "score": extraction_score,
                "extractor_unknown": extractor_unknown,
                "our_metrics": our_metrics,
                "mary_tokens": mary_tokens,
                "mary_family_counts": mary_family_counts,
                "mary_distance": mary_distance,
                "mary_score": mary_score,
                "mary_unknown": mary_unknown,
                "mary_metrics": mary_metrics,
            }
        )

    average_score = sum(float(row["score"]) for row in rows) / len(rows)
    low_rows = sorted(rows, key=lambda row: float(row["score"]))[:3]

    lines: list[str] = [
        "# Analyse d'extraction phonétique",
        "",
        f"- Généré le : `{generated_at.isoformat(timespec='seconds')}`",
        f"- Commit Git : `{git_commit}`",
        f"- Worktree modifié : `{'oui' if git_dirty else 'non'}`",
        f"- Extracteur : `{'vosk (word+CMUdict, grammar-constrained)' if use_vosk else extractor_exe.relative_to(root)}`",
        f"- Dossier de sortie : `{out_dir.relative_to(root) if out_dir.is_relative_to(root) else out_dir}`",
        f"- Score moyen d'extraction : `{percent(average_score)}`",
        "",
        "## Synthèse",
        "",
        "| Échantillon | Focus | Score | Distance | Attendus | Reconnus | Durée | ZCR | Score Mary | Durée Mary |",
        "|---|---|---:|---:|---:|---:|---:|---:|---:|---:|",
    ]

    for row in rows:
        our_metrics = row["our_metrics"]
        mary_metrics = row["mary_metrics"]
        mary_score = row["mary_score"]
        mary_duration = f"{mary_metrics['duration_s']:.3f}s" if mary_metrics is not None else "-"
        lines.append(
            "| "
            f"`{row['id']}` | "
            f"{row['focus']} | "
            f"{percent(float(row['score']))} | "
            f"{row['distance']} | "
            f"{len(row['expected'])} | "
            f"{len(row['extracted'])} | "
            f"{our_metrics['duration_s']:.3f}s | "
            f"{metric(our_metrics['zcr_mean'])} | "
            f"{percent(mary_score) if mary_score is not None else '-'} | "
            f"{mary_duration} |"
        )

    family_rows = extract_family_rows(rows)
    lines.extend(
        [
            "",
            "## Diagnostic par familles phonétiques",
            "",
            "Ces couvertures comparent des familles larges de phonèmes, sans alignement temporel. Elles ne remplacent pas le score strict, mais indiquent si l'extracteur entend au moins la bonne classe acoustique.",
            "",
            "| Échantillon | Focus | Famille | Attendus | Reconnus | Couverture | Mary | Couverture Mary |",
            "|---|---|---|---:|---:|---:|---:|---:|",
        ]
    )
    for family_row in family_rows:
        coverage = family_row["coverage"]
        mary_coverage = family_row["mary_coverage"]
        lines.append(
            "| "
            f"`{family_row['id']}` | "
            f"{family_row['focus']} | "
            f"{family_row['label']} | "
            f"{family_row['expected']} | "
            f"{family_row['extracted'] or '-'} | "
            f"{percent(coverage) if coverage is not None else '-'} | "
            f"{family_row['mary'] if family_row['mary'] is not None else '-'} | "
            f"{percent(mary_coverage) if mary_coverage is not None else '-'} |"
        )

    lines.extend(
        [
            "",
            "## Scores les plus faibles",
            "",
        ]
    )
    for row in low_rows:
        lines.extend(
            [
                f"### `{row['id']}` - {row['focus']}",
                "",
                f"- Texte : `{row['text']}`",
                f"- Score: `{percent(float(row['score']))}`",
                f"- Distance d'édition : `{row['distance']}`",
                f"- Attendu : `{format_tokens(row['expected'])}`",
                f"- Reconnu : `{format_tokens(row['extracted'])}`",
                f"- Piste d'itération : {recommendation_for_row(row)}",
                "",
            ]
        )

    lines.extend(
        [
            "## Vérité terrain MaryTTS",
            "",
            "MaryTTS est utilisée comme vérité terrain pragmatique et perfectible lorsque `mary-*.phonemes.xml` et `mary-*.wav` sont disponibles dans `bin/reference-en`.",
            "Elle aide à séparer les différences de phonémisation des échecs de reconnaissance acoustique.",
            "",
            "## Métriques audio",
            "",
            "| Échantillon | RMS | ZCR | diff_mean | diff_max | RMS Mary | ZCR Mary |",
            "|---|---:|---:|---:|---:|---:|---:|",
        ]
    )
    for row in rows:
        our_metrics = row["our_metrics"]
        mary_metrics = row["mary_metrics"]
        lines.append(
            "| "
            f"`{row['id']}` | "
            f"{metric(our_metrics['mean_rms'])} | "
            f"{metric(our_metrics['zcr_mean'])} | "
            f"{metric(our_metrics['diff_mean'])} | "
            f"{metric(our_metrics['diff_max'])} | "
            f"{metric(mary_metrics['mean_rms']) if mary_metrics is not None else '-'} | "
            f"{metric(mary_metrics['zcr_mean']) if mary_metrics is not None else '-'} |"
        )

    lines.extend(
        [
            "",
            "## Tokens inconnus",
            "",
        ]
    )
    any_unknown = False
    for row in rows:
        extractor_unknown = sorted(set(row["extractor_unknown"]))
        mary_unknown = sorted(set(row["mary_unknown"]))
        if extractor_unknown or mary_unknown:
            any_unknown = True
            lines.append(
                f"- `{row['id']}`: extractor=`{' '.join(extractor_unknown) or '-'}`, "
                f"mary=`{' '.join(mary_unknown) or '-'}`"
            )
    if not any_unknown:
        lines.append("- Aucun")

    lines.extend(
        [
            "",
            "## Artefacts",
            "",
            f"- Derniers WAV/debug/extractions : `{latest_dir.relative_to(root) if latest_dir.is_relative_to(root) else latest_dir}`",
        ]
    )

    report_path = out_dir / "report.md"
    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    history_path: pathlib.Path | None = None
    if args.history:
        history_name = f"{generated_at.strftime('%Y%m%d-%H%M%S')}-{git_commit}.md"
        history_path = history_dir / history_name
        shutil.copy2(report_path, history_path)

    return report_path, history_path


def main() -> int:
    parser = argparse.ArgumentParser(description="Analyse les WAV lib-say avec phonemes_extractor.")
    parser.add_argument("--out-dir", default="bin/reference-en/phoneme-extraction", help="Dossier de sortie du rapport et des artefacts.")
    parser.add_argument("--lang", default="en", choices=["en"], help="Corpus de langue à analyser.")
    parser.add_argument("--extractor", default="legacy", choices=["legacy", "vosk"],
                        help="Extracteur phonétique : legacy=phonemes.exe (CTC acoustique), vosk=Vosk ASR + CMUdict.")
    parser.add_argument("--no-history", dest="history", action="store_false", help="Ne copie pas le rapport dans history/.")
    parser.set_defaults(history=True)
    args = parser.parse_args()

    try:
        report_path, history_path = analyze(args)
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    print(f"Wrote {report_path}")
    if history_path is not None:
        print(f"Wrote {history_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
