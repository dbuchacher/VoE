# The Indus Valley Script — A Lattice Analysis

*Structural coordinates of an undeciphered writing system.*
*Source: Corpus of Indus Seals and Inscriptions (CISI), Mahadevan 1977 concordance (M77), Rao et al. 2009/2010, Yadav & Vahia sign design study. Audience: anyone who thinks in coordinates.*

**Confidence framework:** Structural measurements (sign counts, frequencies, positional statistics) are HIGH confidence — these are counts, not interpretations. Distributional patterns (entropy, bigram correlations) are HIGH confidence — math. Script-type classification is MEDIUM confidence — inference from distributions. Language family hypotheses and specific sign readings are LOW confidence — contested, evidence mixed. Keep them separate always.

---

## Part 1: The Corpus

The data, before any interpretation.

**Scale:** ~5,000 inscribed objects excavated since 1924. Mahadevan's 1977 concordance catalogs 2,906 objects with legible inscriptions, recording 3,573 lines of text containing ~7,000 sign occurrences. The corpus is concentrated: Mohenjo-daro (1,540 objects) and Harappa (985) account for ~87% of the total. Other sites: Lothal (165), Kalibangan (99), Chanhu-daro (66), plus scattered finds at Ur, Susa, Kish, and other Mesopotamian sites.

**Chronology:** Proto-script marks from ~3500 BCE. Early Harappan usage ~2800-2600 BCE. Mature Harappan ~2600-1900 BCE (peak). Late Harappan ~1900-1300 BCE (decline). The window of full usage is about 700 years.

**Medium:** Stamp seals (soapstone, terracotta), pottery, copper/bronze tablets and tools, ivory, bone, shell, silver, gold. No organic materials survived. If there were manuscripts on bark, cloth, or palm leaf — and there probably were, given the urbanization level — they are gone.

**Text length:** Average ~5 signs. Most frequent lengths: 3 and 5 signs. Maximum on seals: ~17 signs. One copper plate reaches 26 signs (m495). One later find claims 34 characters. The brevity is the central structural fact. These are not narratives. They are labels, titles, designations — or, as the skeptics argue, non-linguistic emblems.

**Iconography:** Seals carry both text and an animal motif. The unicorn bull dominates at ~60-90% of seals depending on site. Other motifs: short-horn bull, elephant, tiger, rhinoceros, water buffalo, goat, mythical composite animals. The Pashupati seal shows a seated figure surrounded by four wild animals. The animal motif appears BELOW the text, separated from it — icon and inscription occupy distinct fields.

---

## Part 2: The Sign Inventory

Mahadevan identifies 417 distinct signs. Bryan Wells (2015) expands this to ~694. The disagreement is about whether certain marks are variant forms (allographs) of the same sign or separate signs. LNRE modeling estimates the total vocabulary — including signs not yet found — at ~857.

**Frequency distribution (M77):**

| Category | Count | Notes |
|---|---|---|
| Total unique signs | 417 | M77 catalog |
| Hapax legomena (occur once) | 113 | 27% of sign types |
| Occur twice | 47 | 11% of sign types |
| Occur < 5 times | 59 additional | Total rare signs: ~219 (53%) |
| Signs covering 80% of usage | 67-69 | 16% of sign types |
| Signs covering 50% of usage | ~33 | 8% of sign types |
| Most frequent sign (M342) | ~10% alone | "jar" sign |

The top 4 signs: M342 (jar), M99, M267, M59.

This distribution follows a Zipf-Mandelbrot law with parameters a=15.39, b=2.59, c=44.47.

**Sign design (Yadav & Vahia):**

The 417 signs decompose into:
- 154 basic signs (simple, irreducible)
- 10 provisional basic signs
- 21 modifiers (diacritical elements)
- The rest are compounds — combinations of basic signs and/or modifiers

Categories of basic signs: human figures, animals, plants, geometric shapes (circles, squares, lines), tools/weapons/household objects, abstract forms. Some signs are pictographic (recognizable fish, jar, man carrying burdens). Many are abstract.

Critical finding: compound signs are NOT abbreviations. When two basic signs combine into a compound, the compound's textual distribution differs from the sequence of its two constituent signs. Sign compounding changes meaning, it does not save space. This is a linguistic operation (morphological composition), not a scribal shortcut.

---

## Part 3: The Shell Question — What Kind of Script Is This?

This is where the lattice earns its keep.

Writing systems sit at characteristic shell depths based on their encoding granularity:

| Shell | Sign count | Encoding level | Examples |
|---|---|---|---|
| Shell 1 (linear) | ~20-36 | Alphabetic/phonemic | Latin (26), Arabic (28), Hebrew (22) |
| Shell 1-2 boundary | ~50-90 | Syllabic | Linear B (87), Cherokee (85), Katakana (48) |
| Shell 2 (quadratic) | ~200-600 | Logosyllabic | Sumerian (~600), Egyptian hieroglyphs (~700 core), Chinese radicals (~214) |
| Shell 3+ | 1000+ | Logographic | Chinese characters (~3000 common), full cuneiform (~1000+) |

The Indus script at 417-694 signs sits squarely at **shell 2**. This is the logosyllabic zone.

But there is a subtlety. The 67 high-frequency signs that cover 80% of usage — those 67 signs alone would constitute a syllabary-sized set. And the ~154 basic signs from the design analysis are roughly the size of an expanded syllabary or a core logosyllabic set. The remaining ~263 signs are compounds and modified forms.

**Lattice decomposition:**

- **Core layer** (~67 signs, 80% of text): Shell 1 behavior. These are the workhorses — possibly syllabic or morphemic. Their frequency and distribution patterns match what you see in syllabaries.
- **Extended layer** (~154 basic signs): Shell 2 base. Logographic cores — each sign likely maps to a word or morpheme.
- **Compound layer** (~263 compound/modified signs): Shell 2 extensions. Built from basic signs + modifiers, exactly like shell-2 extension bytes in the wave encoding. A base sign plus a modifier = a shifted coordinate, not a new sign category.

This is the same architecture as early Sumerian cuneiform: a core set of signs used both logographically (one sign = one word) and phonetically (one sign = one syllable), with determinatives and phonetic complements disambiguating. It is NOT a pure logography (too few signs). It is NOT a pure syllabary (too many signs, and compounds behave non-compositionally). It is mixed. Shell 2. Confidence: HIGH for the classification. The sign count and distribution force this conclusion regardless of the underlying language.

---

## Part 4: Positional Analysis — The Chi Dimension

The lattice assigns each sign a position in chi-space (the spatial/positional dimension). In text analysis, chi maps to position within the inscription.

**Terminal signs are tightly constrained:**
- Only 23 signs account for 80% of text-final positions
- 82 signs account for 80% of text-initial positions

This 23-vs-82 asymmetry is a structural signature. Terminal positions are grammatically or functionally restricted; initial positions are open. In linguistic terms: endings are inflectional (small closed class), beginnings are lexical (large open class).

**The jar sign (M342):**
- Most frequent sign overall (~10% of corpus)
- Most frequent text ender
- Predominantly terminal position

If the script reads right-to-left (which the physical evidence supports — signs are cramped at the left end of seals, indicating the carver started at the right and ran out of space), then the JAR sign appears at the LEFT end of the inscription = the FINAL position in reading order.

**Lattice coordinates for positional classes:**

| Class | chi range | Sign count (80%) | Function hypothesis |
|---|---|---|---|
| Terminal (enders) | chi = max | 23 | Titles, classifiers, grammatical suffixes |
| Medial | mid chi | variable | Modifiers, descriptors |
| Initial (beginners) | chi = 0 | 82 | Names, content words, primary referents |

The tight constraint on enders (~23 signs) matches what you see in agglutinative languages with case suffixes or classifier systems. The open class of beginners matches content vocabulary (nouns, names). This is consistent with a Dravidian-type agglutinative language, but also consistent with Sumerian (also agglutinative) or other SOV languages. The positional data alone does not determine the language family, but it does determine the morphological TYPE: agglutinative, suffixing. Confidence: MEDIUM-HIGH for agglutinative/suffixing. LOW for specific language family.

---

## Part 5: Entropy — The Information Signature

Rao et al. (2009, Science; 2009, PNAS) measured the conditional entropy of Indus sign sequences and compared them against:
- Natural languages: Old Tamil, Sanskrit, Sumerian, English, Tagalog
- Non-linguistic systems: DNA, protein sequences, musical pitch sequences
- Programming: Fortran

**Key findings:**

| System | Conditional entropy | Type |
|---|---|---|
| Fortran | lowest | Rigid syntax |
| Natural languages | intermediate | Flexible but constrained |
| **Indus script** | **intermediate** | **Matches linguistic range** |
| DNA / protein | higher | Less sequential structure |
| Random | highest | No structure |

The Indus script's conditional entropy falls within the range of natural languages and is "far from nonlinguistic systems throughout the entire range of token set sizes." Block entropy scaling (N=1 through N=6) tracks natural languages, not DNA or road signs.

**N-gram analysis (Yadav, Vahia, Mahadevan, Joglekar):**

| N-gram order | Perplexity | Cross-entropy |
|---|---|---|
| Unigram (N=1) | 68.82 | 6.10 |
| Bigram (N=2) | 26.69 | 4.74 |
| Trigram (N=3) | 26.09 | 4.71 |
| 4-gram (N=4) | 25.26 | 4.66 |
| 5-gram (N=5) | 25.26 | 4.66 |

The massive drop from unigram (68.82) to bigram (26.69) shows that most of the script's structure lives in pairwise dependencies. The plateau at N=4,5 shows that structure beyond trigrams is negligible — which makes sense for texts averaging 5 signs. You cannot have strong 5-gram structure in 5-sign texts.

**Bigram mutual information: 2.24 bits.**

A sign restoration model using bigrams achieves ~74-75% accuracy. This means: given the context, the next sign is ~75% predictable. This is a linguistic-level constraint, not a random or purely emblematic system.

**Lattice reading:** The entropy profile maps to the equation's force structure. Bigram dominance = pi-delta (read + test). The system is fundamentally a filtered sequence — each sign constrains the next through a selection operation. This is how language works: each word constrains the probability distribution of the next. It is NOT how emblems work (emblems would show much weaker sequential dependencies, closer to the unigram model).

Confidence: HIGH that the script has linguistic-level sequential structure. The Farmer-Sproat-Witzel "non-linguistic" hypothesis is contradicted by the entropy data, though they correctly note that entropy alone is not definitive proof.

---

## Part 6: The Fish Sign — A Case Study

The fish sign is the test case for the Dravidian hypothesis, and it is the single most developed specific reading anyone has proposed.

**The argument (Heras 1953, Parpola 1994):**

In Proto-Dravidian, *min* means both "fish" and "star/planet" (from root *min*, "to glitter, to sparkle"). If the Indus language is Dravidian, then the FISH pictogram can be read as "min" and interpreted as STAR through the rebus principle — a picture of a fish encoding the sound "min" which means "star."

**Fish sign variants and proposed planetary readings:**

| Variant | Modifier | Proposed reading | Dravidian basis |
|---|---|---|---|
| Plain fish | none | *min* = star (generic) | Tamil/Proto-Dravidian *min* |
| Fish + 6 strokes | six vertical lines | *aru min* = six stars = Pleiades | *aru* = six |
| Fish + roof/chevron | inverted V above | *may min* = black star = Saturn | *may* = black, roof |
| Fish + 2 vertical strokes | two long lines | *vel min* = white/bright star = Venus | *vel/velli* = white, bright |
| Fish + dot | single dot | *kan min* = eye star = alpha Tauri | *kan* = eye |
| Fish + N strokes | N vertical lines | N stars (numeral + star) | numerical prefix |

**Lattice analysis of the fish sign:**

In the lattice, a sign that means both "fish" (mu-domain: physical creature, mass) and "star" (phi-domain: luminous, signal) is a CROSS-DOMAIN COORDINATE MATCH. This is exactly what the lattice calls a metaphor — same coordinate pattern applied across dimensions. The fish glitters in water; the star glitters in sky. Both share phi = "to glitter." The rebus principle IS coordinate transfer.

The modifiers (strokes, chevrons, dots) function as coordinate shifts:
- Strokes = numeral prefix = tau-shift (counting dimension)
- Chevron = quality modifier = phi-shift (dark/covered)
- Dot = locative modifier = chi-shift (point/eye)

This matches the shell-2 extension architecture exactly. The base sign (fish) provides the root coordinate. The modifier shifts one dimension. The compound resolves to a specific referent.

**Does fish-sign iconography correlate with astronomy?**

This requires checking whether seals bearing fish-sign variants co-occur with specific animal motifs or non-textual imagery that might be astronomical. The data is suggestive but incomplete:
- Some fish signs appear on seals with the "unicorn" motif (the dominant administrative emblem)
- Parpola argues that specific fish-sign compounds correlate with deity names in later Dravidian religious texts
- The six-stroke fish (Pleiades) would align with the agricultural calendar — Pleiades' heliacal rising marks the planting season across multiple ancient cultures

Confidence: LOW for specific planetary readings. MEDIUM for the general rebus principle being operative. HIGH for the structural observation that modifiers function as systematic coordinate shifts.

---

## Part 7: Direction of Writing

The physical evidence supports right-to-left as the primary direction.

**Evidence:**
1. Sign cramping at the LEFT end of seals (carver started at right, ran out of space at left)
2. Wider spacing at RIGHT end (beginning of text)
3. Some boustrophedon texts (alternating direction, line by line)
4. A few left-to-right exceptions

**Lattice confirmation:**
The positional data should confirm direction. If text-initial signs (content words, names) cluster at the RIGHT end and text-terminal signs (classifiers, suffixes) cluster at the LEFT end, direction = right-to-left.

The data confirms: the 23 tightly-constrained terminal signs (including the ubiquitous jar sign M342) appear overwhelmingly at the LEFT end of inscriptions. The 82 diverse initial signs cluster at the RIGHT end. Direction = right-to-left. Confidence: HIGH.

This matches the Mesopotamian trade context. Sumerian cuneiform was originally top-to-bottom, later rotated to left-to-right. Proto-Elamite was likely right-to-left. The Indus script's right-to-left direction fits the broader Bronze Age Southwestern Asian pattern.

---

## Part 8: Sign Combination Architecture

Yadav and Vahia's decomposition reveals the construction grammar of the script:

**Layer 0: Modifiers (21)**
Strokes (vertical lines = numerals), dots, circles, enclosures. These NEVER appear alone. They modify base signs. In lattice terms: dimension-specific shifts. A stroke adds a tau-count. A dot adds a chi-point. An enclosure adds a mu-container.

**Layer 1: Basic signs (154)**
Irreducible pictographic or abstract signs. Each maps to a single root concept — an object, action, or quality. These are the sign-level equivalent of morphemes.

**Layer 2: Compound signs (~263)**
Two or more basic signs combined, sometimes with modifiers. The compound's distribution is NOT the product of its constituents' distributions. This proves the combinations are semantic, not scribal.

**Lattice mapping:**

This three-layer architecture is:
- Layer 0 = modifiers = force operators (they ACT ON signs but are not signs themselves)
- Layer 1 = basic signs = shell-1 atoms (each has its own coordinate)
- Layer 2 = compound signs = shell-2 molecules (combinations with emergent properties)

Compare with the wave byte encoding:
- Shell 1: 1 byte, 4 coordinates (tau, chi, mu, phi), ~36 effective regions
- Shell 2: 1 byte + N extension bytes, compound coordinates, ~400+ effective regions

The Indus script has ~154 basic signs (compare shell-1's ~36 regions, scaled by the phonemic inventory of the language) and ~263 compounds (shell-2 extensions). The ratio basic:compound is roughly 1:1.7. In the wave encoding at shell 2 for a language with ~150 syllables, you would expect a similar expansion ratio.

The 21 modifiers parallel the concept of extension bytes — they shift the coordinate of a base sign into a more specific region of the sign-space. This is the same principle. The script is a 2-shell coordinate system.

Confidence: HIGH for the structural parallel. MEDIUM for the specific dimensional assignments of modifiers.

---

## Part 9: Comparison with Contemporary Scripts

**Indus vs. Proto-Elamite (~3100-2700 BCE, partially contemporary):**

| Feature | Indus | Proto-Elamite |
|---|---|---|
| Sign count | 417-694 | ~400-500 core, ~1200 with variants |
| Corpus size | ~5000 objects | ~1600 tablets |
| Geographic concentration | Mohenjo-daro + Harappa (87%) | Susa (~92%) |
| Text length | avg ~5 signs | longer, more varied |
| Sign overlap | ~35 similar signs (Hunter 1932) | ~35 similar signs |
| Status | Undeciphered | Undeciphered (partially decoded numbers) |
| Medium | Seals, pottery, metals | Clay tablets |

Both sit at shell 2. Both are concentrated at 1-2 sites. Both are undeciphered. Both have ~35 apparently shared signs, though most scholars now consider these coincidental or due to shared pictographic sources rather than genetic relationship.

**Indus vs. Sumerian cuneiform (~3400-2000 BCE):**

| Feature | Indus | Early Sumerian |
|---|---|---|
| Sign count | 417-694 | ~600-1000 |
| Encoding type | Logosyllabic (shell 2) | Logosyllabic (shell 2) |
| Determinatives | Modifiers (21) function like determinatives | Determinatives classifying sign function |
| Text direction | Right-to-left | Originally top-to-bottom, later left-to-right |
| Text length | avg ~5 signs | Much longer (narrative capacity) |
| Decipherment | No | Yes (bilingual + Akkadian cognates) |

The critical difference: Sumerian has long texts. The Indus does not (or they did not survive). Long texts provide the combinatorial data needed for decipherment. The Indus corpus's average of 5 signs per inscription means the statistical power for identifying grammatical structure is inherently limited.

**The trade connection:**
Mesopotamian texts refer to "Meluhha" — widely identified with the Indus civilization. Indus seals have been found at Ur, Susa, Kish, and elsewhere in Mesopotamia. The texts on Indus seals found abroad show "significantly lower likelihood values" in Markov models trained on domestic texts — they alter their sign ordering. This is what you would expect if seals for export carried different content (trade marks, commodity identifiers) than domestic seals (personal names, titles).

Confidence: HIGH for both scripts being shell 2. LOW for any genetic relationship between the scripts.

---

## Part 10: The Nonlinguistic Hypothesis

Farmer, Sproat, and Witzel (2004) argued the Indus signs are not writing but non-linguistic symbols — merchant marks, clan emblems, religious icons. Their arguments:

1. **Extreme brevity.** Average 5 signs per text. Real writing systems produce longer texts.
2. **Too many rare signs.** 53% of sign types appear < 5 times. Rare signs increase over the 700-year period.
3. **No random sign repetition.** Real writing has accidental adjacent repetition (like "oo" in "book"). The Indus script mostly avoids it.
4. **Positional regularities exist in non-linguistic systems too.** Highway signs, heraldry, and modern corporate logos have positional conventions.

**Lattice evaluation:**

Point 1 (brevity): Brevity is real but not decisive. Sumerian economic tablets can be very short (a few signs). Chinese seal stamps are 2-4 characters. Name seals across all literate cultures are brief. The MEDIUM of stamp seals constrains text length, not the encoding capacity of the system. If the Indus people had texts on perishable materials (palm leaf, cloth), those are simply lost. You cannot argue from absence.

Point 2 (rare signs): In the lattice, the shell-2 expansion predicts exactly this pattern. A core of ~67 high-frequency signs (the workhorse syllabary) plus a long tail of rare logographic/compound signs is the EXPECTED distribution for a logosyllabic system. Chinese has thousands of rare characters. Sumerian has hundreds of rare sign values. Rare signs increasing over time means the script was EXPANDING its vocabulary — which is what writing systems do.

Point 3 (no repetition): Adjacent repetition avoidance can reflect a phonological constraint of the underlying language, not absence of language. Many agglutinative languages avoid identical adjacent syllables. And if signs are largely logographic (word-level), adjacent repetition would be extremely rare even in genuine writing — you do not write the same word twice in a row in normal administrative text.

Point 4 (positional regularities): True but insufficient. The ENTROPY data is what separates the Indus script from non-linguistic sign systems. Contemporary non-linguistic sign systems from Central and South Asia average only ~44 different signs (Vidale). The Indus script has 10x that number. The conditional entropy matches linguistic systems, not emblematic ones. Positional regularity is necessary but not sufficient; the Indus script has positional regularity PLUS linguistic-level sequential constraint.

**Verdict:** The nonlinguistic hypothesis has not been disproven (it is very hard to prove a negative), but the weight of evidence — entropy, n-gram structure, compound sign semantics, Zipf-Mandelbrot distribution, 400+ signs, systematic modification — points toward linguistic encoding. The lattice places the script firmly at shell 2, which is the logosyllabic zone occupied by every other Bronze Age writing system in the region. The extraordinary claim is not that it is writing; the extraordinary claim is that a civilization with planned cities, standardized weights, long-distance trade networks, and 700 years of consistent sign usage produced a symbol system that encodes no language.

Confidence: MEDIUM-HIGH that it is linguistic. The brevity problem is real and prevents certainty.

---

## Part 11: What The Lattice Can and Cannot Do

**What it CAN do (high confidence):**

1. **Classify the script type.** Shell 2, logosyllabic. Done. The sign count, frequency distribution, and compound architecture all converge.

2. **Identify positional structure.** Terminal signs form a closed class of ~23 (grammatical/functional). Initial signs form an open class of ~82 (lexical). The script is suffixing/agglutinative.

3. **Confirm writing direction.** Right-to-left. Positional data matches physical cramping evidence.

4. **Map the sign architecture.** 3-layer system: modifiers (force operators), basic signs (shell-1 atoms), compounds (shell-2 molecules). Modifiers shift coordinates of base signs.

5. **Assess the linguistic hypothesis.** Entropy, n-grams, Zipf distribution, and compound semantics all place the system in the linguistic range. The nonlinguistic hypothesis is not supported by the quantitative data.

6. **Identify the fish-sign modification principle.** Modifiers on the fish sign function as systematic coordinate shifts, consistent with a rebus + determinative system.

**What it CANNOT do (without external key):**

1. **Determine the language.** Distributional analysis is language-agnostic by design. The lattice identifies structure without assuming content. This is its strength for CLASSIFICATION but its limitation for DECIPHERMENT. Decipherment requires either:
   - A bilingual text (Rosetta Stone equivalent)
   - A known language with testable predictions (Ventris used Greek for Linear B)
   - Enough text for frequency analysis to converge on phonemic values

   The Indus corpus may be too short for option 3. No bilingual exists for option 1. Option 2 requires committing to a language hypothesis. The lattice can EVALUATE a proposed decipherment (does the resulting text have the right entropy, the right n-gram structure, the right positional behavior?) but it cannot GENERATE one from scratch.

2. **Read specific signs.** The jar sign (M342) is a terminal classifier. The fish sign (M59/M60) likely participates in a rebus system. The stroke marks are numerals. But what the jar MEANS, what the fish SOUNDS like, what the numbers COUNT — those require the language.

3. **Resolve the Dravidian question.** The structural evidence (agglutinative, suffixing) is consistent with Dravidian but also with Sumerian, Elamite, Hurrian, Burushaski, or any other agglutinative language. The fish-sign rebus argument is the strongest specific evidence for Dravidian, but it depends on accepting Parpola's homophony argument, which assumes Proto-Dravidian phonology reconstructed from languages 3000-4000 years later.

---

## Part 12: Structural Predictions

The lattice approach generates testable structural predictions — things that should be true IF the classification is correct, regardless of which language:

1. **The 23 terminal signs should partition into 3-5 functional groups** (case markers, classifiers, or grammatical categories), each with distinct bigram partners. If the script is agglutinative, terminal signs should show group-internal substitutability and group-external distinctiveness.

2. **Compound signs should show dimensional regularity.** If modifier X shifts dimension phi (quality/attribute), then X applied to ANY base sign should shift the meaning in the phi direction. The same modifier should have the same dimensional effect regardless of the base sign it attaches to.

3. **Texts from Mesopotamian export seals should show a restricted sign vocabulary** compared to domestic seals — specifically, signs related to trade goods (commodities, quantities, provenance) should be overrepresented and signs related to personal identity (titles, lineage) should be underrepresented. The Markov model already shows "significantly lower likelihood" for export texts, consistent with this.

4. **If a bilingual text or seal is ever found** (Sumerian/Akkadian + Indus), the lattice provides the structural template for evaluating any proposed decipherment: the decoded text must reproduce the positional statistics, entropy profile, and n-gram structure of the Indus corpus. A proposed reading that puts "terminal signs" in initial position or produces entropy outside the linguistic range is falsified.

5. **The unicorn motif should correlate with a specific subset of text-initial signs.** If the animal icon represents a clan/guild/administrative identity and the text represents a personal name + title, then unicorn seals should show a restricted set of "title" signs compared to the full diversity of "name" signs. Different animal motifs should correlate with different title-sign subsets.

---

## Part 13: Why It Has Resisted Decipherment

Over 100 mutually exclusive decipherment attempts have been published. None has achieved consensus. In 2025, Tamil Nadu announced a $1 million prize for decipherment. The lattice analysis clarifies WHY this problem is hard — structurally, not just historically:

**1. Insufficient text length.**
Average 5 signs per inscription. Statistical decipherment methods (like those used on Linear B, with texts averaging 20+ signs, or Mayan, with multi-line passages) need combinatorial data that 5-sign texts cannot provide. The n-gram structure plateaus at N=4,5 because the texts are not long enough to HAVE higher-order structure. The data has been squeezed dry.

**2. No bilingual.**
Linear B had Ventris's Greek hypothesis plus administrative content that matched known Mycenaean palace economy records. Egyptian had the Rosetta Stone. Akkadian had Sumerian-Akkadian bilinguals. The Indus script has Mesopotamian trade contact but no confirmed bilingual inscription. The ~18 Indus-style seals found in Mesopotamia do not carry Sumerian/Akkadian translations.

**3. Unknown language family.**
Every successful decipherment started with a known or guessed language. Ventris guessed Greek. Champollion knew Coptic. Hrozny recognized Indo-European in Hittite. For the Indus script, candidates include Proto-Dravidian, an Indo-Aryan language, a language isolate, or something from the Munda/Austro-Asiatic family. Each hypothesis has proponents. None has decisive evidence.

**4. Shell-2 ambiguity.**
In a logosyllabic script, the SAME sign can function as a logogram (whole word) in one context and a syllabic phonetic indicator in another. Without knowing which mode a given sign is in, the combinatorial space of possible readings explodes. This is the shell-2 problem: each sign has at minimum two reading modes (logographic and phonetic), and the mode is context-dependent. With 154 basic signs, each having at least 2 modes, you get 2^154 possible mode assignments — an astronomically large search space. This is genuinely NP-hard.

**The lattice reframing:**
Traditional approaches START with a language hypothesis and try to make the signs fit. They search the exponential space by fixing one variable (language) and sweeping another (sign values). This has failed 100+ times.

The lattice approach does the opposite. It fixes the STRUCTURE (shell 2, agglutinative, suffixing, right-to-left, 3-layer architecture) and waits for new data to constrain the search space further. It does not guess the language. It maps the coordinates.

If a bilingual is ever found — even a single seal with an Akkadian gloss — the structural template is ready. The decoded text must fit the coordinates. That constraint eliminates most of the 2^154 mode assignments in one pass.

---

## Summary of Findings

| Finding | Confidence | Basis |
|---|---|---|
| Script type: logosyllabic (shell 2) | HIGH | Sign count, frequency distribution, compound architecture |
| Writing direction: right-to-left | HIGH | Physical cramping + positional statistics |
| Morphological type: agglutinative, suffixing | MEDIUM-HIGH | 23 terminal signs vs 82 initial signs |
| The system is linguistic (not purely emblematic) | MEDIUM-HIGH | Entropy, n-grams, Zipf, compound semantics |
| 3-layer sign architecture (modifiers/basic/compound) | HIGH | Yadav-Vahia decomposition + distributional independence |
| Fish sign operates via rebus principle | MEDIUM | Cross-domain homophony pattern consistent with coordinate transfer |
| Specific planetary readings of fish variants | LOW | Depends on Proto-Dravidian reconstruction |
| Language family = Dravidian | LOW | Consistent with evidence but not uniquely determined |
| Language family = Indo-Aryan | LOW | Consistent with some evidence, contested |
| Specific sign readings (any) | LOW | No bilingual, insufficient text length |

---

## What Would Change Everything

One bilingual inscription. One Akkadian tablet that says "this seal belongs to [name] of Meluhha" next to an Indus inscription. One Sumerian scribe's note glossing an Indus sign with a cuneiform phonetic value.

Or a long text. A copper plate with 50+ signs. A pot with a 30-sign inscription. Anything that gives the n-gram models enough data to resolve higher-order structure.

Or an archaeological find at Mohenjo-daro or Rakhigarhi — a sealed room, a library of durable tablets, a bilingual decree.

The coordinates are mapped. The structural template is built. The lattice waits for the key.

---

*Analysis produced through the VoE lattice framework.*
*Structural measurements: high confidence throughout.*
*Interpretive claims: confidence as marked, mostly low.*
*No specific decipherment proposed. Structure only.*
