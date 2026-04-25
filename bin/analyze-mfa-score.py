"""
MFA Duration Similarity Score (DSS) for lib-say vs MaryTTS.

Reads TextGrid files produced by bin/run-mfa-analysis.py.
Computes per-sample and global DSS: mean of min(r, 1/r) where r = ours/mary
duration for each aligned phoneme pair. 100% = perfect timing match.

Usage: python bin/analyze-mfa-score.py [--regen]
  --regen  re-run MFA alignment before scoring (default: use cached TextGrids)
"""

import csv, math, os, re, subprocess, sys
from pathlib import Path
from datetime import datetime, timezone

sys.stdout.reconfigure(encoding="utf-8")

REPO_ROOT = Path(__file__).resolve().parent.parent
OUT_DIR   = REPO_ROOT / "bin" / "reference-en" / "mfa-analysis"
ALIGN_DIR = OUT_DIR / "aligned"
REPORT    = OUT_DIR / "score.md"

CORPUS = [
    ("01-demo",           "Hello from lib-say. This is an English demo sentence."),
    ("02-hamlet",         "To be or not to be, that is the question."),
    ("03-sibilants",      "She sells seashells by the seashore."),
    ("04-dentals",        "This thing is worth the effort."),
    ("05-voiced-dentals", "Those feathers gather there."),
    ("06-affricates",     "Judge the changing church bells."),
    ("07-stress",         "English fricatives shift sharply."),
    ("08-finals",         "Leave these clothes and bags."),
]

IPA_TO_ARPA = {
    "i": "IY", "iː": "IY", "e": "EH", "ɛ": "EH", "æ": "AE",
    "a": "AH", "ɑ": "AA", "ɔ": "AO", "o": "OW", "u": "UW", "uː": "UW",
    "ʊ": "UH", "ɪ": "IH", "ə": "AH", "ɐ": "AH", "ʌ": "AH",
    "eʲ": "EY", "əw": "OW", "ɑɪ": "AY", "aj": "AY",
    "ɑʊ": "AW", "aw": "AW", "ɔɪ": "OY",
    "ɜː": "ER", "ɚ": "ER",
    "p": "P", "b": "B", "t": "T", "d": "D", "k": "K", "g": "G", "ɡ": "G",
    "f": "F", "v": "V", "s": "S", "z": "Z",
    "ʃ": "SH", "ʒ": "ZH", "θ": "TH", "ð": "DH", "d̪": "DH",
    "tʃ": "CH", "dʒ": "JH", "ɟ": "JH", "tʃʡ": "CH", "dʒʡ": "JH",
    "m": "M", "n": "N", "ŋ": "NG", "ɲ": "N",
    "l": "L", "ʎ": "L", "ɹ": "R", "ɾ": "R",
    "h": "HH", "ɦ": "HH", "j": "Y", "w": "W",
    "spn": "SPN", "": "SIL",
}

FAMILIES = {
    "Sibilants":  {"S", "Z", "SH", "ZH"},
    "Dentals":    {"TH", "DH"},
    "Affricates": {"CH", "JH"},
    "VoicedFric": {"V", "DH", "ZH"},
    "Vowels":     {"IY","IH","EH","AE","AA","AO","AH","OW","OY","AW","AY","ER","UH","UW","EY"},
    "Nasals":     {"M", "N", "NG"},
    "Stops":      {"P", "B", "T", "D", "K", "G"},
    "Liquids":    {"L", "R"},
}

PAUSE_TOKENS = {"SPN", "SIL", "SP"}


def to_arpa(ipa: str) -> str:
    return IPA_TO_ARPA.get(ipa, ipa.upper()[:4] if ipa else "SIL")


def parse_phones(tg_path: Path) -> list[tuple[str, float]]:
    """Return [(arpa, duration_ms), ...] for non-silence phones."""
    txt = tg_path.read_text(encoding="utf-8", errors="replace")
    parts = re.split(r'name = "phones"', txt)
    if len(parts) < 2:
        return []
    block = parts[1]
    ivs = re.findall(r'xmin = ([\d.]+)\s+xmax = ([\d.]+)\s+text = "([^"]*)"', block)
    phones = []
    for xmin, xmax, ph in ivs:
        dur = (float(xmax) - float(xmin)) * 1000.0
        arpa = to_arpa(ph)
        if arpa == "SIL" or dur < 3:
            continue
        phones.append((arpa, dur))
    return phones


def pair_sim(ours_ms: float, mary_ms: float) -> float:
    """Duration similarity: 1.0 = perfect, 0.5 = 2:1 mismatch."""
    if mary_ms < 1:
        return 0.0
    r = ours_ms / mary_ms
    return min(r, 1.0 / r)


def read_llh(tag: str, spk: str) -> float | None:
    """Read speech_log_likelihood / duration from alignment CSV."""
    csv_path = ALIGN_DIR / f"{spk}-{tag}" / "alignment_analysis.csv"
    if not csv_path.exists():
        return None
    with csv_path.open(encoding="utf-8") as f:
        for row in csv.DictReader(f):
            dur = float(row["end"]) - float(row["begin"])
            if dur < 0.01:
                continue
            return float(row["speech_log_likelihood"]) / dur
    return None


def family_of(arpa: str) -> str:
    for fam, members in FAMILIES.items():
        if arpa in members:
            return fam
    return "Other"


def main() -> None:
    regen = "--regen" in sys.argv

    if regen:
        print("Re-running MFA alignment...")
        subprocess.run(
            [r"C:\tools\miniforge3\Scripts\conda.exe", "run", "-n", "mfa",
             "python", str(REPO_ROOT / "bin" / "run-mfa-analysis.py")],
            check=True,
        )

    lines: list[str] = []
    now = datetime.now(timezone.utc).astimezone().isoformat(timespec="seconds")
    lines += [
        "# MFA Duration Similarity Score",
        "",
        f"- Generated: `{now}`",
        f"- Metric: DSS = mean( min(r, 1/r) ) per phoneme pair, r = ours/mary duration",
        f"- 100% = perfect timing match with MaryTTS reference",
        "",
    ]

    sample_scores: list[float] = []
    all_pairs: list[tuple[str, str, float, float]] = []  # tag, arpa, ours_ms, mary_ms

    lines.append("## Per-sample results")
    lines.append("")
    lines.append("| Sample | Text | DSS | Pairs | LLH ours/s | LLH mary/s |")
    lines.append("|---|---|---:|---:|---:|---:|")

    per_sample_details: list[tuple[str, list]] = []

    for tag, text in CORPUS:
        og = ALIGN_DIR / f"ours-{tag}" / f"ours-{tag}.TextGrid"
        mg = ALIGN_DIR / f"mary-{tag}" / f"mary-{tag}.TextGrid"
        if not og.exists() or not mg.exists():
            lines.append(f"| `{tag}` | {text} | — | — | — | — |")
            continue

        o_phones = parse_phones(og)
        m_phones = parse_phones(mg)

        pairs = list(zip(o_phones, m_phones))
        if not pairs:
            continue

        sims = [pair_sim(o[1], m[1]) for o, m in pairs]
        dss = sum(sims) / len(sims)
        sample_scores.append(dss)

        for (a, a_ms), (b, b_ms) in pairs:
            all_pairs.append((tag, a, a_ms, b_ms))

        o_llh = read_llh(tag, "ours")
        m_llh = read_llh(tag, "mary")
        llh_str_o = f"{o_llh:.2f}" if o_llh is not None else "—"
        llh_str_m = f"{m_llh:.2f}" if m_llh is not None else "—"

        lines.append(
            f"| `{tag}` | {text} | **{dss*100:.1f}%** | {len(pairs)} | {llh_str_o} | {llh_str_m} |"
        )
        per_sample_details.append((tag, pairs))

    global_dss = sum(sample_scores) / len(sample_scores) if sample_scores else 0.0
    lines.append("")
    lines.append(f"**Global DSS: {global_dss*100:.1f}%**")
    lines.append("")

    # ── Per-sample phoneme tables ─────────────────────────────────────────────
    lines.append("## Per-sample phoneme pairs")
    lines.append("")
    for tag, pairs in per_sample_details:
        text = next(t for s, t in CORPUS if s == tag)
        lines.append(f"### `{tag}` — {text}")
        lines.append("")
        lines.append("| ARPA | ours (ms) | mary (ms) | ratio | sim | family |")
        lines.append("|---|---:|---:|---:|---:|---|")
        for (arpa, o_ms), (_, m_ms) in pairs:
            r = o_ms / m_ms if m_ms > 0 else 0
            sim = pair_sim(o_ms, m_ms)
            flag = " ⚠" if sim < 0.5 else ""
            fam = family_of(arpa)
            lines.append(
                f"| {arpa} | {o_ms:.0f} | {m_ms:.0f} | {r:.2f} | {sim*100:.0f}%{flag} | {fam} |"
            )
        sims = [pair_sim(o[1], m[1]) for o, m in pairs]
        lines.append(f"| **DSS** | | | | **{sum(sims)/len(sims)*100:.1f}%** | |")
        lines.append("")

    # ── Family summary ────────────────────────────────────────────────────────
    lines.append("## Family summary")
    lines.append("")
    lines.append("| Family | N | Mean sim | Mean ratio |")
    lines.append("|---|---:|---:|---:|")

    family_data: dict[str, list[tuple[float, float]]] = {}
    for tag, arpa, o_ms, m_ms in all_pairs:
        fam = family_of(arpa)
        r = o_ms / m_ms if m_ms > 0 else 0
        sim = pair_sim(o_ms, m_ms)
        family_data.setdefault(fam, []).append((sim, r))

    for fam in ["Vowels", "Sibilants", "Stops", "Nasals", "Liquids",
                "Dentals", "VoicedFric", "Affricates", "Other"]:
        items = family_data.get(fam, [])
        if not items:
            continue
        mean_sim = sum(s for s, _ in items) / len(items)
        mean_r   = sum(r for _, r in items) / len(items)
        lines.append(f"| {fam} | {len(items)} | {mean_sim*100:.1f}% | {mean_r:.2f} |")

    lines.append("")

    # ── Improvement leads ─────────────────────────────────────────────────────
    lines.append("## Improvement leads (sim < 60%)")
    lines.append("")
    lines.append("| ARPA | Sample | ours (ms) | mary (ms) | ratio | sim | family |")
    lines.append("|---|---|---:|---:|---:|---:|---|")

    leads = []
    for tag, arpa, o_ms, m_ms in all_pairs:
        sim = pair_sim(o_ms, m_ms)
        if sim < 0.60:
            r = o_ms / m_ms if m_ms > 0 else 0
            leads.append((sim, tag, arpa, o_ms, m_ms, r))

    leads.sort()
    for sim, tag, arpa, o_ms, m_ms, r in leads:
        direction = "too long" if r > 1 else "too short"
        fam = family_of(arpa)
        lines.append(
            f"| {arpa} | `{tag}` | {o_ms:.0f} | {m_ms:.0f} | {r:.2f} | {sim*100:.0f}% | {fam} |"
        )
        lines.append(
            f"| | | | | | → {direction} | |"
        )

    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Report written to {REPORT}")
    print(f"\nGlobal DSS: {global_dss*100:.1f}%")
    print(f"Per sample: " + ", ".join(
        f"{tag}: {s*100:.1f}%" for (tag, _), s in zip(CORPUS, sample_scores)
    ))


if __name__ == "__main__":
    main()
