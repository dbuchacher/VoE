#!/usr/bin/env python3
"""
Lattice analysis of Linear A and Linear B sign systems.

Approach: map signs to (τ,χ,μ,φ) coordinates using distributional
properties, then compare. Linear B is our control (known phonetic values).

Coordinate mapping from distributional statistics:
  τ (duration/time):  word length tendency (sustained vs transient)
  χ (place/space):    positional preference (front vs back of word)
  μ (manner/mass):    frequency (common = open/vowel-like, rare = stop-like)
  φ (voicing/charge): co-occurrence asymmetry (voiced = combinatorial)
"""

import re
import csv
import sys
from collections import Counter, defaultdict
from math import log, sqrt

# ─── Linear A extraction ───

def extract_linear_a():
    with open('lineara/LinearAInscriptions.js') as f:
        text = f.read()

    # Extract transliterated words per inscription
    inscriptions = {}
    blocks = re.findall(
        r'\["([^"]+)",\{.*?"transliteratedWords":\s*\[(.*?)\]',
        text, re.DOTALL
    )
    all_words = []
    for name, block in blocks:
        words = re.findall(r'"([A-Z][A-Z0-9₂₃-]*)"', block)
        # filter out numbers and single-char noise
        words = [w for w in words if not w.isdigit() and len(w) > 0]
        inscriptions[name] = words
        all_words.extend(words)

    return inscriptions, all_words


def extract_linear_b():
    words = []
    with open('linearb/tablet-sets/tablets.csv', newline='') as f:
        reader = csv.reader(f, delimiter=';')
        for row in reader:
            if len(row) >= 4:
                # inscription column has comma-separated words
                for w in row[3].split(','):
                    w = w.strip()
                    if w and not w.isdigit() and '-' in w:
                        words.append(w.upper())

    # Also get character stats
    char_stats = {}
    with open('linearb/character-sets/everything.csv', newline='') as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row['character'] == 'test':
                continue
            char_stats[row['character'].upper()] = {
                'total': int(row['total']),
                'start': int(row['start']),
                'middle': int(row['middle']),
                'end': int(row['end']),
            }

    return words, char_stats


# ─── Sign analysis ───

def analyze_signs(words, label=""):
    """Compute distributional properties for each syllable sign."""
    sign_freq = Counter()
    sign_start = Counter()   # appears at word start
    sign_end = Counter()     # appears at word end
    sign_middle = Counter()  # appears in word middle
    sign_pairs = Counter()   # bigram co-occurrences
    sign_word_lens = defaultdict(list)  # word lengths sign appears in
    sign_neighbors = defaultdict(Counter)  # what appears next to each sign

    for word in words:
        parts = word.split('-')
        n = len(parts)
        for i, sign in enumerate(parts):
            sign_freq[sign] += 1
            sign_word_lens[sign].append(n)

            if i == 0:
                sign_start[sign] += 1
            elif i == n - 1:
                sign_end[sign] += 1
            else:
                sign_middle[sign] += 1

            if i < n - 1:
                sign_pairs[(sign, parts[i+1])] += 1
                sign_neighbors[sign][parts[i+1]] += 1
            if i > 0:
                sign_neighbors[sign][parts[i-1]] += 1

    # Compute coordinates for each sign
    signs = {}
    total = sum(sign_freq.values())
    max_freq = max(sign_freq.values()) if sign_freq else 1

    for sign in sign_freq:
        freq = sign_freq[sign]
        s = sign_start[sign]
        m = sign_middle[sign]
        e = sign_end[sign]
        total_pos = s + m + e
        if total_pos == 0:
            total_pos = 1

        # τ (duration): avg word length the sign appears in
        # longer words = more sustained context
        avg_len = sum(sign_word_lens[sign]) / len(sign_word_lens[sign])
        tau = (avg_len - 2.5) / 1.5  # normalize around typical word length

        # χ (place): positional preference
        # +1 = tends toward start (front), -1 = tends toward end (back)
        chi = (s - e) / total_pos if total_pos > 0 else 0

        # μ (manner): frequency maps to openness
        # high freq = vowel-like (open), low freq = stop-like (closed)
        mu = (log(freq + 1) / log(max_freq + 1)) * 2 - 1

        # φ (voicing): combinatorial diversity
        # many different neighbors = voiced (combinatorial), few = voiceless
        n_neighbors = len(sign_neighbors[sign])
        max_neighbors = max(len(v) for v in sign_neighbors.values()) if sign_neighbors else 1
        phi = (n_neighbors / max_neighbors) * 2 - 1

        signs[sign] = {
            'freq': freq,
            'start': s, 'middle': m, 'end': e,
            'tau': round(tau, 3),
            'chi': round(chi, 3),
            'mu': round(mu, 3),
            'phi': round(phi, 3),
            'avg_word_len': round(avg_len, 2),
            'n_neighbors': n_neighbors,
        }

    return signs, sign_pairs


def distance(a, b):
    """4D Euclidean distance between two sign coordinates."""
    return sqrt(
        (a['tau'] - b['tau'])**2 +
        (a['chi'] - b['chi'])**2 +
        (a['mu'] - b['mu'])**2 +
        (a['phi'] - b['phi'])**2
    )


def print_signs(signs, label, top_n=40):
    """Print signs sorted by frequency with their coordinates."""
    print(f"\n{'='*75}")
    print(f"  {label}")
    print(f"{'='*75}")
    print(f"{'sign':>8}  {'freq':>5}  {'s/m/e':>10}  "
          f"{'τ':>6}  {'χ':>6}  {'μ':>6}  {'φ':>6}  {'neighbors':>4}")
    print(f"{'-'*75}")

    sorted_signs = sorted(signs.items(), key=lambda x: -x[1]['freq'])
    for sign, s in sorted_signs[:top_n]:
        print(f"{sign:>8}  {s['freq']:>5}  "
              f"{s['start']:>3}/{s['middle']:>3}/{s['end']:>3}  "
              f"{s['tau']:>6.3f}  {s['chi']:>6.3f}  {s['mu']:>6.3f}  {s['phi']:>6.3f}  "
              f"{s['n_neighbors']:>4}")


def find_cross_script_matches(a_signs, b_signs, top_n=20):
    """Find Linear A signs closest to Linear B signs in coordinate space."""
    print(f"\n{'='*75}")
    print(f"  Cross-Script Coordinate Matches (Linear A → Linear B)")
    print(f"{'='*75}")
    print(f"{'A sign':>8}  {'B sign':>8}  {'dist':>6}  "
          f"{'A coords (τ,χ,μ,φ)':>24}  {'B coords':>24}")
    print(f"{'-'*75}")

    matches = []
    for a_name, a in a_signs.items():
        best_dist = float('inf')
        best_b = None
        for b_name, b in b_signs.items():
            d = distance(a, b)
            if d < best_dist:
                best_dist = d
                best_b = b_name
        matches.append((a_name, best_b, best_dist, a, b_signs[best_b]))

    matches.sort(key=lambda x: x[2])
    for a_name, b_name, d, a, b in matches[:top_n]:
        ac = f"({a['tau']:+.2f},{a['chi']:+.2f},{a['mu']:+.2f},{a['phi']:+.2f})"
        bc = f"({b['tau']:+.2f},{b['chi']:+.2f},{b['mu']:+.2f},{b['phi']:+.2f})"
        print(f"{a_name:>8}  {b_name:>8}  {d:>6.3f}  {ac:>24}  {bc:>24}")


def analyze_bond_patterns(words, signs, label=""):
    """Detect bond patterns (force combinations) in sign sequences."""
    print(f"\n{'='*75}")
    print(f"  Bond Patterns in {label}")
    print(f"{'='*75}")

    # For each consecutive pair, classify the relationship
    # by coordinate deltas
    pattern_counts = Counter()
    pattern_examples = defaultdict(list)

    for word in words:
        parts = word.split('-')
        for i in range(len(parts) - 1):
            a, b = parts[i], parts[i+1]
            if a not in signs or b not in signs:
                continue
            sa, sb = signs[a], signs[b]

            # Classify by which dimensions change significantly
            forces = []
            dt = sb['tau'] - sa['tau']
            dc = sb['chi'] - sa['chi']
            dm = sb['mu'] - sa['mu']
            dp = sb['phi'] - sa['phi']

            threshold = 0.3
            if abs(dt) > threshold: forces.append('τ')
            if abs(dc) > threshold: forces.append('χ')
            if abs(dm) > threshold: forces.append('μ')
            if abs(dp) > threshold: forces.append('φ')

            pattern = '+'.join(forces) if forces else 'identity'
            pattern_counts[pattern] += 1
            if len(pattern_examples[pattern]) < 3:
                pattern_examples[pattern].append(f"{a}→{b}")

    print(f"{'pattern':>20}  {'count':>6}  {'%':>6}  examples")
    print(f"{'-'*75}")
    total = sum(pattern_counts.values())
    for pattern, count in pattern_counts.most_common(20):
        pct = count / total * 100 if total else 0
        examples = ', '.join(pattern_examples[pattern])
        print(f"{pattern:>20}  {count:>6}  {pct:>5.1f}%  {examples}")


def vowel_consonant_detection(signs):
    """Identify likely vowels vs consonants from coordinates.
    Vowels: high μ (frequency), balanced χ (no strong position preference).
    """
    print(f"\n  Vowel/Consonant classification (by coordinate clustering):")
    print(f"  {'sign':>8} {'μ':>6} {'χ':>6} {'class':>10}")
    print(f"  {'-'*40}")

    classified = []
    for sign, s in sorted(signs.items(), key=lambda x: -x[1]['mu']):
        # High μ + near-zero χ = vowel-like
        # Low μ or extreme χ = consonant-like
        if s['mu'] > 0.3 and abs(s['chi']) < 0.4:
            cls = 'VOWEL'
        elif s['mu'] > 0.1:
            cls = 'semi'
        else:
            cls = 'consonant'
        classified.append((sign, s, cls))

    for sign, s, cls in classified[:25]:
        marker = '◆' if cls == 'VOWEL' else '◇' if cls == 'semi' else ' '
        print(f"  {sign:>8} {s['mu']:>6.3f} {s['chi']:>6.3f} {cls:>10} {marker}")

    return classified


# ─── Main ───

if __name__ == '__main__':
    print("Extracting Linear A corpus...")
    a_inscriptions, a_words = extract_linear_a()
    print(f"  {len(a_inscriptions)} inscriptions, {len(a_words)} words")

    print("Extracting Linear B corpus...")
    b_words, b_char_stats = extract_linear_b()
    print(f"  {len(b_words)} words, {len(b_char_stats)} character entries")

    print("\nAnalyzing Linear A signs...")
    a_signs, a_pairs = analyze_signs(a_words, "Linear A")
    print_signs(a_signs, "LINEAR A — Sign Coordinates", top_n=50)

    print("\nAnalyzing Linear B signs...")
    b_signs, b_pairs = analyze_signs(b_words, "Linear B")
    print_signs(b_signs, "LINEAR B — Sign Coordinates", top_n=50)

    # Cross-script matching
    find_cross_script_matches(a_signs, b_signs)

    # Bond pattern analysis
    analyze_bond_patterns(a_words, a_signs, "Linear A")
    analyze_bond_patterns(b_words, b_signs, "Linear B")

    # Vowel/consonant detection
    print(f"\n{'='*75}")
    print(f"  Linear A — Vowel/Consonant Detection")
    print(f"{'='*75}")
    a_vc = vowel_consonant_detection(a_signs)

    print(f"\n{'='*75}")
    print(f"  Linear B — Vowel/Consonant Detection (validation)")
    print(f"{'='*75}")
    b_vc = vowel_consonant_detection(b_signs)

    # Validate against known Linear B phonetics
    print(f"\n{'='*75}")
    print(f"  Validation: Linear B known vowels vs detected")
    print(f"{'='*75}")
    known_vowels = {'A', 'E', 'I', 'O', 'U'}
    detected_vowels = {sign for sign, _, cls in b_vc if cls == 'VOWEL'}
    print(f"  Known pure vowels: {known_vowels}")
    print(f"  Detected as VOWEL: {detected_vowels}")
    print(f"  Intersection: {known_vowels & detected_vowels}")
    print(f"  Missed: {known_vowels - detected_vowels}")
    print(f"  False positive: {detected_vowels - known_vowels}")
