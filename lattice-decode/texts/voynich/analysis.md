# Voynich Manuscript — Lattice Analysis

**Date:** 2026-03-30
**Source:** Takahashi EVA transcription (IT2a-n.txt, 5444 lines, 37026 words)
**Method:** Distributional (τ,χ,μ,φ) coordinate assignment + structural tests
**Confidence:** Mixed. Structure-level findings are strong (60-90%). Identification claims are speculative (30-40%).


## The Question

The Voynich Manuscript is ~240 pages in an unknown writing system, dated to ~1400 CE. A century of study has produced no consensus. Three hypotheses: (a) natural language in an unknown script, (b) cipher enciphering a known language, (c) elaborate hoax / constructed glossolalia.

The lattice gives us a structural test. Natural languages, ciphers, and constructed systems produce different distributional signatures across (τ,χ,μ,φ) — positional preference, frequency, neighbor diversity, word-length tendency. We cannot read the Voynich, but we can measure what kind of thing it is.


## Raw Numbers

| Metric | Voynich | English | Latin | Random |
|--------|---------|---------|-------|--------|
| Unigram entropy | 4.046 | 4.07 | 4.00 | 4.70 |
| Bigram entropy | 6.260 | 7.12 | 6.80 | 9.40 |
| Conditional entropy | 2.395 | 3.56 | 3.30 | 4.70 |
| Word entropy | 10.551 | 9.80 | 10.20 | 14.00 |
| Type-token ratio | 0.230 | 0.05 | 0.08 | 0.70 |
| Hapax legomena | 0.713 | 0.45 | 0.50 | 0.90 |
| Zipf CV | 0.234 | 0.35 | 0.32 | 0.80 |
| Double-letter rate | 0.005 | 0.10 | 0.06 | — |
| Mean word length | 4.41 | — | — | — |
| Word length std | 1.76 | ~2.5 | ~2.8 | — |

37,026 words. 163,288 glyphs. 32 distinct glyph types (after EVA digraph/trigraph tokenization). 8,525 unique words.


## Finding 1: Unigram Entropy Is Natural — Bigram Entropy Is Not

Unigram entropy (4.046 bits) falls squarely between English and Latin. By this measure alone, the Voynich looks like a natural language. It has the right number of symbols at the right frequencies.

But bigram entropy (6.260 bits) is substantially lower than any known European language (English 7.12, Latin 6.80). This means adjacent glyph pairs are more predictable than in natural language. Conditional entropy (2.395 bits) confirms it — given the current glyph, the next glyph carries only 2.4 bits of surprise, compared to 3.3-3.6 for real European languages.

**Lattice interpretation:** The system operates in fewer effective dimensions than a natural alphabetic language. The walks through glyph-space are constrained to a narrower corridor. This is the single most important structural finding.

**Confidence: 85%** — entropy measurements are robust.


## Finding 2: Extreme Position Locking

Voynich glyphs are position-locked far beyond any known natural alphabet:

| Glyph | Position Lock | Detail |
|-------|--------------|--------|
| q | 98.8% initial | Almost never appears except at word start |
| y | 84.5% final | Overwhelmingly word-final |
| n | 97.0% final | Almost exclusively word-final |
| m | 92.3% final | Almost exclusively word-final |
| e | 98.6% medial | Never starts or ends a word |
| k | 87.8% medial | Nearly always interior |
| a | 85.8% medial | Nearly always interior |
| ii | 99.9% medial | Never starts or ends a word |
| ee | 98.1% medial | Never starts or ends a word |

Natural alphabets do have positional tendencies — English 'q' prefers initial position, for example. But nothing like this. These aren't tendencies. They're rules. The system has hard constraints on where glyphs can appear within a word.

**Lattice interpretation:** The τ-dimension (positional preference) carries almost as much information as identity. This is the signature of a syllabary, an abugida, or a constructional system — NOT a simple alphabetic cipher. In an alphabetic cipher of Latin or Italian, the positional freedom of the plaintext alphabet would survive the substitution.

**Confidence: 90%** — the position-locking data is unambiguous.


## Finding 3: The φ-Dimension Collapses

When we assign lattice coordinates, the φ-dimension (word-length tendency, which in the phonetic framework maps to voicing) shows almost no variance across glyphs:

- φ range: [-1.000, -0.422]
- φ std: 0.132
- 28 of 30 glyphs cluster within φ = -0.92 to -1.00

In a natural alphabetic script encoding a European language, voiced/voiceless pairs (b/p, d/t, g/k, v/f, z/s) create clear bimodal structure in this dimension. The Voynich shows no such bimodality.

The effective dimensionality of the Voynich writing system is **3, not 4**. It operates in (τ,χ,μ) with φ collapsed.

**Lattice interpretation:** This is exactly what you'd expect from:
- A **syllabary** (voicing is embedded within the syllable sign, not an independent dimension)
- A **semantic** rather than phonetic encoding (where the symbol = meaning, not sound)
- A **constructed system** that deliberately omits a phonetic distinction

It is NOT what you'd expect from a simple substitution cipher on any European alphabet.

**Confidence: 85%**


## Finding 4: 48% Forbidden Bigrams

Of the 225 possible bigram pairs among the top 15 glyphs, 108 (48%) are effectively forbidden (observed count < 5% of expected). This is far higher than natural languages (15-25%) and far higher than ciphers (5-15%).

Meanwhile, 12 pairs are "required" (observed > 5x expected):

| Pair | Ratio |
|------|-------|
| a + ii | 13.8x |
| ii + n | 31.2x |
| k + ee | 9.5x |
| sh + e | 9.3x |
| q + o | 8.1x |
| ch + e | 7.7x |
| a + r | 6.4x |
| d + y | 6.3x |

The pair `ii + n` is 31x more common than expected. The pair `a + ii` is 14x. These aren't co-occurrence tendencies. These are structural bonds — glyph clusters that function as units.

**Lattice interpretation:** The writing system contains compound glyphs that behave as single bonds. `aiin` (a + ii + n), `chedy` (ch + e + d + y), `qokedy` (q + o + k + ee + d + y) are not sequences of independent phonemes — they are words with internal structure governed by strict positional rules. The bond patterns within words are heavily constrained. Of the 14 observed bond-type patterns (dimensions that change between adjacent glyphs), one pattern — τχμ change with φ constant — accounts for 58.6% of all transitions.

This is extremely regular. Natural language bond patterns distribute more evenly across the 16 possible types.

**Confidence: 80%**


## Finding 5: Zipf Holds, But Type-Token Is Wrong

The Voynich obeys Zipf's law beautifully (CV = 0.234, better than English at 0.35). The rank-frequency distribution is genuinely Zipfian. This is the strongest argument that the manuscript contains meaningful content — Zipf's law emerges from the compression dynamics of communication.

But the type-token ratio (0.230) is 3-5x higher than natural language (0.04-0.08). The vocabulary is too large relative to the text length — or equivalently, each word form repeats too few times.

Combined with the high hapax legomena ratio (0.713 vs English 0.45), the vocabulary behaves like a system with many near-synonyms or morphological variants.

**Lattice interpretation:** The combination of strong Zipf + high TTR + high hapax is consistent with either:
- A **highly inflected language** (like Basque, Georgian, or Turkish, where each root generates many surface forms)
- A **generative construction** where simple rules produce many word forms, most appearing only once or twice

The lattice leans toward the latter. The walk-pattern analysis shows only 4,611 unique walks across 36,351 words — meaning the same small set of coordinate trajectories generates the vocabulary through variation. This is more consistent with a **productive morphological system** (or a table-based generator) than with an analytic language like English.

**Confidence: 70%**


## Finding 6: Section Variation Exists But Is Modest

| Section | Words | H(unigram) | H(bigram) | H(conditional) | Avg len |
|---------|-------|-----------|----------|----------------|---------|
| herbal_A | 9,595 | 4.058 | 6.350 | 2.471 | 4.23 |
| herbal_B | 6,866 | 3.950 | 5.670 | 1.876 | 4.38 |
| astronomical | 1,694 | 3.992 | 6.293 | 2.488 | 4.47 |
| balneological | 2,571 | 3.880 | 6.287 | 2.591 | 4.50 |
| pharmaceutical | 1,810 | 4.030 | 6.031 | 2.200 | 4.43 |
| stars_recipes | 14,490 | 4.039 | 6.135 | 2.304 | 4.52 |

Entropy varies across sections (range 0.178 bits unigram, 0.715 conditional). Herbal_B is notably more predictable than herbal_A (conditional entropy 1.876 vs 2.471). This means either the topic constrains the vocabulary more tightly, or two different scribes used slightly different habits.

For a meaningless hoax, all sections would show similar statistics (same generation process). The variation here is a point in favor of real content.

**Confidence: 65%** — the variation is real, but its interpretation is debatable.


## Finding 7: No Cipher Period, Normal Line Filling

Word repeat distances show no periodic structure. The gap distribution is smooth and near-uniform (gap entropy 10.52 bits out of 12.51 max). This rules out simple polyalphabetic ciphers (which produce characteristic periodicity).

Lines are filled to a visual margin — character count per line has lower CV than word count per line (0.658 vs 0.662). This is normal writing behavior, not the rigid counting you'd see in a columnar transposition cipher.

**Confidence: 80%**


## Finding 8: Multi-Word Phrases Exist But Are Rare

837 two-word phrases repeat 3+ times. Only 10 three-word phrases do. Zero four-word phrases repeat 3+ times.

Natural language text of this length would show hundreds of repeated 3-word phrases and dozens of 4-word phrases. The near-absence of longer repeated sequences is anomalous. It means either:
- The text has very little formulaic content (unusual for medieval religious/medical texts)
- The encoding disrupts phrase-level repetition while preserving word-level repetition
- The content is generated with local rules that don't produce long-range repetition

**Confidence: 75%**


## The Bond Pattern Verdict

The coordinate walk analysis gives 4,611 unique walk patterns across 36,351 words. Walk pattern entropy is 8.595 bits out of a possible 12.171 — a diversity ratio of 0.706. The walks are diverse but not maximally so. The most common single walk pattern accounts for 6.1% of all words. The top 15 patterns account for about 30%.

In the lattice framework, natural language produces walks where:
- Bond patterns are diverse (many different dimensional transitions)
- No single pattern dominates excessively
- The walk-distance distribution has moderate variance

The Voynich shows:
- Bond pattern diversity is LOW (1.93 bits, below the 2.0 threshold)
- One bond type (τχμ with φ=0) dominates at 58.6%
- Walk distances have moderate variance (CV = 0.56)

This is the signature of a system with **constrained phonotactics** — hard rules about what can follow what — rather than the freer combination seen in alphabetic European languages.


## Structural Assessment: What Is It?

### What the lattice rules out:

**Simple substitution cipher of a European language (90% confidence).**
Position locking, φ-collapse, and the extreme bigram constraints all survive as strong evidence against this. A substitution cipher preserves the statistical fingerprint of the plaintext language. The Voynich fingerprint does not match any known European language.

**Random meaningless text (85% confidence).**
Zipf's law, section variation, visual-margin line filling, and the existence of repeated phrases all argue against pure noise. Something is being communicated.

**Simple polyalphabetic cipher (80% confidence).**
No periodic structure in word-repeat distances. No columnar regularity.

### What the lattice supports:

**Constructed system with real underlying content (60% confidence).**
The statistics fall in an uncanny valley — too regular for natural language, too structured for random noise. The very low conditional entropy (2.395) combined with strict position locking suggests a **finite-state machine** generating words from a small set of rules. But the Zipf distribution and section variation suggest this machine was driven by real information.

The best structural match is a system where:
1. A small number of glyph-groups (perhaps 15-20 compound signs) serve as the actual encoding units
2. Word structure follows strict positional rules (initial set, medial set, final set)
3. The effective dimensionality is 3, not 4 — one phonetic dimension is collapsed
4. The system generates many surface word forms from a smaller inventory of meaning-bearing units

This could be:
- **A syllabary or abugida encoding an unknown language** (the collapsed φ and position-locking support this strongly)
- **A verbose cipher** where each plaintext letter maps to a Voynich word or word-part (explains the high TTR and few repeated phrases)
- **A steganographic system** where the real content is encoded in some subset of the visible text
- **A constructed philosophical language** (like Wilkins' Real Character, but 250 years earlier)

### The lattice's best guess:

The Voynich most likely encodes **real information** through a **constructed writing system** that is **not a simple cipher**. The writing system operates on a different structural principle than European alphabets — more like a syllabary with constrained phonotactics than like enciphered Latin.

The 3-dimensional effective space (τ,χ,μ with φ collapsed) is structurally similar to how Japanese kana or Korean jamo work — syllabic units with internal positional structure. The bond patterns within words follow a small, rigid grammar. The vocabulary is generated rather than arbitrary.

Whether the underlying content is in a natural language, an artificial language, or a systematic encoding of information (like alchemical recipes compressed into a table), I cannot determine from structure alone.


## What Would Settle It

The lattice gives us structural diagnosis but not content. To go further:

1. **Identify the compound glyphs.** The mandatory bigrams (`aiin`, `qokee`, `chedy`) should be treated as atomic units. Reanalyze with ~15-20 compound glyphs instead of ~30 basic ones. If this raises the conditional entropy toward 3.0+, the compound units ARE the alphabet.

2. **Cross-reference with known syllabaries.** Map the position-locked glyph classes (initial set, medial set, final set) against known syllabic writing systems. Does the inventory size match any known language family?

3. **Entropy-per-character after compound glyph resolution.** If the information rate per compound-glyph is 4-5 bits, the system is linguistically efficient. If it drops to 2-3, there's redundancy suggesting a cipher or padding.

4. **Section-specific vocabulary.** Extract the words unique to each section. If the herbal pages have different vocabulary than the astronomical pages (beyond shared function words), the system carries semantic content.


## Honest Assessment

The Voynich has defeated cryptanalysts for 100+ years. This analysis doesn't crack it. What it does:

- Establishes that the Voynich is NOT random noise. The lattice structure is clear.
- Establishes that it is NOT a simple alphabetic cipher. The distributional signature is wrong for that.
- Identifies the φ-collapse as a key structural constraint previously underappreciated.
- Shows that the writing system generates words through constrained rules, not free combination.
- Points toward compound-glyph analysis as the most productive next step.

The lattice sees a 3-dimensional writing system encoding real information through position-locked glyph classes with strict transition rules. It cannot say what that information is.

That's honest. That's what the data shows. Anything beyond this is speculation.
