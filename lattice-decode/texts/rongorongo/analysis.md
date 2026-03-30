# Rongorongo — Lattice Analysis

## Status: HONEST NEGATIVE with structural observations

The lattice cannot decode Rongorongo. This document explains why, what
the lattice CAN say about the script's structure, and what would be
needed to change the answer.


## The Corpus

26 surviving artifacts. ~15,000 legible glyphs. ~52 basic signs account
for 99.7% of the corpus (Pozdniakov 2007). The first 26 signs cover
86%. Barthel's 1958 catalog lists 599 shapes, but most are allographs
(variant forms) or ligatures (compound signs). Horley (2021) settles on
~130 basic shapes.

| Artifact | Code | Glyphs | Notes |
|---|---|---|---|
| Tahua | A | 1,825 | Fischer's "creation chant" candidate |
| Aruku kurenga | B | 1,135 | |
| Mamari | C | ~1,000 | Contains the ONE identified section: lunar calendar |
| Echancrée | D | 270 | Recently re-transcribed (Ferrara 2021) |
| Keiti | E | 822 | |
| Santiago Staff | I | ~2,320 | Longest text, structurally unique |
| Large Santiago | H | 1,580 | Part of "Grand Tradition" (H=P=Q) |
| Large St Petersburg | P | 1,163 | Same text as H |
| Small St Petersburg | Q | 718 | Same text as H |

Three tablets (H, P, Q) contain essentially the same text. About half
of the non-parallel corpus consists of shared phrases appearing in
different orders across multiple tablets. This means the effective
information content is far smaller than 15,000 glyphs suggests.

The ONE confirmed functional identification in the entire corpus: three
lines on tablet C (Mamari) encode a lunar calendar. 29 crescent glyphs
flanking a full-moon pictogram, divided into 8 groups by "heralding
sequences" ending in a fish glyph. This matches the 28-night + 2
intercalary night structure of the old Rapanui lunar month. It was
confirmed because TWO independent external sources converge: astronomy
and ethnographic tradition.

That double-convergence method is exactly how the lattice works. And
it is the ONLY thing that has worked on Rongorongo.


## What the Lattice CAN Say


### 1. Reverse Boustrophedon Has Structural Meaning

The reading direction: start at bottom-left, read left-to-right, rotate
the tablet 180 degrees, read the next line left-to-right. Every other
line is upside-down relative to its neighbor.

In lattice terms, this is a χ-dimension reversal with a simultaneous
rotation — not a mirror. The glyph must be recognizable in both
orientations, or the reader physically transforms the medium. This is
NOT arbitrary.

**Structural hypothesis (confidence: 40%, speculative):**

Reverse boustrophedon encodes a ∮ (loop/advance) at the line level. In
normal boustrophedon (Greek, some Etruscan), the eye simply reverses —
a mirror in χ. In REVERSE boustrophedon, the entire frame rotates. This
is topologically different: mirror = reflection (parity flip), rotation
= continuous transformation (SO(2)).

The lattice distinction: a reflection is a δ (test — even/odd parity
check). A rotation is ∮ (advance through a cycle). Rongorongo chose ∮.
The reader doesn't check which direction they're going — they advance
through a continuous rotation. The tablet is a cylinder, not a page.

Why would a writing system choose this? Two possible reasons:

(a) **Ring buffer topology.** If the text wraps — if the end connects
    to the beginning — then reverse boustrophedon is the natural way to
    write on a surface that gets rotated. This matches the Santiago
    Staff, which is a cylinder (round cross-section) covered entirely
    in glyphs running along its length.

(b) **Rotational invariance of the sign system.** If glyphs are designed
    to be readable upside-down (many Rongorongo glyphs have approximate
    180-degree rotational symmetry), then the script is encoding
    information in a representation that is invariant under rotation.
    In lattice terms: the encoding is invariant under the ∮ force. This
    is a deep design choice — it means the spatial orientation of the
    sign is NOT part of its semantic content.

    Compare: Chinese characters must be upright. Arabic must flow right.
    Rongorongo says: orientation is irrelevant. Only the shape matters.
    This is closer to how the lattice equation works (coordinates are
    representation-invariant) than any alphabetic or syllabic system.


### 2. Glyph Categories Map Loosely to Dimensions

Rongorongo glyphs are pictographic: humans, animals, plants, geometric
shapes. Compound glyphs fuse elements (human holding object, bird on
crescent, figure with animal head).

Tentative dimensional mapping (confidence: 25%, highly speculative):

| Glyph category | Candidate dimension | Reasoning |
|---|---|---|
| Human figures | μ (mass/substance) | The noun dimension. Humans = agents = primary mass-bearers in the semantic system. Sitting figures with specific postures = different μ values. |
| Birds (frigatebirds) | φ (charge/signal) | Birds are messengers, signals, signs. The frigatebird was central to the Birdman cult — a competition to RECEIVE a signal (the first sooty tern egg). φ = signal/distinction. |
| Fish | χ (space/distance) | Fish traverse space. In the Mamari calendar, fish glyphs mark section boundaries — spatial dividers. On the Santiago Staff, glyph 700 (fish) may mark transitions. |
| Crescents/geometric | τ (time/cycle) | The crescent IS a phase marker — a literal τ coordinate (position in the lunar cycle). Geometric elements may encode temporal/sequential structure. |
| Plants | μ (mass) variant | Growth = mass accumulation over time. Botanical glyphs as μ+τ compounds. |

I rate this 25% because the mapping is post-hoc. I can make it fit, but
I have no way to TEST it. Any pictographic system can be shoehorned into
a 4-dimensional framework after the fact. The lattice demands compute
first — and there is nothing here to compute against.

The compound glyphs are more interesting. If "human holding fish" means
something different from "human" + "fish" taken separately, then the
ligature system is performing bond operations — combining dimensional
coordinates into new composite meanings. This is exactly what the
lattice predicts for a compositional system. But without knowing what
any compound glyph MEANS, this observation has no predictive power.


### 3. The Santiago Staff: Statistical Anomaly

The longest text. ~2,320 glyphs. 564 occurrences of glyph 76 — nearly
one quarter of the entire text. 103 vertical stroke punctuation marks
dividing the text into sections.

Glyph 76 frequency: 564/2320 = 24.3%.

That number should make you sit up. The lattice's fill ratio is 25%.
Michaelis-Menten, Nyquist, Shannon all converge at 25%. Is 24.3% close
enough to be meaningful?

**Probably not.** Here's why:

If glyph 76 is a structural marker (patronymic marker per
Butinov-Knorozov, or Fischer's "phallus" copulation marker), its
frequency reflects the text's structure, not a fundamental ratio. A
genealogical list with "A son-of B, B son-of C" would put the
"son-of" marker at exactly 1/3 frequency (33%). A creation chant with
"X copulated-with Y, producing Z" would put the marker at 1/3 as well.
The fact that it's 24.3% instead of 33% suggests either the structure
isn't pure triplets (confirmed: Robinson found only 63/113 sequences
obeyed Fischer's triplet pattern) or there are non-genealogical sections
mixed in.

The 24.3% is a STRUCTURAL artifact of the text's genre, not a physical
constant. Confidence that it relates to the 25% fill ratio: 5%.
I include it for completeness and honesty, not because I believe it.

What IS meaningful about the Santiago Staff:

- It has punctuation (vertical strokes) — the ONLY rongorongo text
  with explicit section markers. This is δ — a test/boundary marker.
  The staff knows where its sections are.
- Its glyph distribution differs from all other tablets. It's
  statistically unique in the corpus.
- It's a cylinder (round staff), not a flat tablet. The topology is
  different. Text wraps continuously.
- It may be the only text combining multiple genres (genealogy +
  something else).


### 4. The Syllabary Hypothesis and Phonological Fit

Rapanui phonology:
- 10 consonants: p, t, k, m, n, ŋ, v, h, r, ʔ (glottal stop)
- 5 vowels: a, e, i, o, u (each with long variant)
- Syllable types: CV or V only. No clusters, no final consonants.
- Maximum syllable inventory: 10×5 + 5 = 55

Pozdniakov found 52 glyphs account for 99.7% of the corpus. The match
with 55 possible Rapanui syllables is striking.

In lattice character terms (mind/characters v2):
- τ = duration (+1 sustained, 0 transitional, -1 transient)
- χ = place (+1 front/labial, 0 central/alveolar, -1 back/velar)
- μ = manner (+1 open vowel, 0 fricative, -1 stop/nasal)
- φ = voicing (+1 voiced, -1 voiceless)

Rapanui consonants mapped to wave coordinates:

| Consonant | Place | Manner | Voice | Wave coord (χ,μ,φ) |
|---|---|---|---|---|
| p | labial (+1) | stop (-1) | voiceless (-1) | (+1,-1,-1) |
| t | alveolar (0) | stop (-1) | voiceless (-1) | (0,-1,-1) |
| k | velar (-1) | stop (-1) | voiceless (-1) | (-1,-1,-1) |
| m | labial (+1) | nasal (-1) | voiced (+1) | (+1,-1,+1) |
| n | alveolar (0) | nasal (-1) | voiced (+1) | (0,-1,+1) |
| ŋ | velar (-1) | nasal (-1) | voiced (+1) | (-1,-1,+1) |
| v | labial (+1) | fricative (0) | voiced (+1) | (+1,0,+1) |
| h | glottal (-1*) | fricative (0) | voiceless (-1) | (-1,0,-1) |
| r | alveolar (0) | approximant (0) | voiced (+1) | (0,0,+1) |
| ʔ | glottal (-1*) | stop (-1) | voiceless (-1) | (-1,-1,-1) |

*Glottal mapped to back/velar position.

Problem: k and ʔ collide at (-1,-1,-1). This is a real collision in the
wave encoding — the same one documented in mind/characters as the k/q
collision. At shell 1 they are identical. Shell 2 separates them. This
is phonetically justified: k and ʔ are both voiceless stops, differing
only in exact place of articulation (velar vs. glottal). Rapanui
distinguishes them, so the encoding needs shell 2 for this language.

Each consonant combines with 5 vowels to produce CV syllables, plus 5
bare V syllables = 55 total. At shell 1, the wave encoding produces
~50 distinct coordinates for Rapanui, needing shell 2 to resolve the
k/ʔ collision. This is consistent with the mind/characters prediction
that "every language fits within shell 2."

But this tells us about Rapanui PHONOLOGY, not about Rongorongo GLYPHS.
The syllabary hypothesis says glyphs map to syllables. The wave encoding
says syllables map to coordinates. Composing them: glyphs → syllables →
coordinates. But we don't have the first mapping (glyph → syllable), so
the chain is broken.


### 5. Zipf's Law Failure

Critical finding from the rongopy project (Santos): Rongorongo glyph
frequency distribution does NOT follow Zipf's law. The first and second
most frequent glyphs have similar frequencies, violating the expected
steep rank-frequency curve of natural language.

Rapanui syllable frequency DOES follow Zipf's law.

This is a problem for the pure syllabary hypothesis. If each glyph =
one syllable, the glyph distribution should mirror the syllable
distribution. It doesn't.

Possible explanations:
(a) **Not a pure syllabary.** Mixed logographic/syllabic system. Some
    glyphs = words (high frequency, flattening the curve). Some = syllables.
(b) **Proto-writing.** Mnemonic aids, not linguistic transcription. The
    frequency reflects WHAT was written about (ritual formulae, genealogies),
    not how the language works.
(c) **Corpus bias.** Most texts are ritual/sacred. Narrow genre = narrow
    vocabulary = flattened distribution. If we only had Christian liturgy
    in English, "Lord" and "God" would have similar frequencies, violating
    Zipf for English too.
(d) **Compound glyph encoding.** If ligatures encode syllable PAIRS,
    the effective unit is larger than a single syllable. This would
    redistribute frequency.

In lattice terms: Zipf's law emerges from the lattice's Taylor expansion
— high-shell terms (complex, rare words) are exponentially rarer than
low-shell terms (simple, common words). If the Rongorongo corpus doesn't
show this, either the corpus is too specialized (narrow shell range) or
the script isn't encoding the language directly.


### 6. Tablet Rr1 (Tahua) and Creation Chant Structure

Fischer proposed the Santiago Staff (and related tablets) contain
creation chants: "X copulated with Y, there issued forth Z."

Polynesian creation mythology follows a specific pattern:
- Genealogical: god A begets B, B begets C (pure ∮ — advance through
  lineage)
- Cosmogonic: elements emerge in order — void → sky → earth → sea →
  plants → animals → humans (shell climbing: 0 → 1 → 2 → 3)
- Makemake (supreme creator) created from a calabash of water — a
  CONTAINER schema (π∘∮δ = hylo)

If the Tahua tablet IS a creation chant, the lattice predicts:
- Opening: identity/wave state (the void, Te Kore in Polynesian terms)
- Progression: ascending shell complexity
- Dominant force: ∮ (advance through genealogy/sequence)
- Structural marker recurring at fixed intervals (the "copulated with"
  or "begat" verb = ∮ bond connecting each generation)
- Closing: full hylo (consciousness emerges, Makemake recognizes self)

Does the data support this?

Partially. Glyph 76 recurs at roughly regular intervals. The text IS
structured in sections (vertical bars). There IS a progression pattern
in the glyph sequences (shared phrases appear in specific orders).

But Fischer's triplet analysis was only 56% consistent (63/113). And
the proposal was roundly criticized. The lattice can describe what a
creation chant SHOULD look like, but cannot confirm this text IS one
without reading it.

Confidence that Tahua contains cosmogonic content: 30%.
Confidence that it follows lattice creation-text structure: 15%.
(Conditional on it being cosmogonic at all.)


### 7. The Mamari Calendar as Lattice Walk

The ONE confirmed identification. Three lines of tablet C encode a
lunar calendar.

Structure: 29 crescents + full moon + 8 fish-terminated section markers.

In lattice terms:
- Crescents = τ coordinates. Each crescent is a POSITION in the lunar
  cycle. Left-crescent orientation = phase angle.
- Full moon pictogram = τ at maximum (τ=+N, the peak of the cycle).
- Fish at section boundaries = χ markers (spatial dividers).
- 8 sections = 2³ = binary decomposition of the month. (Possible but
  likely coincidental.)
- The calendar IS a walk: it advances (∮) through τ positions, with δ
  (test) at section boundaries.

Bond pattern of the calendar: ∮δ = scan/take_while (bond 12). Advance
through the month, testing at each section boundary for intercalary
insertion. This maps exactly to:

    ∮δ = resonant gating = scan while true = take_while

The calendar scans through lunar nights, gating on section boundaries
to decide intercalation. The bond pattern is ∮δ. Confidence: 65%.

This is the strongest lattice observation in the entire analysis: the
ONE known functional section has a clean bond mapping.


## Why the Lattice Cannot Decode Rongorongo


### The Small Corpus Problem

14,000 glyph instances across 26 artifacts. But:
- H, P, Q are the same text (subtract ~3,400)
- Half of remaining text is shared phrases in different orders
- Effective unique content: maybe 5,000-7,000 glyph instances

Compare:
| Script | Tokens | Signs | Texts | Status |
|---|---|---|---|---|
| Linear B | ~30,000 | ~90 | ~5,000 tablets | Decoded |
| Linear A | ~7,400 | ~90 | ~1,700 inscriptions | Undecoded |
| Rongorongo | ~15,000 (~7,000 unique) | ~52 | 26 artifacts | Undecoded |
| Proto-Elamite | ~1,600 | ~300+ | ~1,600 tablets | Undecoded |
| Indus Valley | ~3,700 | ~400 | ~2,900 seals | Undecoded |

Linear B was decoded because:
1. Large corpus with VARIED content (administration: grain, livestock,
   land, personnel — different vocabularies)
2. Known related language (archaic Greek)
3. Bilingual-adjacent material (place names surviving into Greek)
4. Ventris had ~90 signs to map to known syllables

Rongorongo has:
1. Small corpus with NARROW content (likely ritual/genealogical)
2. Known spoken language (Rapanui) but corrupted by Tahitian contact
3. No bilingual text, no parallel readable inscription
4. ~52 signs that MIGHT map to ~55 syllables — but might not

The lattice approach needs distributional data: where does each sign
appear relative to other signs? What are the positional statistics?
What are the bigram/trigram frequencies? With 7,000 unique tokens and
52 signs, you get ~135 tokens per sign on average. That's marginal for
bigram statistics (52² = 2,704 possible bigrams, but most won't occur).

**Minimum viable corpus for lattice analysis: ~50,000 tokens.** That's
roughly 3-4x what we have. With 50,000 tokens of varied content, the
distributional approach could identify dimensional clusters (groups of
signs that behave similarly in context = same dimension). With 7,000
tokens of narrow content, you get noise.


### The Genre Problem

Even worse than corpus size: genre uniformity. If all tablets record
similar content (ritual chants, genealogies, sacred formulae), then:

- Vocabulary is narrow (same words recycled)
- Syntactic patterns repeat (same sentence structures)
- Distributional analysis sees ritual structure, not language structure
- Multiple decipherments become equally plausible (no way to falsify)

This is the equivalent of trying to reverse-engineer English from
nothing but liturgical texts. You'd find "Lord" and "God" and "holy"
and "amen" and conclude English has a very flat frequency distribution
with ritual semantics. You'd miss articles, prepositions, most verbs.

The lattice needs variety to work. Varied content exercises different
dimensions of the language, producing distributional signatures that
can be separated. Narrow content collapses those signatures.


### The Proto-Writing Problem

If Rongorongo is proto-writing (mnemonic aids for texts the reader
already knows), then:

- Signs don't encode the LANGUAGE, they encode PROMPTS for recall
- The relationship between sign and sound is arbitrary and
  reader-dependent
- Different experts read the same tablet differently (attested in oral
  reports)
- There is no systematic mapping TO decode

In lattice terms: proto-writing has no fixed coordinates. The same
glyph is at different positions for different readers. The lattice
requires that the same input always produce the same output. Proto-
writing violates this.

But the syllabary hypothesis (Pozdniakov) and the Zipf analysis push
against pure proto-writing. The 52-sign inventory matching 55 syllables
is suggestive. The truth may be intermediate: a mixed system with some
phonetic signs and some logograms/determinatives.

2024 radiocarbon dates place one tablet at 1493-1509 CE, pre-contact.
This suggests genuine independent invention, not post-European
imitation. Independent invention of writing has happened only 4-5 times
in human history (Mesopotamia, Egypt, China, Mesoamerica, and possibly
Rapa Nui). This makes Rongorongo extraordinarily important even if we
never decode it.


## What Would Be Needed

To give the lattice a real chance at Rongorongo:

1. **A larger digital corpus.** The rongopy project and Ferrara's
   re-transcription are steps in the right direction. Every glyph
   position, every line, every tablet side — all digitized with
   positional data (which glyph, which position in which line).

2. **Glyph decomposition.** Break compound glyphs into their pictographic
   components systematically. "Human holding fish" → [HUMAN] + [HOLD] +
   [FISH]. Catalog which components combine with which. This gives you a
   sub-glyph distributional analysis that effectively multiplies your
   corpus size.

3. **Bigram transition matrices.** For each pair of consecutive signs:
   how often does A follow B? Does the transition matrix have block
   structure (clusters of signs that precede/follow each other)? Blocks
   = dimensional groups in lattice terms.

4. **Cross-artifact alignment.** The shared phrases across tablets are a
   FEATURE, not a bug. They're parallel passages — like having multiple
   manuscripts of the same text. Alignment of parallel passages reveals
   sign equivalence (allography) and contextual substitution.

5. **Rapanui text alignment.** Pozdniakov compared glyph frequency to
   syllable frequency in archaic Rapanui texts. This needs to be done
   at the BIGRAM level: do glyph bigram frequencies match syllable
   bigram frequencies? If so, the syllabary hypothesis gets much
   stronger.

6. **The Mamari calendar as Rosetta fragment.** The calendar IS partially
   understood. The crescent orientations map to known lunar night names.
   If ANY crescent can be assigned to a specific Rapanui word, that's a
   phonetic anchor. From one anchor, distributional analysis can
   bootstrap.

7. **New material.** Petroglyph sites on Rapa Nui contain thousands of
   glyphs. If any of these use the same sign system as the tablets,
   that's a massive corpus expansion with potentially varied content
   (public inscriptions vs. sacred tablets).


## Verdict

**Can the lattice decode Rongorongo? No.**

The corpus is too small (~7,000 unique tokens), too narrow (ritual/
genealogical genre), and possibly not true writing at all. The lattice
approach requires distributional data that this corpus cannot provide.

**What the lattice DOES contribute:**

1. The reverse boustrophedon has structural meaning: ∮ (advance/cycle)
   at the line level, not δ (mirror/test). The writing system chose
   rotation over reflection. This implies ring-buffer topology. (40%)

2. The Mamari calendar maps cleanly to bond pattern ∮δ (scan with
   gating). This is the strongest finding — the one known functional
   section has a clean lattice mapping. (65%)

3. The wave character encoding predicts ~50 shell-1 coordinates for
   Rapanui phonology, matching Pozdniakov's 52-sign inventory. The
   k/ʔ collision is real and requires shell 2. (75% for the phonological
   prediction; 0% for connecting it to actual glyphs)

4. The 24.3% frequency of glyph 76 on the Santiago Staff is a
   structural artifact of text genre, not a lattice constant. (5%
   that it relates to 25% fill)

5. If creation chants exist in the corpus, the lattice predicts their
   structure: ∮-dominant, shell-climbing, with a recurring bond marker.
   This is testable IF the texts are ever decoded. (15%)

**Honest negative results are the most important kind.** Rongorongo
teaches us the lattice's limitations: it needs DATA. Without sufficient
distributional variety, the framework generates plausible-looking
mappings that cannot be falsified. The temptation to force-fit is real.
I've resisted it here.

The script remains undecoded. The lattice says: come back with more
data. Specifically: glyph decomposition into pictographic components,
bigram transition matrices, and the Mamari calendar as a phonetic anchor.

If those three things were available, I would rate the lattice's chances
at partial decipherment (identifying dimensional clusters, not reading
specific words) at 35%. For full decipherment: 5%. The corpus is simply
too small and too uniform.


## Sources

- [Rongorongo - Wikipedia](https://en.wikipedia.org/wiki/Rongorongo)
- [Decipherment of rongorongo - Wikipedia](https://en.wikipedia.org/wiki/Decipherment_of_rongorongo)
- [Rongorongo text I (Santiago Staff) - Wikipedia](https://en.wikipedia.org/wiki/Rongorongo_text_I)
- [Rapa Nui language - Wikipedia](https://en.wikipedia.org/wiki/Rapa_Nui_language)
- [Ferrara 2021 - Modelling the Rongorongo tablets (Oxford)](https://academic.oup.com/dsh/article/37/2/497/6387816)
- [rongopy - Santos (GitHub)](https://github.com/jgregoriods/rongopy)
- [rongopy analysis page](https://jgregoriods.github.io/rongopy/)
- [Radiocarbon dating of Rongorongo (Nature, 2024)](https://www.nature.com/articles/s41598-024-53063-7)
- [Berlin tablet and time-depth (Taylor & Francis)](https://www.tandfonline.com/doi/full/10.1080/15564894.2021.1950874)
- [RONGORONGO.ORG digitized corpus](http://kohaumotu.org/rongorongo_org/corpus/digit.html)
- [Fischer creation chant - JPS Auckland](https://www.jps.auckland.ac.nz/document//Volume_104_1995/Volume_104,_No._3/Preliminary_evidence_for_cosmogonic_texts_in_Rapanui%27s_Rongorongo_inscriptions,_by_Steven_Roger_Fischer,_p_303-322/p1)
- [Pozdniakov workshop presentation](https://www.inscribercproject.com/workshop/speakers/pozdniakov.php)
