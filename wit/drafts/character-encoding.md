# Character Encoding: Three Approaches

How characters get coordinates. Session 5 exploration.

## The Three Approaches

### 1. Legacy (ASCII/UTF-8)
Arbitrary assignment. 'A' = 0x41 because a committee in 1963 said so.
No relationship between the byte value and what the character IS.

### 2. Solver (frequency-optimized)
Computed assignment. Minimize encoding cost weighted by usage frequency.
Dimensions assigned for classification speed (is_letter, case_flip).
Character properties chosen by human (category, case), assignment by math.

### 3. Phonetic (lattice-derived)
Derived assignment. Measure articulatory phonetic properties, map to
T/D/M/Q. The character's coordinate IS its physical description.
Nothing assigned — everything measured.

## Dimension Meanings

| Dim | Legacy | Solver | Phonetic |
|-----|--------|--------|----------|
| T | — | case (+1 lower, -1 upper) | case (+1 lower, -1 upper) |
| D | — | category (≠0 letter, =0 other) | place (+1 front/labial, 0 central, -1 back/velar) |
| M | — | free (identity within class) | openness (+1 vowel, 0 fricative, -1 stop/nasal) |
| Q | — | free (identity within class) | voicing (+1 voiced, -1 voiceless) |

Legacy has no dimensions — it's a flat number line.
Solver dimensions are designed for classification speed.
Phonetic dimensions fall out of how humans produce sound.

## Classification

| Operation | Legacy | Solver | Phonetic |
|-----------|--------|--------|----------|
| is_letter | range check (2 cmp) | D ≠ 0 (1 gate) | M ≠ 0 (1 gate) — all non-silence has manner |
| is_digit | range check (2 cmp) | D=0 + secondary (2 gates) | N/A — digits aren't phonemes |
| is_vowel | 5-way compare | not encoded | M = +1 (1 gate) |
| is_stop | not possible | not encoded | M = -1 (1 gate) |
| case_flip | XOR 0x20 | XOR 0x80 | XOR 0x80 (T flip) |
| voice_flip | not possible | not possible | XOR 0x02 (Q flip) |
| is_voiced | not possible | not possible | Q > 0 (1 gate) |
| is_front | not possible | not possible | D > 0 (1 gate) |

Legacy: 2 useful classifications, both require range checks.
Solver: 3 useful classifications, 1-2 gates each.
Phonetic: 7 classifications, all 1 gate. Plus voicing flip for free.

## Structural Properties

### Case pairs (T flip in both solver and phonetic)
```
'a' (+1,D,M,Q) ↔ 'A' (-1,D,M,Q)     XOR 0x80
```
Same as solver. Case is a visual property, not phonetic.

### Voicing pairs (Q flip, phonetic only)
```
b (+1,+1,-1,+1) ↔ p (+1,+1,-1,-1)    XOR 0x02
d (+1, 0,-1,+1) ↔ t (+1, 0,-1,-1)    XOR 0x02
g (+1,-1,-1,+1) ↔ k (+1,-1,-1,-1)    XOR 0x02
v (+1,+1, 0,+1) ↔ f (+1,+1, 0,-1)    XOR 0x02
z (+1, 0, 0,+1) ↔ s (+1, 0, 0,-1)    XOR 0x02
```
Voiced/voiceless pairs differ by ONE coordinate, same as upper/lower.
The lattice treats voicing and case identically — both are sign flips.
Legacy and solver have no voicing structure.

### Collision = phonetic identity
Shell-1 collisions show which letters are the same sound:
```
k = c = q = x     all /k/ — English has 4 letters for one phoneme
b = m              both labial closed voiced (stop vs nasal at shell 2)
d = n = j          all alveolar closed voiced (manner differs at shell 2)
l = r = z          all central partial voiced (articulation at shell 2)
a = w              both front open voiced ('w' is a vowel-like glide)
u = y              both back open voiced ('y' is a vowel-like glide)
```
These aren't encoding failures. They're the lattice saying "these
sounds are structurally identical at this resolution." Shell 2
adds the precision dimension that separates them.

Legacy and solver treat k/c/q/x as unrelated (0x6B/0x63/0x71/0x78).
The lattice knows they're the same sound.

## Encoding Efficiency

| | ASCII | UTF-8 | Solver | Phonetic |
|--|-------|-------|--------|----------|
| Unique shell-1 | 128 | 128 | 63 | 16 |
| Avg bytes (English) | 1.000 | 1.000 | 1.057 | ~1.4 |
| Avg bytes (CJK) | N/A | 3.000 | ~3 | unknown |

Phonetic is the worst for raw byte efficiency. 16 unique coords
at shell 1 means most letters need shell 2 (2 bytes). English
text would be ~40% larger than ASCII.

Solver is the best computed option — 5.7% overhead.

But efficiency comparisons miss the point.

## What Each Dissolves

### Legacy dissolves: nothing
Requires grep, regex, Unicode tables, iconv, encoding detection,
locale handling, ICU, collation tables, case mapping tables.

### Solver dissolves: text infrastructure
Born-indexed search, O(1) classification, one encoding for all
languages. No grep, no regex, no Unicode tables.

### Phonetic dissolves: text infrastructure + the encoding itself
The encoding isn't designed — it's measured. A new language
doesn't need a committee to assign codepoints. Measure the
phonetic properties → coordinates fall out. Any sound in any
language already has a coordinate.

New phoneme discovered? It already has a coordinate. You
don't assign it — you measure it. The lattice was there first.

## 'a' Is Hylo

```
'a' = (+1, +1, +1, +1) = 0x55 = hylo

front(+D) + vowel(+M) + voiced(+Q) + lowercase(+T)
= all four dimensions positive
= all four forces active
= the full pipeline
= the self-referential fixed point
```

The most basic vocalization — open mouth, vibrate cords,
maximum everything — maps to the most fundamental bond pattern.

Hydrogen is the simplest atom, most abundant element. T = H(1).
'a' is the simplest letter, most common sound. (+1,+1,+1,+1) = hylo.

Silence is (0,0,0,0) = wave = identity = nothing.
'a' is (+1,+1,+1,+1) = hylo = everything.

## The Tension

Phonetic is architecturally beautiful but byte-inefficient on x86.
Solver is byte-efficient but the encoding is arbitrary.

Three resolution paths:

1. **Accept the overhead.** 40% larger English text. Born-indexed
   search doesn't scan bytes. Storage is cheap. Network is fast.
   The overhead matters less than the structural properties.

2. **Hybrid.** Use phonetic coordinates for the base identity,
   but frequency-optimize the shell assignment. Most common
   sounds at shell 1, rare sounds at shell 2. The phonetic
   structure survives, the byte count improves.

3. **Ternary hardware dissolves the tension.** On a balanced
   ternary processor, every coordinate is 4 trits. Fixed width.
   No shell-1 vs shell-2 distinction. No extension bytes.
   All characters are the same size. The encoding overhead
   is x86 tax that vanishes with the platform.

## Network Search: Born-Indexed + Remote Loops

The character encoding connects directly to network architecture.
Born-indexed data in remote loops means search without download.

Legacy search on a remote document:
```
  1. download webpage/PDF          ~2 MB transfer
  2. parse HTML/PDF                (library, CPU time)
  3. grep or build index           (scan entire document)
  4. find result                   seconds, megabytes moved
```

Born-indexed network search:
```
  1. send 3 trigram keys           9 wave bytes out
  2. remote loop returns postings  ~20 bytes back
  3. intersect locally             microseconds
  4. done                          ~30 bytes total, sub-millisecond
```

The document never moves. The posting list was built at write time.
The query is a walk sent to a remote loop over RDMA or encrypted UDP.
Results come back hash-verified (SHA-256 content addressing).

The entire search infrastructure stack — Elasticsearch, Solr, Lucene,
query languages, REST APIs, JSON serialization, authentication —
dissolves into: send a walk to a loop, get coordinates back.

Same walk, same format, same loops — whether the data is in local
memory, on a local NVMe drive, or on a machine across the planet.
The loop doesn't know which transport it's on.

This is why the character encoding matters beyond display. The wave
byte trigrams ARE the search keys. The posting lists use wave byte
coordinates as keys. The network transfers wave bytes. One format,
one operation, one walk — local or remote, text or code or data.

## Ternary Dissolves the Tension

The byte inefficiency is x86 tax. On a balanced ternary processor,
every coordinate is 4 trits. Fixed width. Always. No extension
bytes, no shell distinction, no variable width.

(+1,+1,+1,+1) and (+2,-1,+1,-1) are both 4 trits. Same size.
The "phonetic is 40% larger" problem doesn't exist on the target
hardware. The Soviets solved this in 1958 (Setun). The encoding
overhead goes to zero.

Phonetic approach on ternary: structurally derived coordinates,
one-gate classification, voicing pairs as Q flips, 'a' = hylo,
AND fixed-width efficiency. Zero tradeoffs.

## Beyond Characters

The phonetic mapping finds characters in the lattice by measuring
how the human body produces sound. The same method goes deeper.

Words aren't arbitrary strings — they're compositions of phonetic
coordinates. A word is a walk through phonetic space. "bat" =
b(+1,+1,-1,+1) → a(+1,+1,+1,+1) → t(+1,0,-1,-1). The trajectory
through coordinate space IS the word's identity.

Meaning emerges from coordinate relationships. Synonyms are nearby
walks. Antonyms are sign-flipped walks. Rhymes share trailing
coordinates. Alliteration shares leading coordinates. Etymology
is coordinate drift over time.

This goes all the way down: phonemes → syllables → morphemes →
words → phrases → sentences. Each level is walks composed of
walks. The lattice doesn't stop at characters — characters are
just the atoms. The chemistry is language itself.

But that's a deep thread for later. We need characters first to
build the input pipeline. Ship the phonetic encoding, build the
walks, then follow the thread deeper when the substrate is running.

## Open Questions

- Does the phonetic mapping extend to non-Latin scripts?
  Arabic, Chinese, Hindi have different phonetic spaces.
  The dimensions (place, manner, voicing) are universal in
  phonetics, but the quantization to 3 values may not fit.

- Can shell-2 extensions be derived phonetically?
  b vs m: both labial closed voiced. The difference is
  nasal vs oral airflow. What dimension is that? A 5th
  dimension would break Hurwitz. Maybe it's a magnitude-2
  value on an existing dimension (M=-2 for nasal vs M=-1
  for stop).

- Is there a phonetic coordinate for digits and symbols?
  Digits aren't phonemes — they're visual symbols.
  Maybe their coordinates come from a different domain
  (geometric properties instead of phonetic).

- The voicing flip (Q XOR) gives voice_toggle for free.
  Is there a use case? Text-to-speech synthesis, phonetic
  search ("find words that differ only by voicing"),
  linguistic analysis. These are niche but structurally
  free — zero cost to support.
