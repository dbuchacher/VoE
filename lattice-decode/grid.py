#!/usr/bin/env python3
"""
Grid recovery: can distributional coordinates recover the known
Linear B syllabary grid (consonant onset × vowel nucleus)?

If yes → apply same clustering to Linear A to reveal its grid.

Linear B grid (known):
       a    e    i    o    u
  d   da   de   di   do   du
  k   ka   ke   ki   ko   ku
  ...etc

The test: signs sharing an onset (da/de/di/do/du) should cluster
on some coordinate axes. Signs sharing a vowel (da/ka/ma/na/pa...)
should cluster on others.
"""

import re, csv, sys
from collections import Counter, defaultdict
from math import log, sqrt
import json

# ─── Known Linear B phonetic grid ───

LINB_ONSET = {
    'DA':'d','DE':'d','DI':'d','DO':'d','DU':'d',
    'KA':'k','KE':'k','KI':'k','KO':'k','KU':'k',
    'MA':'m','ME':'m','MI':'m','MO':'m','MU':'m',
    'NA':'n','NE':'n','NI':'n','NO':'n','NU':'n',
    'PA':'p','PE':'p','PI':'p','PO':'p','PU':'p',
    'RA':'r','RE':'r','RI':'r','RO':'r','RU':'r',
    'SA':'s','SE':'s','SI':'s','SO':'s','SU':'s',
    'TA':'t','TE':'t','TI':'t','TO':'t','TU':'t',
    'WA':'w','WE':'w','WI':'w','WO':'w',
    'JA':'j','JE':'j','JO':'j','JU':'j',
    'QA':'q','QE':'q','QI':'q','QO':'q',
    'ZA':'z','ZE':'z','ZO':'z','ZU':'z',
    'A':'V','E':'V','I':'V','O':'V','U':'V',  # pure vowels
}

LINB_VOWEL = {}
for sign in LINB_ONSET:
    if len(sign) == 1:
        LINB_VOWEL[sign] = sign.lower()
    elif len(sign) == 2:
        LINB_VOWEL[sign] = sign[1].lower()


# ─── Extract functions (from extract.py) ───

def extract_linear_a():
    with open('lineara/LinearAInscriptions.js') as f:
        text = f.read()
    blocks = re.findall(
        r'\["([^"]+)",\{.*?"transliteratedWords":\s*\[(.*?)\]',
        text, re.DOTALL
    )
    all_words = []
    for name, block in blocks:
        words = re.findall(r'"([A-Z][A-Z0-9₂₃-]*)"', block)
        words = [w for w in words if not w.isdigit() and len(w) > 0]
        all_words.extend(words)
    return all_words


def extract_linear_b():
    words = []
    with open('linearb/tablet-sets/tablets.csv', newline='') as f:
        reader = csv.reader(f, delimiter=';')
        for row in reader:
            if len(row) >= 4:
                for w in row[3].split(','):
                    w = w.strip()
                    if w and not w.isdigit() and '-' in w:
                        words.append(w.upper())
    return words


def compute_signs(words):
    """Compute full distributional features for each sign."""
    sign_freq = Counter()
    sign_start = Counter()
    sign_end = Counter()
    sign_middle = Counter()
    sign_word_lens = defaultdict(list)
    sign_neighbors = defaultdict(Counter)
    sign_prev = defaultdict(Counter)
    sign_next = defaultdict(Counter)

    for word in words:
        parts = word.split('-')
        n = len(parts)
        for i, sign in enumerate(parts):
            sign_freq[sign] += 1
            sign_word_lens[sign].append(n)
            if i == 0: sign_start[sign] += 1
            elif i == n-1: sign_end[sign] += 1
            else: sign_middle[sign] += 1
            if i < n-1:
                sign_next[sign][parts[i+1]] += 1
                sign_neighbors[sign][parts[i+1]] += 1
            if i > 0:
                sign_prev[sign][parts[i-1]] += 1
                sign_neighbors[sign][parts[i-1]] += 1

    total = sum(sign_freq.values())
    max_freq = max(sign_freq.values()) if sign_freq else 1

    signs = {}
    for sign in sign_freq:
        freq = sign_freq[sign]
        s = sign_start[sign]
        m = sign_middle[sign]
        e = sign_end[sign]
        tp = s + m + e or 1

        avg_len = sum(sign_word_lens[sign]) / len(sign_word_lens[sign])

        # Entropy of following signs (high = versatile)
        next_total = sum(sign_next[sign].values()) or 1
        next_entropy = -sum(
            (c/next_total) * log(c/next_total + 1e-10)
            for c in sign_next[sign].values()
        )
        prev_total = sum(sign_prev[sign].values()) or 1
        prev_entropy = -sum(
            (c/prev_total) * log(c/prev_total + 1e-10)
            for c in sign_prev[sign].values()
        )

        signs[sign] = {
            'freq': freq,
            'freq_rank': 0,  # filled later
            'start_pct': s / tp,
            'mid_pct': m / tp,
            'end_pct': e / tp,
            'avg_word_len': avg_len,
            'n_unique_next': len(sign_next[sign]),
            'n_unique_prev': len(sign_prev[sign]),
            'next_entropy': next_entropy,
            'prev_entropy': prev_entropy,
            'n_neighbors': len(sign_neighbors[sign]),
            # Normalized coordinates
            'tau': (avg_len - 2.5) / 1.5,
            'chi': (s - e) / tp,
            'mu': (log(freq + 1) / log(max_freq + 1)) * 2 - 1,
            'phi': (len(sign_neighbors[sign]) / max(len(v) for v in sign_neighbors.values())) * 2 - 1,
        }

    # Add frequency rank
    for rank, (sign, _) in enumerate(sorted(signs.items(), key=lambda x: -x[1]['freq'])):
        signs[sign]['freq_rank'] = rank

    return signs


def test_grid_recovery(signs, onset_map, vowel_map, label=""):
    """Test whether coordinate dimensions separate onset and vowel."""
    print(f"\n{'='*80}")
    print(f"  Grid Recovery Test: {label}")
    print(f"{'='*80}")

    # Only test signs we know the phonetics of
    known = {s: signs[s] for s in signs if s in onset_map}
    print(f"  Signs with known phonetics: {len(known)}")

    # For each coordinate, compute how well it separates onset groups
    coords = ['tau', 'chi', 'mu', 'phi',
              'start_pct', 'end_pct', 'mid_pct',
              'next_entropy', 'prev_entropy', 'avg_word_len']

    print(f"\n  Onset separation (same consonant should cluster):")
    print(f"  {'coord':>15}  {'within_var':>10}  {'between_var':>11}  {'F-ratio':>8}  {'separates?':>10}")
    print(f"  {'-'*65}")

    best_onset_coords = []
    for coord in coords:
        groups = defaultdict(list)
        for sign, data in known.items():
            groups[onset_map[sign]].append(data[coord])

        # F-ratio: between-group variance / within-group variance
        grand_mean = sum(data[coord] for data in known.values()) / len(known)
        between = sum(
            len(vals) * (sum(vals)/len(vals) - grand_mean)**2
            for vals in groups.values() if vals
        ) / max(len(groups) - 1, 1)
        within = sum(
            sum((v - sum(vals)/len(vals))**2 for v in vals)
            for vals in groups.values() if len(vals) > 1
        ) / max(sum(len(v)-1 for v in groups.values() if len(v)>1), 1)

        f_ratio = between / within if within > 0.001 else 0
        good = '  ★' if f_ratio > 1.0 else ''
        print(f"  {coord:>15}  {within:>10.4f}  {between:>11.4f}  {f_ratio:>8.2f}{good}")
        best_onset_coords.append((coord, f_ratio))

    print(f"\n  Vowel separation (same vowel should cluster):")
    print(f"  {'coord':>15}  {'within_var':>10}  {'between_var':>11}  {'F-ratio':>8}  {'separates?':>10}")
    print(f"  {'-'*65}")

    best_vowel_coords = []
    for coord in coords:
        groups = defaultdict(list)
        for sign, data in known.items():
            if sign in vowel_map:
                groups[vowel_map[sign]].append(data[coord])

        grand_mean = sum(data[coord] for sign, data in known.items() if sign in vowel_map) / max(sum(1 for s in known if s in vowel_map), 1)
        between = sum(
            len(vals) * (sum(vals)/len(vals) - grand_mean)**2
            for vals in groups.values() if vals
        ) / max(len(groups) - 1, 1)
        within = sum(
            sum((v - sum(vals)/len(vals))**2 for v in vals)
            for vals in groups.values() if len(vals) > 1
        ) / max(sum(len(v)-1 for v in groups.values() if len(v)>1), 1)

        f_ratio = between / within if within > 0.001 else 0
        good = '  ★' if f_ratio > 1.0 else ''
        print(f"  {coord:>15}  {within:>10.4f}  {between:>11.4f}  {f_ratio:>8.2f}{good}")
        best_vowel_coords.append((coord, f_ratio))

    # Show actual onset clusters on best coordinate
    best_onset_coords.sort(key=lambda x: -x[1])
    best_coord = best_onset_coords[0][0]
    print(f"\n  Best onset separator: {best_coord}")
    groups = defaultdict(list)
    for sign, data in known.items():
        groups[onset_map[sign]].append((sign, data[best_coord]))
    for onset in sorted(groups):
        members = sorted(groups[onset], key=lambda x: x[1])
        vals = [f"{s}={v:.2f}" for s, v in members]
        mean = sum(v for _, v in members) / len(members)
        print(f"    {onset:>2}: mean={mean:+.3f}  {', '.join(vals)}")

    best_vowel_coords.sort(key=lambda x: -x[1])
    best_coord = best_vowel_coords[0][0]
    print(f"\n  Best vowel separator: {best_coord}")
    groups = defaultdict(list)
    for sign, data in known.items():
        if sign in vowel_map:
            groups[vowel_map[sign]].append((sign, data[best_coord]))
    for vowel in sorted(groups):
        members = sorted(groups[vowel], key=lambda x: x[1])
        vals = [f"{s}={v:.2f}" for s, v in members]
        mean = sum(v for _, v in members) / len(members)
        print(f"    {vowel}: mean={mean:+.3f}  {', '.join(vals)}")

    return best_onset_coords, best_vowel_coords


def cluster_linear_a(a_signs, best_onset_dim, best_vowel_dim):
    """Use the best-separating dimensions to propose Linear A groups."""
    print(f"\n{'='*80}")
    print(f"  Linear A — Proposed Syllabary Grid")
    print(f"  (onset clustered by {best_onset_dim}, vowel by {best_vowel_dim})")
    print(f"{'='*80}")

    # Filter to syllabic signs (exclude logograms like GRA, VIN, etc)
    syllabic = {s: d for s, d in a_signs.items()
                if len(s) <= 3 and not s.isupper()
                or (len(s) <= 2)}
    # Actually, Linear A transliterations use uppercase CV like DA, KU etc
    # Filter out obvious logograms (all-caps 3+ letters, likely commodity signs)
    syllabic = {s: d for s, d in a_signs.items()
                if not (len(s) >= 3 and s.isalpha() and s.isupper())
                or s in ('GRA','VIN','OLE','CYP','OLIV','AROM','CAP','GAL')}
    # Better: keep signs that look like syllables
    syllabic = {}
    for s, d in a_signs.items():
        # CV pattern (1-2 chars), or subscript variants, or pure vowels
        if (len(s) <= 2 and s[0].isalpha()) or '₂' in s or '₃' in s:
            syllabic[s] = d

    print(f"  Syllabic signs: {len(syllabic)}")

    # Sort by onset dimension, split into groups
    sorted_by_onset = sorted(syllabic.items(), key=lambda x: x[1][best_onset_dim])

    # K-means-ish: split into ~12-15 onset groups by natural gaps
    onset_vals = [d[best_onset_dim] for _, d in sorted_by_onset]
    n = len(onset_vals)

    # Simple approach: quantize into bins
    n_onset_bins = 13  # typical syllabary has ~12-15 onsets
    n_vowel_bins = 5   # 5 vowels

    print(f"\n  Signs sorted by onset dimension ({best_onset_dim}):")
    for sign, data in sorted_by_onset:
        vowel_val = data[best_vowel_dim]
        onset_val = data[best_onset_dim]
        print(f"    {sign:>6}  onset={onset_val:+.3f}  vowel={vowel_val:+.3f}  freq={data['freq']:>4}")

    # Also show 2D scatter
    print(f"\n  2D scatter (onset={best_onset_dim} × vowel={best_vowel_dim}):")
    print(f"  {'sign':>6}  {best_onset_dim:>8}  {best_vowel_dim:>8}  {'freq':>5}")
    print(f"  {'-'*40}")
    for sign, data in sorted(syllabic.items(),
                              key=lambda x: (x[1][best_onset_dim], x[1][best_vowel_dim])):
        print(f"  {sign:>6}  {data[best_onset_dim]:>+8.3f}  {data[best_vowel_dim]:>+8.3f}  {data['freq']:>5}")


# ─── Shared signs between Linear A and Linear B ───

def shared_signs_analysis(a_signs, b_signs):
    """Many Linear A signs share transliterations with Linear B.
    Compare their distributional properties."""
    print(f"\n{'='*80}")
    print(f"  Shared Signs: Linear A vs Linear B distributional comparison")
    print(f"{'='*80}")

    shared = set(a_signs.keys()) & set(b_signs.keys())
    shared = {s for s in shared if len(s) <= 3}  # filter logograms
    print(f"  Shared sign names: {len(shared)}")
    print(f"\n  {'sign':>6}  {'A-τ':>6} {'B-τ':>6}  {'A-χ':>6} {'B-χ':>6}  "
          f"{'A-μ':>6} {'B-μ':>6}  {'A-φ':>6} {'B-φ':>6}  {'dist':>6}")
    print(f"  {'-'*75}")

    distances = []
    for sign in sorted(shared):
        a, b = a_signs[sign], b_signs[sign]
        d = sqrt(sum((a[c]-b[c])**2 for c in ['tau','chi','mu','phi']))
        distances.append((sign, d))
        print(f"  {sign:>6}  {a['tau']:>+6.2f} {b['tau']:>+6.2f}  "
              f"{a['chi']:>+6.2f} {b['chi']:>+6.2f}  "
              f"{a['mu']:>+6.2f} {b['mu']:>+6.2f}  "
              f"{a['phi']:>+6.2f} {b['phi']:>+6.2f}  {d:>6.3f}")

    distances.sort(key=lambda x: x[1])
    print(f"\n  Most similar (same phonetic value likely):")
    for sign, d in distances[:10]:
        print(f"    {sign}: distance = {d:.3f}")
    print(f"\n  Most different (different phonetic value likely):")
    for sign, d in distances[-10:]:
        print(f"    {sign}: distance = {d:.3f}")

    avg_d = sum(d for _, d in distances) / len(distances)
    print(f"\n  Average distance: {avg_d:.3f}")
    close = sum(1 for _, d in distances if d < avg_d)
    print(f"  Signs closer than average: {close}/{len(distances)} ({close/len(distances)*100:.0f}%)")
    print(f"  → If most are close, Linear A signs likely have similar phonetic values to Linear B")
    print(f"  → If many are far, the transliteration convention masks different sounds")


# ─── Main ───

if __name__ == '__main__':
    print("Loading corpora...")
    a_words = extract_linear_a()
    b_words = extract_linear_b()
    print(f"  Linear A: {len(a_words)} words")
    print(f"  Linear B: {len(b_words)} words")

    print("\nComputing sign features...")
    a_signs = compute_signs(a_words)
    b_signs = compute_signs(b_words)
    print(f"  Linear A: {len(a_signs)} unique signs")
    print(f"  Linear B: {len(b_signs)} unique signs")

    # Test: can we recover Linear B's known grid?
    best_onset, best_vowel = test_grid_recovery(
        b_signs, LINB_ONSET, LINB_VOWEL, "Linear B (validation)"
    )

    # Shared signs analysis
    shared_signs_analysis(a_signs, b_signs)

    # Apply to Linear A
    best_onset_dim = best_onset[0][0]
    best_vowel_dim = best_vowel[0][0]
    cluster_linear_a(a_signs, best_onset_dim, best_vowel_dim)
