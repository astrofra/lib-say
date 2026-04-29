"""
MFA forced-alignment analysis for lib-say vs MaryTTS.

Usage: python bin/run-mfa-analysis.py

Runs MFA on all ours-*.wav and mary-*.wav, parses TextGrids,
produces a phoneme-duration comparison report.
"""

import os, re, subprocess, sys, shutil, csv, math
from pathlib import Path

# ── config ──────────────────────────────────────────────────────────────────
REPO_ROOT   = Path(__file__).resolve().parent.parent
REF_DIR     = REPO_ROOT / "bin" / "reference-en"
OUT_DIR     = REF_DIR / "mfa-analysis"
CORPUS_DIR  = OUT_DIR / "corpus"
ALIGN_DIR   = OUT_DIR / "aligned"
MFA_CONDA_ENV = "mfa"
SAMPLE_RATE = 22050


def resolve_conda_exe():
    candidates = [
        os.environ.get("LIBSAY_CONDA_EXE"),
        os.environ.get("CONDA_EXE"),
        r"S:\tools\miniforge3\Scripts\conda.exe",
        r"C:\tools\miniforge3\Scripts\conda.exe",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).exists():
            return candidate
    raise FileNotFoundError(
        "Unable to find conda.exe. Set LIBSAY_CONDA_EXE to the full conda.exe path."
    )


MFA_EXE = resolve_conda_exe()

if not os.environ.get("MFA_ROOT_DIR") and Path(r"S:\tools\mfa-data").exists():
    os.environ["MFA_ROOT_DIR"] = r"S:\tools\mfa-data"

CORPUS = [
    ("01-demo",          "Hello from lib-say. This is an English demo sentence."),
    ("02-hamlet",        "To be or not to be, that is the question."),
    ("03-sibilants",     "She sells seashells by the seashore."),
    ("04-dentals",       "This thing is worth the effort."),
    ("05-voiced-dentals","Those feathers gather there."),
    ("06-affricates",    "Judge the changing church bells."),
    ("07-stress",        "English fricatives shift sharply."),
    ("08-finals",        "Leave these clothes and bags."),
]

# IPA → ARPAbet broad mapping (MFA english_mfa model phones → familiar labels)
# Covers the phones we care about most; everything else mapped to itself.
IPA_TO_ARPABET = {
    # vowels
    "i": "IY", "e": "EH", "ɛ": "EH", "æ": "AE", "a": "AH",
    "ɑ": "AA", "ɔ": "AO", "o": "OW", "u": "UW", "ʊ": "UH",
    "ɪ": "IH", "ə": "AH", "ɐ": "AH", "ʌ": "AH",
    "əw": "OW", "ɑɪ": "AY", "ɑʊ": "AW", "ɔɪ": "OY",
    "ej": "EY", "iː": "IY", "uː": "UW",
    # consonants
    "p": "P", "b": "B", "t": "T", "d": "D", "k": "K", "ɡ": "G",
    "f": "F", "v": "V", "s": "S", "z": "Z",
    "ʃ": "SH", "ʒ": "ZH",
    "θ": "TH", "ð": "DH",
    "d̪": "DH",          # dental d — often DH in fast speech
    "tʃ": "CH", "dʒ": "JH",
    "tʃ͡": "CH", "d͡ʒ": "JH",   # tie-bar affricates
    "m": "M", "n": "N", "ŋ": "NG",
    "ɲ": "N",            # palatal nasal (pre-/j/ allophone)
    "ɟ": "JH",           # palatal stop (English → often JH)
    "ʎ": "L",            # palatal lateral
    "l": "L", "ɹ": "R", "j": "Y", "w": "W",
    "h": "HH",
    "spn": "SPN",        # speaker noise / silence
    "": "SIL",
}

# Phoneme families for summary
FAMILIES = {
    "Sibilants":  {"S", "Z", "SH", "ZH"},
    "Dentals":    {"TH", "DH"},
    "Affricates": {"CH", "JH"},
    "VoicedFric": {"V", "DH", "ZH"},
    "Vowels":     {"IY","IH","EH","AE","AH","AA","AO","OW","UW","UH","ER","AY","AW","OY","EY"},
    "Nasals":     {"M","N","NG"},
    "Stops":      {"P","B","T","D","K","G"},
    "Liquids":    {"L","R"},
}

# ── helpers ─────────────────────────────────────────────────────────────────

def mfa(*args):
    cmd = [MFA_EXE, "run", "-n", MFA_CONDA_ENV, "mfa"] + list(args)
    r = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
    return r

def parse_textgrid(path):
    """Return list of (xmin, xmax, phone_ipa, phone_arpa) for the phones tier."""
    txt = Path(path).read_text(encoding="utf-8", errors="replace")
    # find phones tier
    phones_block = re.split(r'name = "phones"', txt)
    if len(phones_block) < 2:
        return []
    block = phones_block[1]
    intervals = re.findall(
        r'xmin = ([\d.]+)\s+xmax = ([\d.]+)\s+text = "([^"]*)"',
        block
    )
    result = []
    for xmin, xmax, ipa in intervals:
        arpa = IPA_TO_ARPABET.get(ipa, ipa.upper() if ipa else "SIL")
        result.append((float(xmin), float(xmax), ipa, arpa))
    return result

def phone_duration_ms(phones):
    return {(ipa, arpa): (xmax - xmin) * 1000
            for (xmin, xmax, ipa, arpa) in phones if ipa not in ("", "spn")}

def group_by_arpa(phones):
    """Aggregate durations by ARPAbet label (sum across multiple occurrences)."""
    d = {}
    for (xmin, xmax, ipa, arpa) in phones:
        if arpa in ("SIL", "SPN"):
            continue
        dur = (xmax - xmin) * 1000
        d.setdefault(arpa, []).append(dur)
    return d

def fmt(ms):
    if ms is None:
        return "   —  "
    return f"{ms:6.1f}"

def ratio_bar(r):
    if r is None:
        return ""
    stars = min(int(r * 4), 12)
    return "█" * stars if r >= 0.5 else "▒" * max(1, int(r * 8))

# ── main ─────────────────────────────────────────────────────────────────────

def build_corpus():
    """Create corpus dirs: one per (speaker, sample), with WAV + .lab."""
    shutil.rmtree(CORPUS_DIR, ignore_errors=True)
    for tag, transcript in CORPUS:
        for speaker in ("ours", "mary"):
            wav_src = REF_DIR / f"{speaker}-{tag}.wav"
            if not wav_src.exists():
                continue
            dest_dir = CORPUS_DIR / f"{speaker}-{tag}"
            dest_dir.mkdir(parents=True, exist_ok=True)
            shutil.copy(wav_src, dest_dir / f"{speaker}-{tag}.wav")
            (dest_dir / f"{speaker}-{tag}.lab").write_text(transcript, encoding="utf-8")

def run_alignment():
    """Run MFA on each corpus sub-dir separately (avoids cross-contamination)."""
    shutil.rmtree(ALIGN_DIR, ignore_errors=True)
    ALIGN_DIR.mkdir(parents=True, exist_ok=True)
    for corpus_subdir in sorted(CORPUS_DIR.iterdir()):
        name = corpus_subdir.name
        out  = ALIGN_DIR / name
        print(f"  aligning {name} ...", end=" ", flush=True)
        r = mfa("align", str(corpus_subdir), "english_us_mfa", "english_mfa",
                str(out), "--clean", "--quiet")
        if r.returncode == 0:
            print("OK")
        else:
            print("FAIL")
            try:
                print(r.stderr[-600:] if r.stderr else "(no stderr)")
            except UnicodeEncodeError:
                print(r.stderr[-600:].encode("ascii", "replace").decode("ascii") if r.stderr else "(no stderr)")

def load_alignments():
    """Return dict: key=(speaker, tag) → list of phone intervals."""
    result = {}
    for tag, _ in CORPUS:
        for speaker in ("ours", "mary"):
            tg = ALIGN_DIR / f"{speaker}-{tag}" / f"{speaker}-{tag}.TextGrid"
            if tg.exists():
                result[(speaker, tag)] = parse_textgrid(tg)
    return result

def load_alignment_scores():
    """Return dict: key=(speaker, tag) → mean log-likelihood from analysis CSV."""
    scores = {}
    for tag, _ in CORPUS:
        for speaker in ("ours", "mary"):
            csv_path = ALIGN_DIR / f"{speaker}-{tag}" / "alignment_analysis.csv"
            if not csv_path.exists():
                continue
            try:
                with open(csv_path, newline="", encoding="utf-8") as f:
                    rows = list(csv.DictReader(f))
                lls = [float(r["log_likelihood"]) for r in rows if r.get("log_likelihood")]
                if lls:
                    scores[(speaker, tag)] = sum(lls) / len(lls)
            except Exception:
                pass
    return scores

def report(alignments, scores):
    lines = []
    W = 90
    lines.append("=" * W)
    lines.append("MFA FORCED-ALIGNMENT ANALYSIS — lib-say vs MaryTTS")
    lines.append("=" * W)
    lines.append("")
    lines.append("Phoneme durations in milliseconds (mean when multiple occurrences).")
    lines.append("Ratio = ours / mary.  < 0.5 flagged with ▒, >= 0.5 flagged with █")
    lines.append("")

    # ── per-sample tables ──
    for tag, transcript in CORPUS:
        ours  = alignments.get(("ours",  tag))
        mary  = alignments.get(("mary",  tag))
        if not ours or not mary:
            lines.append(f"[{tag}] — alignment missing for one or both speakers")
            continue

        o_score = scores.get(("ours",  tag))
        m_score = scores.get(("mary",  tag))

        lines.append(f"{'─'*W}")
        lines.append(f"  [{tag}]  \"{transcript}\"")
        score_str = ""
        if o_score is not None and m_score is not None:
            score_str = f"  align-LL: ours={o_score:.2f}  mary={m_score:.2f}"
        lines.append(f"  Phoneme count: ours={len([p for p in ours if p[2] not in ('','spn')])}  "
                     f"mary={len([p for p in mary if p[2] not in ('','spn')])}"
                     + score_str)
        lines.append("")

        o_groups = group_by_arpa(ours)
        m_groups = group_by_arpa(mary)
        all_arpa = sorted(set(o_groups) | set(m_groups))

        # find interesting phonemes: short in ours, or big ratio diff
        rows = []
        for arpa in all_arpa:
            o_vals = o_groups.get(arpa, [])
            m_vals = m_groups.get(arpa, [])
            o_mean = sum(o_vals) / len(o_vals) if o_vals else None
            m_mean = sum(m_vals) / len(m_vals) if m_vals else None
            ratio  = (o_mean / m_mean) if (o_mean and m_mean) else None
            rows.append((arpa, o_mean, m_mean, ratio, len(o_vals), len(m_vals)))

        # sort: worst ratio first (shortest ours vs mary)
        rows.sort(key=lambda r: (r[3] if r[3] is not None else 99))

        lines.append(f"  {'ARPA':<6} {'ours(ms)':>9} {'mary(ms)':>9} {'ratio':>6}  bar          note")
        for arpa, o_ms, m_ms, ratio, o_cnt, m_cnt in rows:
            note = ""
            if o_ms is not None and o_ms < 25:
                note = "⚠ very short"
            elif ratio is not None and ratio < 0.40:
                note = "⚠ < 40% of reference"
            elif ratio is not None and ratio > 2.5:
                note = "↑ too long"
            # find family
            for fam, members in FAMILIES.items():
                if arpa in members:
                    note += f"  [{fam}]"
                    break
            bar = ratio_bar(ratio)
            lines.append(f"  {arpa:<6} {fmt(o_ms):>9} {fmt(m_ms):>9} "
                         f"{(f'{ratio:.2f}' if ratio else '  —  '):>6}  {bar:<12} {note}")
        lines.append("")

    # ── phoneme family summary across all samples ──
    lines.append("=" * W)
    lines.append("  PHONEME FAMILY SUMMARY  (mean ratio ours/mary across all samples)")
    lines.append("=" * W)
    lines.append("")

    family_data = {fam: [] for fam in FAMILIES}
    for tag, _ in CORPUS:
        ours = alignments.get(("ours", tag))
        mary = alignments.get(("mary", tag))
        if not ours or not mary:
            continue
        og = group_by_arpa(ours)
        mg = group_by_arpa(mary)
        for fam, members in FAMILIES.items():
            for arpa in members:
                if arpa in og and arpa in mg:
                    o_m = sum(og[arpa]) / len(og[arpa])
                    m_m = sum(mg[arpa]) / len(mg[arpa])
                    if m_m > 0:
                        family_data[fam].append(o_m / m_m)

    lines.append(f"  {'Family':<16} {'N':>4}  {'mean ratio':>10}  bar")
    for fam, ratios in sorted(family_data.items(), key=lambda x: (sum(x[1])/len(x[1])) if x[1] else 99):
        if not ratios:
            lines.append(f"  {fam:<16} {'—':>4}  {'no data':>10}")
            continue
        mr = sum(ratios) / len(ratios)
        lines.append(f"  {fam:<16} {len(ratios):>4}  {mr:>10.2f}  {ratio_bar(mr)}")
    lines.append("")

    # ── lead list ──
    lines.append("=" * W)
    lines.append("  IMPROVEMENT LEADS")
    lines.append("=" * W)
    lines.append("")

    leads = []
    seen = set()
    for tag, transcript in CORPUS:
        ours = alignments.get(("ours", tag))
        mary = alignments.get(("mary", tag))
        if not ours or not mary:
            continue
        og = group_by_arpa(ours)
        mg = group_by_arpa(mary)
        for arpa in set(og) | set(mg):
            o_vals = og.get(arpa, [])
            m_vals = mg.get(arpa, [])
            o_mean = sum(o_vals)/len(o_vals) if o_vals else None
            m_mean = sum(m_vals)/len(m_vals) if m_vals else None
            if not o_mean or not m_mean:
                continue
            ratio = o_mean / m_mean
            key = arpa
            if ratio < 0.35 and key not in seen:
                seen.add(key)
                leads.append((ratio, arpa, o_mean, m_mean, tag))

    leads.sort()
    for ratio, arpa, o_ms, m_ms, tag in leads:
        fam = next((f for f, m in FAMILIES.items() if arpa in m), "—")
        lines.append(f"  [{arpa}] ratio={ratio:.2f}  ours={o_ms:.0f}ms  mary={m_ms:.0f}ms"
                     f"  first seen in [{tag}]  family={fam}")
        if ratio < 0.15:
            lines.append(f"      → nearly absent: synthesizer barely produces this phoneme")
        elif ratio < 0.35:
            lines.append(f"      → too short: extractor likely misses it below ~{m_ms*0.5:.0f}ms threshold")

    lines.append("")
    return "\n".join(lines)


if __name__ == "__main__":
    print("Building corpus...")
    build_corpus()

    print("Running MFA alignment (this takes ~2 min)...")
    run_alignment()

    print("Parsing TextGrids...")
    alignments = load_alignments()
    scores     = load_alignment_scores()

    print(f"  Loaded {len(alignments)} TextGrids")

    report_txt = report(alignments, scores)
    out_path = OUT_DIR / "report.txt"
    out_path.write_text(report_txt, encoding="utf-8")
    print(f"\nReport written to {out_path}\n")
    print(report_txt)
