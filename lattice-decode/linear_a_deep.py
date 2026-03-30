#!/usr/bin/env python3
"""
Deep analysis of Linear A through the lattice.

Tasks:
1. Coordinate walks for common words
2. Administrative vs non-administrative context clustering
3. Phonetic value proposals from shared A/B signs
4. Religious vs administrative register analysis
5. Grammatical suffix clustering
"""

import re
import csv
import sys
from collections import Counter, defaultdict
from math import log, sqrt, pi as PI
import json

# ─── Extraction ───

def extract_linear_a_full():
    """Extract Linear A with full context per inscription."""
    with open('lineara/LinearAInscriptions.js') as f:
        text = f.read()

    inscriptions = {}
    # More robust extraction
    entries = re.findall(r'\["([^"]+)",\{(.*?)\}\]', text, re.DOTALL)

    for name, body in entries:
        # Get transliterated words
        tw_match = re.search(r'"transliteratedWords":\s*\[(.*?)\]', body, re.DOTALL)
        if not tw_match:
            continue
        raw = tw_match.group(1)
        tokens = re.findall(r'"([^"]*)"', raw)

        # Get site
        site_match = re.search(r'"site":\s*"([^"]*)"', body)
        site = site_match.group(1) if site_match else ""

        # Get support type
        support_match = re.search(r'"support":\s*"([^"]*)"', body)
        support = support_match.group(1) if support_match else ""

        # Separate words from numbers and markers
        words = []
        numbers = []
        has_numbers = False
        for i, t in enumerate(tokens):
            if t in ('\n', '', '𐄁'):
                continue
            if re.match(r'^\d+$', t):
                has_numbers = True
                numbers.append(int(t))
            elif t.startswith('OLE') or t.startswith('GRA') or t.startswith('VIN') or t.startswith('CYP') or t.startswith('OLIV') or t.startswith('AROM') or t.startswith('CAP') or t.startswith('*'):
                # Logograms / commodity signs
                words.append(t)
            elif re.match(r'^[A-Z]', t) and '-' in t:
                words.append(t)
            elif re.match(r'^[A-Z]', t) and len(t) <= 3:
                words.append(t)

        inscriptions[name] = {
            'site': site,
            'support': support,
            'words': words,
            'numbers': numbers,
            'has_numbers': has_numbers,
            'tokens': tokens,
        }

    return inscriptions


def compute_signs(words):
    """Compute distributional features for each sign."""
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

    max_freq = max(sign_freq.values()) if sign_freq else 1
    max_neigh = max(len(v) for v in sign_neighbors.values()) if sign_neighbors else 1

    signs = {}
    for sign in sign_freq:
        freq = sign_freq[sign]
        s = sign_start[sign]
        m = sign_middle[sign]
        e = sign_end[sign]
        tp = s + m + e or 1

        avg_len = sum(sign_word_lens[sign]) / len(sign_word_lens[sign])

        signs[sign] = {
            'freq': freq,
            'start': s, 'middle': m, 'end': e,
            'start_pct': s / tp,
            'end_pct': e / tp,
            'tau': (avg_len - 2.5) / 1.5,
            'chi': (s - e) / tp,
            'mu': (log(freq + 1) / log(max_freq + 1)) * 2 - 1,
            'phi': (len(sign_neighbors[sign]) / max_neigh) * 2 - 1 if max_neigh > 0 else 0,
            'avg_word_len': avg_len,
            'n_neighbors': len(sign_neighbors[sign]),
        }

    return signs


def extract_linear_b_signs():
    """Get Linear B signs for comparison."""
    words = []
    with open('linearb/tablet-sets/tablets.csv', newline='') as f:
        reader = csv.reader(f, delimiter=';')
        for row in reader:
            if len(row) >= 4:
                for w in row[3].split(','):
                    w = w.strip()
                    if w and not w.isdigit() and '-' in w:
                        words.append(w.upper())
    return compute_signs(words)


# ─── Known Linear B phonetic values ───

LINB_PHONETIC = {
    'DA':'da','DE':'de','DI':'di','DO':'do','DU':'du',
    'KA':'ka','KE':'ke','KI':'ki','KO':'ko','KU':'ku',
    'MA':'ma','ME':'me','MI':'mi','MO':'mo','MU':'mu',
    'NA':'na','NE':'ne','NI':'ni','NO':'no','NU':'nu',
    'PA':'pa','PE':'pe','PI':'pi','PO':'po','PU':'pu',
    'RA':'ra','RE':'re','RI':'ri','RO':'ro','RU':'ru',
    'SA':'sa','SE':'se','SI':'si','SO':'so','SU':'su',
    'TA':'ta','TE':'te','TI':'ti','TO':'to','TU':'tu',
    'WA':'wa','WE':'we','WI':'wi','WO':'wo',
    'JA':'ja','JE':'je','JO':'jo','JU':'ju',
    'QA':'qa','QE':'qe','QI':'qi','QO':'qo',
    'ZA':'za','ZE':'ze','ZO':'zo','ZU':'zu',
    'A':'a','E':'e','I':'i','O':'o','U':'u',
    'RA₂':'ra2','PA₃':'pa3','TA₂':'ta2',
}


# ─── Task 1: Coordinate Walks ───

def coordinate_walks(words, signs, label=""):
    """Map common words as coordinate paths through 4D space."""
    out = []
    out.append(f"\n## 1. Coordinate Walks — {label}\n")
    out.append("Each word = sequence of syllable signs. Each sign has (tau, chi, mu, phi).")
    out.append("A word is a PATH through 4D space.\n")

    word_freq = Counter(words)
    top_words = word_freq.most_common(40)

    out.append(f"### Top {len(top_words)} words as coordinate walks:\n")

    walk_stats = []

    for word, freq in top_words:
        parts = word.split('-')
        if len(parts) < 2:
            continue
        coords = []
        for p in parts:
            if p in signs:
                s = signs[p]
                coords.append((s['tau'], s['chi'], s['mu'], s['phi']))

        if len(coords) < 2:
            continue

        # Compute walk properties
        total_dist = 0
        deltas = []
        for i in range(len(coords) - 1):
            d = sqrt(sum((coords[i+1][j] - coords[i][j])**2 for j in range(4)))
            total_dist += d
            delta = tuple(coords[i+1][j] - coords[i][j] for j in range(4))
            deltas.append(delta)

        # Net displacement (start to end)
        net = tuple(coords[-1][j] - coords[0][j] for j in range(4))
        net_dist = sqrt(sum(x**2 for x in net))

        # Linearity: net displacement / total path length
        linearity = net_dist / total_dist if total_dist > 0 else 0

        # Average coordinate
        avg = tuple(sum(c[j] for c in coords) / len(coords) for j in range(4))

        coord_str = " -> ".join(
            f"{p}({c[0]:+.2f},{c[1]:+.2f},{c[2]:+.2f},{c[3]:+.2f})"
            for p, c in zip(parts, coords)
        )

        out.append(f"**{word}** (freq={freq})")
        out.append(f"  Walk: {coord_str}")
        out.append(f"  Path length: {total_dist:.3f}, Net displacement: {net_dist:.3f}, Linearity: {linearity:.3f}")
        out.append(f"  Centroid: ({avg[0]:+.2f},{avg[1]:+.2f},{avg[2]:+.2f},{avg[3]:+.2f})")
        out.append("")

        walk_stats.append({
            'word': word, 'freq': freq, 'n_signs': len(parts),
            'total_dist': total_dist, 'net_dist': net_dist,
            'linearity': linearity, 'centroid': avg,
        })

    # Pattern analysis
    out.append("### Walk Pattern Summary:\n")

    if walk_stats:
        avg_linearity = sum(w['linearity'] for w in walk_stats) / len(walk_stats)
        out.append(f"Average linearity: {avg_linearity:.3f}")
        out.append(f"  (1.0 = straight line, 0.0 = returns to start)")
        out.append(f"  {avg_linearity:.1%} linearity suggests {'directed/purposeful' if avg_linearity > 0.5 else 'looping/returning'} walks")
        out.append("")

        # Cluster by centroid location
        high_mu = [w for w in walk_stats if w['centroid'][2] > 0.5]
        low_mu = [w for w in walk_stats if w['centroid'][2] <= 0.5]
        out.append(f"High-mu centroid (common/open signs): {len(high_mu)} words")
        for w in high_mu[:5]:
            out.append(f"  {w['word']} (mu_avg={w['centroid'][2]:+.2f})")
        out.append(f"Low-mu centroid (rare/closed signs): {len(low_mu)} words")
        for w in low_mu[:5]:
            out.append(f"  {w['word']} (mu_avg={w['centroid'][2]:+.2f})")
        out.append("")

        # Linearity distribution
        linear_words = [w for w in walk_stats if w['linearity'] > 0.7]
        circular_words = [w for w in walk_stats if w['linearity'] < 0.3]
        out.append(f"Linear walks (>0.7): {len(linear_words)} — these tend to be 'directional' semantically")
        for w in linear_words[:5]:
            out.append(f"  {w['word']} (linearity={w['linearity']:.3f})")
        out.append(f"Circular walks (<0.3): {len(circular_words)} — these 'return' to origin")
        for w in circular_words[:5]:
            out.append(f"  {w['word']} (linearity={w['linearity']:.3f})")

    return '\n'.join(out), walk_stats


# ─── Task 2: Administrative context clustering ───

def admin_clustering(inscriptions, signs):
    """Cluster words by whether they appear near numbers."""
    out = []
    out.append("\n## 2. Administrative Context Clustering\n")
    out.append("HT tablets list items + numbers. Words near numbers = commodity names or quantities.")
    out.append("Words without numbers = personal names, verbs, or titles.\n")

    # Collect words that appear with numbers vs without
    words_with_numbers = Counter()
    words_without_numbers = Counter()
    kiro_count = 0
    kiro_contexts = []

    for name, insc in inscriptions.items():
        has_num = insc['has_numbers']
        for w in insc['words']:
            if '-' not in w and len(w) >= 3 and w.isupper():
                continue  # skip logograms
            if has_num:
                words_with_numbers[w] += 1
            else:
                words_without_numbers[w] += 1

            if w == 'KI-RO':
                kiro_count += 1
                kiro_contexts.append(name)

    # KI-RO analysis
    out.append(f"### KI-RO Analysis\n")
    out.append(f"KI-RO appears {kiro_count} times across {len(set(kiro_contexts))} inscriptions.")
    out.append(f"Inscriptions with KI-RO: {', '.join(sorted(set(kiro_contexts))[:20])}")

    # All KI-RO inscriptions are admin?
    kiro_admin = sum(1 for name in kiro_contexts if inscriptions[name]['has_numbers'])
    out.append(f"KI-RO with numbers: {kiro_admin}/{kiro_count} ({kiro_admin/max(kiro_count,1)*100:.0f}%)")

    if 'KI' in signs and 'RO' in signs:
        ki, ro = signs['KI'], signs['RO']
        out.append(f"\nKI-RO coordinate walk:")
        out.append(f"  KI: ({ki['tau']:+.2f},{ki['chi']:+.2f},{ki['mu']:+.2f},{ki['phi']:+.2f})")
        out.append(f"  RO: ({ro['tau']:+.2f},{ro['chi']:+.2f},{ro['mu']:+.2f},{ro['phi']:+.2f})")
        delta = tuple(ro[d] - ki[d] for d in ['tau','chi','mu','phi'])
        out.append(f"  Delta: ({delta[0]:+.2f},{delta[1]:+.2f},{delta[2]:+.2f},{delta[3]:+.2f})")
        out.append(f"  Walk: chi drops (frontward), phi drops (voicing down)")
        out.append(f"  Interpretation: KI→RO moves from high-position to end-position sign")
    out.append("")

    # Separate word classes
    admin_only = {w: c for w, c in words_with_numbers.items()
                  if c >= 2 and words_without_numbers.get(w, 0) == 0}
    non_admin_only = {w: c for w, c in words_without_numbers.items()
                      if c >= 2 and words_with_numbers.get(w, 0) == 0}
    both = {w: words_with_numbers[w] + words_without_numbers.get(w, 0)
            for w in words_with_numbers
            if w in words_without_numbers and words_with_numbers[w] >= 2}

    out.append(f"### Word Distribution by Context\n")
    out.append(f"Words appearing ONLY with numbers (likely commodities/measures): {len(admin_only)}")
    for w, c in sorted(admin_only.items(), key=lambda x: -x[1])[:15]:
        out.append(f"  {w:20s} (x{c})")
    out.append(f"\nWords appearing ONLY without numbers (likely names/verbs): {len(non_admin_only)}")
    for w, c in sorted(non_admin_only.items(), key=lambda x: -x[1])[:15]:
        out.append(f"  {w:20s} (x{c})")
    out.append(f"\nWords appearing in both contexts (function words/titles): {len(both)}")
    for w, c in sorted(both.items(), key=lambda x: -x[1])[:15]:
        out.append(f"  {w:20s} (x{c})")

    # Coordinate centroid comparison
    out.append(f"\n### Coordinate Centroids by Word Class\n")

    def word_centroid(word_set, signs):
        coords = {d: [] for d in ['tau','chi','mu','phi']}
        for word in word_set:
            parts = word.split('-')
            for p in parts:
                if p in signs:
                    for d in coords:
                        coords[d].append(signs[p][d])
        return {d: sum(v)/len(v) if v else 0 for d, v in coords.items()}

    if admin_only:
        c_admin = word_centroid(admin_only.keys(), signs)
        out.append(f"Admin-only centroid:     tau={c_admin['tau']:+.3f}  chi={c_admin['chi']:+.3f}  mu={c_admin['mu']:+.3f}  phi={c_admin['phi']:+.3f}")
    if non_admin_only:
        c_non = word_centroid(non_admin_only.keys(), signs)
        out.append(f"Non-admin centroid:      tau={c_non['tau']:+.3f}  chi={c_non['chi']:+.3f}  mu={c_non['mu']:+.3f}  phi={c_non['phi']:+.3f}")
    if both:
        c_both = word_centroid(both.keys(), signs)
        out.append(f"Both-context centroid:   tau={c_both['tau']:+.3f}  chi={c_both['chi']:+.3f}  mu={c_both['mu']:+.3f}  phi={c_both['phi']:+.3f}")

    if admin_only and non_admin_only:
        sep = sqrt(sum((c_admin[d] - c_non[d])**2 for d in ['tau','chi','mu','phi']))
        out.append(f"\nSeparation between admin and non-admin centroids: {sep:.3f}")
        out.append(f"(This is a 4D distance. >0.3 suggests meaningful clustering.)")

        # Which dimension separates most?
        for d in ['tau','chi','mu','phi']:
            diff = abs(c_admin[d] - c_non[d])
            if diff > 0.1:
                out.append(f"  {d}: admin={c_admin[d]:+.3f} vs non-admin={c_non[d]:+.3f} (diff={diff:.3f})")

    return '\n'.join(out)


# ─── Task 3: Phonetic value proposals ───

def phonetic_proposals(a_signs, b_signs):
    """Propose phonetic values for Linear A signs based on Linear B matches."""
    out = []
    out.append("\n## 3. Phonetic Value Proposals\n")
    out.append("Method: For shared sign names between A and B, compute distributional distance.")
    out.append("Signs with distance < 0.7 likely have the same phonetic value.")
    out.append("Signs with distance > 1.5 likely have DIFFERENT values.\n")

    shared = sorted(set(a_signs.keys()) & set(b_signs.keys()))
    shared = [s for s in shared if len(s) <= 3]

    proposals = []
    for sign in shared:
        a, b = a_signs[sign], b_signs[sign]
        d = sqrt(sum((a[c]-b[c])**2 for c in ['tau','chi','mu','phi']))
        proposals.append((sign, d, a, b))

    proposals.sort(key=lambda x: x[1])

    # Confident matches (dist < 0.7)
    confident = [(s, d, a, b) for s, d, a, b in proposals if d < 0.7]
    out.append(f"### HIGH-CONFIDENCE matches (dist < 0.7): {len(confident)} signs\n")
    out.append(f"These Linear A signs likely have the SAME phonetic value as their Linear B counterparts.\n")
    out.append(f"{'Sign':>6}  {'Dist':>6}  {'LinB value':>10}  {'Proposed LinA':>14}  {'Confidence':>10}")
    out.append(f"{'-'*60}")

    for sign, d, a, b in confident:
        linb_val = LINB_PHONETIC.get(sign, '?')
        conf = 'HIGH' if d < 0.5 else 'MEDIUM'
        proposals_for_sign = linb_val
        out.append(f"{sign:>6}  {d:>6.3f}  {linb_val:>10}  {proposals_for_sign:>14}  {conf:>10}")

    # Uncertain matches
    uncertain = [(s, d, a, b) for s, d, a, b in proposals if 0.7 <= d < 1.2]
    out.append(f"\n### UNCERTAIN matches (0.7 <= dist < 1.2): {len(uncertain)} signs\n")
    out.append(f"{'Sign':>6}  {'Dist':>6}  {'LinB value':>10}  {'Note':>30}")
    out.append(f"{'-'*60}")
    for sign, d, a, b in uncertain:
        linb_val = LINB_PHONETIC.get(sign, '?')
        out.append(f"{sign:>6}  {d:>6.3f}  {linb_val:>10}  similar phoneme but not identical")

    # Different values
    different = [(s, d, a, b) for s, d, a, b in proposals if d >= 1.2]
    out.append(f"\n### LIKELY DIFFERENT (dist >= 1.2): {len(different)} signs\n")
    out.append(f"These signs share the name but probably have DIFFERENT phonetic values in Linear A.\n")
    for sign, d, a, b in different:
        linb_val = LINB_PHONETIC.get(sign, '?')
        out.append(f"  {sign}: dist={d:.3f} (LinB = {linb_val}) — DIFFERENT in LinA")
        # What makes it different?
        diffs = []
        for dim in ['tau','chi','mu','phi']:
            dd = abs(a[dim] - b[dim])
            if dd > 0.5:
                diffs.append(f"{dim}: A={a[dim]:+.2f} vs B={b[dim]:+.2f}")
        if diffs:
            out.append(f"    Key differences: {', '.join(diffs)}")

    # Now attempt syllabic readings
    out.append(f"\n### Attempted Syllabic Readings\n")
    out.append(f"Using confident matches to read common Linear A words.\n")

    confident_map = {s: LINB_PHONETIC.get(s, '?') for s, d, a, b in confident}
    uncertain_map = {s: LINB_PHONETIC.get(s, '?') + '?' for s, d, a, b in uncertain}
    all_map = {**confident_map, **uncertain_map}

    # Get common A words
    all_words = []
    for insc in extract_linear_a_full().values():
        all_words.extend(insc['words'])
    word_freq = Counter(all_words)

    out.append(f"{'Word':>20}  {'Freq':>5}  {'Reading':>30}  {'Coverage':>8}")
    out.append(f"{'-'*70}")

    for word, freq in word_freq.most_common(50):
        if '-' not in word:
            continue
        parts = word.split('-')
        reading_parts = []
        confident_count = 0
        for p in parts:
            if p in confident_map:
                reading_parts.append(confident_map[p])
                confident_count += 1
            elif p in uncertain_map:
                reading_parts.append(uncertain_map[p])
            else:
                reading_parts.append(f'[{p}]')

        reading = '-'.join(reading_parts)
        coverage = confident_count / len(parts)
        if coverage > 0:  # only show if we can read something
            out.append(f"{word:>20}  {freq:>5}  {reading:>30}  {coverage:>7.0%}")

    return '\n'.join(out)


# ─── Task 4: Religious vs administrative register ───

def register_analysis(inscriptions, signs):
    """Compare coordinate walks in religious vs administrative texts."""
    out = []
    out.append("\n## 4. Religious vs Administrative Register\n")

    # Identify religious inscriptions (libation tables, etc)
    religious = {}
    administrative = {}
    other = {}

    for name, insc in inscriptions.items():
        support = insc['support'].lower()
        if 'libation' in support or 'altar' in support or 'offering' in support:
            religious[name] = insc
        elif 'tablet' in support:
            administrative[name] = insc
        else:
            other[name] = insc

    out.append(f"Religious inscriptions (libation tables, altars): {len(religious)}")
    out.append(f"Administrative inscriptions (tablets): {len(administrative)}")
    out.append(f"Other: {len(other)}")
    out.append("")

    # Collect words by register
    rel_words = []
    admin_words = []
    for insc in religious.values():
        rel_words.extend([w for w in insc['words'] if '-' in w])
    for insc in administrative.values():
        admin_words.extend([w for w in insc['words'] if '-' in w])

    out.append(f"Religious word tokens: {len(rel_words)}")
    out.append(f"Administrative word tokens: {len(admin_words)}")

    # Word frequency by register
    rel_freq = Counter(rel_words)
    admin_freq = Counter(admin_words)

    out.append(f"\n### Most common religious words:\n")
    for w, c in rel_freq.most_common(20):
        admin_c = admin_freq.get(w, 0)
        ratio = c / (c + admin_c) if (c + admin_c) > 0 else 0
        marker = " *** RELIGIOUS-SPECIFIC" if admin_c == 0 and c >= 2 else ""
        out.append(f"  {w:20s}  rel={c:3d}  admin={admin_c:3d}  rel_ratio={ratio:.2f}{marker}")

    out.append(f"\n### Most common admin-only words:\n")
    for w, c in admin_freq.most_common(20):
        rel_c = rel_freq.get(w, 0)
        if rel_c == 0:
            out.append(f"  {w:20s}  admin={c:3d}  rel=0  ADMIN-SPECIFIC")

    # A-SA-SA-RA analysis
    out.append(f"\n### A-SA-SA-RA Analysis\n")
    asasara_count = rel_freq.get('A-SA-SA-RA', 0) + admin_freq.get('A-SA-SA-RA', 0)
    out.append(f"A-SA-SA-RA total occurrences: {asasara_count}")
    out.append(f"  In religious context: {rel_freq.get('A-SA-SA-RA', 0)}")
    out.append(f"  In administrative context: {admin_freq.get('A-SA-SA-RA', 0)}")

    # Also check variants
    for variant in ['A-SA-SA-RA-ME', 'A-SA-SA-RA-NA']:
        rc = rel_freq.get(variant, 0) + admin_freq.get(variant, 0)
        if rc > 0:
            out.append(f"  Variant {variant}: rel={rel_freq.get(variant, 0)} admin={admin_freq.get(variant, 0)}")

    if all(s in signs for s in ['A','SA','RA']):
        out.append(f"\nA-SA-SA-RA coordinate walk:")
        path = ['A', 'SA', 'SA', 'RA']
        for i, p in enumerate(path):
            s = signs[p]
            out.append(f"  [{i}] {p}: ({s['tau']:+.2f},{s['chi']:+.2f},{s['mu']:+.2f},{s['phi']:+.2f})")
        # Walk properties
        coords = [(signs[p]['tau'], signs[p]['chi'], signs[p]['mu'], signs[p]['phi']) for p in path]
        total_dist = sum(sqrt(sum((coords[i+1][j]-coords[i][j])**2 for j in range(4))) for i in range(len(coords)-1))
        net = tuple(coords[-1][j] - coords[0][j] for j in range(4))
        net_dist = sqrt(sum(x**2 for x in net))
        out.append(f"  Total path: {total_dist:.3f}, Net displacement: {net_dist:.3f}")
        out.append(f"  Net direction: tau={net[0]:+.2f}, chi={net[1]:+.2f}, mu={net[2]:+.2f}, phi={net[3]:+.2f}")
        out.append(f"  Pattern: A(high-start) → SA-SA(repeated middle) → RA(end-position)")
        out.append(f"  The SA-SA repetition creates an oscillation pattern (∮ force)")

    # A-TA-I-*301-WA-JA analysis
    out.append(f"\n### A-TA-I-*301-WA-JA Analysis\n")
    # Search for this pattern and variants
    for word, freq in Counter(rel_words + admin_words).most_common(500):
        if 'TA-I' in word and 'WA' in word:
            out.append(f"  Found: {word} (freq={freq})")

    # Coordinate centroid comparison
    out.append(f"\n### Register Centroid Comparison\n")

    def compute_register_centroid(word_list):
        coords = {d: [] for d in ['tau','chi','mu','phi']}
        for word in word_list:
            parts = word.split('-')
            for p in parts:
                if p in signs:
                    for d in coords:
                        coords[d].append(signs[p][d])
        return {d: sum(v)/len(v) if v else 0 for d, v in coords.items()}

    if rel_words:
        rc = compute_register_centroid(rel_words)
        out.append(f"Religious centroid:      tau={rc['tau']:+.3f}  chi={rc['chi']:+.3f}  mu={rc['mu']:+.3f}  phi={rc['phi']:+.3f}")
    if admin_words:
        ac = compute_register_centroid(admin_words)
        out.append(f"Administrative centroid: tau={ac['tau']:+.3f}  chi={ac['chi']:+.3f}  mu={ac['mu']:+.3f}  phi={ac['phi']:+.3f}")

    if rel_words and admin_words:
        sep = sqrt(sum((rc[d] - ac[d])**2 for d in ['tau','chi','mu','phi']))
        out.append(f"Register separation: {sep:.3f}")
        if sep > 0.1:
            out.append("Different registers have measurably different coordinate profiles.")
            for d in ['tau','chi','mu','phi']:
                diff = rc[d] - ac[d]
                if abs(diff) > 0.05:
                    direction = "higher" if diff > 0 else "lower"
                    out.append(f"  Religious text is {direction} in {d} by {abs(diff):.3f}")

    # Walk linearity comparison
    out.append(f"\n### Walk Linearity by Register\n")

    def compute_linearities(word_list):
        linearities = []
        for word in word_list:
            parts = word.split('-')
            if len(parts) < 2:
                continue
            coords = []
            for p in parts:
                if p in signs:
                    s = signs[p]
                    coords.append((s['tau'], s['chi'], s['mu'], s['phi']))
            if len(coords) < 2:
                continue
            total_dist = sum(sqrt(sum((coords[i+1][j]-coords[i][j])**2 for j in range(4))) for i in range(len(coords)-1))
            net = tuple(coords[-1][j] - coords[0][j] for j in range(4))
            net_dist = sqrt(sum(x**2 for x in net))
            if total_dist > 0:
                linearities.append(net_dist / total_dist)
        return linearities

    rel_lin = compute_linearities(rel_words)
    admin_lin = compute_linearities(admin_words)

    if rel_lin:
        out.append(f"Religious walks: avg linearity = {sum(rel_lin)/len(rel_lin):.3f} (n={len(rel_lin)})")
    if admin_lin:
        out.append(f"Admin walks:     avg linearity = {sum(admin_lin)/len(admin_lin):.3f} (n={len(admin_lin)})")
    if rel_lin and admin_lin:
        diff = sum(rel_lin)/len(rel_lin) - sum(admin_lin)/len(admin_lin)
        if abs(diff) > 0.01:
            out.append(f"Difference: {abs(diff):.3f} ({'religious more linear' if diff > 0 else 'admin more linear'})")

    return '\n'.join(out)


# ─── Task 5: Grammatical suffix analysis ───

def suffix_analysis(words, signs):
    """Cluster words by final 1-2 signs to detect grammatical suffixes."""
    out = []
    out.append("\n## 5. Grammatical Suffix Analysis\n")
    out.append("If Linear A has suffixes, words ending in the same signs should cluster semantically.")
    out.append("We look for: (a) common endings, (b) whether ending predicts word properties.\n")

    # Collect final sign and final bigram
    final_sign = Counter()
    final_bigram = Counter()
    words_by_ending = defaultdict(list)
    words_by_bigram = defaultdict(list)

    word_freq = Counter(words)

    for word in set(words):
        parts = word.split('-')
        if len(parts) < 2:
            continue

        final = parts[-1]
        final_sign[final] += word_freq[word]
        words_by_ending[final].append((word, word_freq[word]))

        if len(parts) >= 2:
            bigram = f"{parts[-2]}-{parts[-1]}"
            final_bigram[bigram] += word_freq[word]
            words_by_bigram[bigram].append((word, word_freq[word]))

    out.append("### Most Common Final Signs\n")
    out.append(f"{'Sign':>8}  {'Freq':>6}  {'Unique words':>12}  {'Top words'}")
    out.append(f"{'-'*80}")

    for sign, freq in final_sign.most_common(20):
        word_list = words_by_ending[sign]
        n_unique = len(word_list)
        top = ', '.join(f"{w}" for w, c in sorted(word_list, key=lambda x: -x[1])[:5])
        out.append(f"{sign:>8}  {freq:>6}  {n_unique:>12}  {top}")

    out.append(f"\n### Most Common Final Bigrams\n")
    out.append(f"{'Bigram':>12}  {'Freq':>6}  {'Unique words':>12}  {'Top words'}")
    out.append(f"{'-'*80}")

    for bigram, freq in final_bigram.most_common(20):
        word_list = words_by_bigram[bigram]
        n_unique = len(word_list)
        top = ', '.join(f"{w}" for w, c in sorted(word_list, key=lambda x: -x[1])[:4])
        out.append(f"{bigram:>12}  {freq:>6}  {n_unique:>12}  {top}")

    # Statistical test: do words with same ending have similar INITIAL signs?
    out.append(f"\n### Suffix Consistency Test\n")
    out.append("If -RE is a grammatical suffix, words ending in RE should have diverse beginnings")
    out.append("but the suffix should NOT predict the first sign.\n")

    for sign, freq in final_sign.most_common(10):
        word_list = words_by_ending[sign]
        if len(word_list) < 3:
            continue

        initial_signs = Counter()
        word_lens = []
        for word, wf in word_list:
            parts = word.split('-')
            initial_signs[parts[0]] += wf
            word_lens.append(len(parts))

        n_initial = len(initial_signs)
        avg_len = sum(word_lens) / len(word_lens) if word_lens else 0

        # Entropy of initial signs (high = diverse = likely suffix)
        total = sum(initial_signs.values())
        entropy = -sum((c/total) * log(c/total + 1e-10) for c in initial_signs.values())
        max_entropy = log(n_initial + 1e-10)
        normalized_entropy = entropy / max_entropy if max_entropy > 0 else 0

        out.append(f"  -{sign}: {len(word_list)} unique words, {n_initial} different initial signs, "
                   f"entropy={normalized_entropy:.2f}, avg_len={avg_len:.1f}")
        if normalized_entropy > 0.7:
            out.append(f"    HIGH diversity — consistent with grammatical suffix")
        elif normalized_entropy < 0.4:
            out.append(f"    LOW diversity — more likely part of fixed stems")

    # Word families: same beginning, different endings
    out.append(f"\n### Word Families (Same Start, Different End)\n")
    out.append("Groups of words sharing the same first 1-2 signs but differing in endings.\n")

    families = defaultdict(list)
    for word in set(words):
        parts = word.split('-')
        if len(parts) >= 3:
            stem = '-'.join(parts[:2])
            families[stem].append((word, word_freq[word]))

    # Only show families with 3+ members
    big_families = {k: v for k, v in families.items() if len(v) >= 3}
    out.append(f"Families with 3+ members: {len(big_families)}\n")

    for stem, members in sorted(big_families.items(), key=lambda x: -sum(c for _, c in x[1]))[:15]:
        endings = []
        for word, freq in sorted(members, key=lambda x: -x[1]):
            parts = word.split('-')
            ending = '-'.join(parts[2:])
            endings.append(f"{ending}(x{freq})")
        out.append(f"  {stem}-... : {', '.join(endings)}")

    # Prefix analysis too
    out.append(f"\n### Most Common Initial Signs\n")
    initial_sign = Counter()
    for word in words:
        parts = word.split('-')
        if len(parts) >= 2:
            initial_sign[parts[0]] += 1

    out.append(f"{'Sign':>8}  {'Freq':>6}  {'% of words':>10}")
    out.append(f"{'-'*30}")
    total_words = sum(initial_sign.values())
    for sign, freq in initial_sign.most_common(15):
        out.append(f"{sign:>8}  {freq:>6}  {freq/total_words*100:>9.1f}%")

    return '\n'.join(out)


# ─── Main ───

if __name__ == '__main__':
    print("Loading data...", file=sys.stderr)
    inscriptions = extract_linear_a_full()

    all_words = []
    for insc in inscriptions.values():
        all_words.extend([w for w in insc['words'] if '-' in w or (len(w) <= 2 and w[0].isalpha())])

    print(f"  {len(inscriptions)} inscriptions, {len(all_words)} words", file=sys.stderr)

    signs = compute_signs(all_words)
    b_signs = extract_linear_b_signs()

    print(f"  {len(signs)} unique A signs, {len(b_signs)} unique B signs", file=sys.stderr)

    # Task 1
    walks_text, walk_stats = coordinate_walks(all_words, signs, "Linear A")
    print(walks_text)

    # Task 2
    print(admin_clustering(inscriptions, signs))

    # Task 3
    print(phonetic_proposals(signs, b_signs))

    # Task 4
    print(register_analysis(inscriptions, signs))

    # Task 5
    print(suffix_analysis(all_words, signs))
