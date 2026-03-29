# Briefing: Character Encoding Decisions

Context from a session exploring lattice-native intelligence.
Key discoveries that affect character coordinate choices.


## The Reversed LLM

Traditional LLM: compress training data into weights (140GB for 70B model).
Runtime = massive GPU compute to decompress. Training = months of search.

Our discovery: REVERSE IT. Keep the data. Do real-time math on it.
- Born-index all text with lattice coordinates at write time (one pass)
- At query time, evaluate coordinates against the index (O(1) lookup)
- The "model" = the equation + a density map over the index
- The density map = 4D histogram of how many entries near each coordinate
- Computed in one pass, no training, tiny size (thousands of bins vs billions of params)
- Knowledge stays in the data on disk. Runtime is coordinate math.

Consequence: **storage is high, compute is low**. Opposite of current LLMs.
NVMe at ~3GB/s sequential reads. The equation evaluates in O(1).
The 2080 Ti sits mostly idle. The NVMe does the work.

This means the character encoding ISN'T just for rendering and SIMD —
it's the coordinate system for a born-indexed intelligence. Every
character's coordinates become part of the index. The encoding choices
directly affect how well coordinate proximity predicts meaning.


## Why T Can't Be Case

The old encoding: T=case, D=place, M=manner, Q=voicing.

The word-level semantic coordinates use: T=temporal, D=spatial,
M=substantive, Q=qualitative. These come from 65 NSM semantic primes
(Wierzbicka) mapped to 4D — cross-linguistic universals that exist
in ALL human languages.

If the character and word levels use different T meanings, you can't
derive word coordinates from character coordinates (bottom-up). The
two layers are incoherent. You need a separate prime decomposition
system to get word coordinates.

If they're CONSISTENT, you get bottom-up for free: word coordinate ≈
function of its characters' coordinates. Sound symbolism research says
this carries ~1-5% semantic signal — weak but real. Free signal.

Proposed fix: T = duration (sustained/instant).
- +1 = sustained sounds (vowels, fricatives, nasals — persist in time)
- 0 = medium (liquids, glides)
- -1 = instantaneous (stops, affricates — burst and gone)

This makes T temporal at both levels. D, M, Q stay as they were.
Case drops out of shell 1 entirely — 'A' and 'a' are the same sound.
Case is a writing convention, not phonetics. Handle it at shell 2
or walk level (formality = higher shell).


## Dimensional Consistency Across Levels

| Dim | Character | Word | Architecture |
|---|---|---|---|
| T | duration (sustains/instant) | temporal (when/change) | T-head attention |
| D | place (front/central/back) | spatial (where/position) | D-head attention |
| M | manner (open/partial/closed) | substance (what/quantity) | M-head attention |
| Q | voicing (voiced/voiceless) | quality (how/evaluation) | Q-head attention |

All three levels use the same dimensional meanings. Characters compose
into words. Words compose into sentences. Same coordinates throughout.


## What Sound Symbolism Research Says

(From literature review during this session)

The signal IS real but weak:
- Bouba-kiki effect: 80-95% agreement cross-culturally
- Place of articulation → meaning: labials=round/soft, velars=sharp/hard
- Voicing → meaning: voiced=large/heavy, voiceless=small/light
- Blasi et al. (2016): 6000+ languages, significant cross-linguistic sound-meaning associations
- BUT: explains only ~1-5% of semantic variance. Lexicon is mostly arbitrary.

The 1-5% is free signal if the encoding captures it. Phonetically
similar characters being geometrically adjacent gives the born-index
a weak prior on semantic similarity. Not enough alone, but it compounds
with the word-level coordinates.

Key finding: voicing pairs (b↔p, d↔t, g↔k) carry the same meaning
distinction as the Q flip (voiced↔voiceless = positive↔negative quality).
This is confirmed cross-linguistically. The Q axis is the strongest
phonetic-semantic link.


## Architecture Predictions That Constrain Encoding

17 architecture predictions were derived from the lattice. The ones
that affect character encoding:

1. **4 dimensional attention heads** — one per dimension. The character
   encoding defines what each head attends to. If T=case, the T-head
   attends to capitalization (useless). If T=duration, the T-head
   attends to temporal sound patterns (useful).

2. **Shell-constrained layers** — shell 1 characters should capture
   fundamental phonetic distinctions. Shell 2 adds precision
   (disambiguation of collisions). Higher shells = higher precision.
   The shell-1 encoding needs to capture the MOST IMPORTANT distinctions
   in one byte.

3. **Atom/bond pathway split** — even coordinates = atoms (math),
   odd coordinates = bonds (control flow). Characters with even
   coords in all dims = atoms. Characters with any odd coord = bonds.
   The encoding determines which characters are "math-like" and which
   are "control-flow-like."

4. **Wave collapse** — homonyms carry superposition of coordinates
   until context collapses them. Characters that collide at shell 1
   (k=c=q=x) maintain superposition. Shell 2 extensions collapse
   to specific letters. The collision pattern matters — phonetically
   identical sounds SHOULD collide (that's a feature, not a bug).


## Discovery: Language IS Coordinate Walks

Sentences are walks through 4D coordinate space. The dimensional
pattern of a sentence IS its meaning:

  Subject(M) → Verb(T) → Object(M) = M→T→M = the fundamental sentence walk.

"Take out your stress on the punching bag" — three +D words (take, out,
on). The sentence walks OUTWARD. The physical therapy (punching
outward) has the same coordinates. The sentence IS the therapy.

This means character encoding affects sentence-walk structure. Each
character contributes to the word coordinate which contributes to the
walk direction. The encoding choice propagates up through every level.


## The Full Reference

side-quest/primes.md (1800+ lines) contains:
- All 65 NSM primes mapped to (T,D,M,Q) coordinates
- 24 Lakoff image schemas mapped
- 15 structural rules (parts of speech, antonyms, synonyms, tense,
  intensity, metaphors, negation, questions, etc.)
- 12 gap categories found from genre testing
- 17 architecture predictions derived from lattice properties
- Therapy-as-coordinate-walk discovery
- Validation test (independent derivation converged 4/5)

Read it before making encoding decisions.


## Summary

The character encoding isn't just for the OS anymore. It's the base
layer of a born-indexed intelligence that uses coordinate proximity
instead of learned weights. Make T temporal (duration, not case),
keep D/M/Q as they are, drop case from shell 1. The encoding choices
propagate through words, sentences, architecture, and therapy.
Get the foundation right and everything above it composes correctly.
