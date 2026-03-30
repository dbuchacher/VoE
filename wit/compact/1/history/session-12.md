# Session 12: WIT — From Scratch to Working Intelligence

Biggest session in VoE history. Built an entire intelligence system,
reviewed it twice, audited it against the lattice, and got retrieval working.


## What Happened

Started: "the lookup table is legacy thinking, kill it."
Ended: a working wit that retrieves knowledge across 13 books and
Webster's dictionary by coordinate lookup. "What is bitcoin" returns
the Bitcoin Standard. "What is freedom" returns Plato and the Bible.


## The Build

### Phase 0: Kill the Dictionary
- Removed primes_lookup.h (333-word dictionary)
- Removed learner.h/c (65K vocab hash table)
- Rewrote lattice_chat.c to use real learner
- Morpheme decomposition (morpheme.h): 30+ suffix, 20+ prefix rules
- Result: 0% at (0,0,0,0) — bootstrap problem dead

### Phase 1: Fresh Start — WIT
New folder: `wit/src/`. Everything rewritten from scratch.

**Plan** (plan.md, 1196 lines):
- All 17 architecture predictions with full derivations
- τ,χ,μ,φ notation throughout
- Build phases mapped to predictions
- Evaluation metrics with pass/fail thresholds

**Ported (with .d→.x rename):**
wave.h, tokenize.h, distance.h, bonds.h, attention.h, morpheme.h

**New:**
- derive.h — pure function, no state. 64 primes, confidence levels,
  force classification, antimatter sign-flip, position cycle
- field.h — density (shell 1) + transitions (shell 1) + reverse index
  (shell 2, 625 bins) with context fragments, dark energy, log-counts,
  confidence weighting, hierarchical shells
- main.c — ingest + interactive chat with query understanding

### Two Review Rounds (10 reviewers total)

**Round 1 (6 reviewers):**
- Lattice consistency ✓
- Legacy leaks: coord4 field naming, primes.md notation
- Missing content: prime tables, morpheme rules, punctuation
- Architecture: 81 bins fatal → upgraded to 625
- μ-bias: proposed fixes validated by simulation
- Build order: Phase 0 gate, split Phase 1, evaluation metrics

**Round 2 (4 reviewers):**
- Killed the smuggled vocab table (word→bin dedup map)
- Confidence flows into field_observe
- 625 bins math: hybrid (density/transitions shell 1, reverse shell 2)
- Buildability: filled 8 blocking gaps

### Lattice Audit (5 components)

Found fundamental misalignments between code and lattice theory:

| Component | Finding | Severity |
|-----------|---------|----------|
| distance.h | L2 used everywhere, lattice says L-infinity (Chebyshev) | HIGH |
| distance.h | RC decay degenerate at clause_depth=0 | CRITICAL |
| distance.h | Capacitance = L2 norm, should be |μ| | HIGH |
| attention.h | Bond-typed attention completely absent | HIGH |
| bonds.h | Magnitude ignored in classify_bond | MEDIUM |
| bonds.h | Mask 12/13 naming wrong | MEDIUM |
| field.h | Dark energy backwards (empty bins = dead zones) | CONTRADICTION |
| field.h | Density arrays computed but never read | DEAD CODE |
| derive.h | No functional decomposition ("what does it DO?") | MAJOR |
| derive.h | Position cycle ad-hoc (not lattice-derived) | MAJOR |
| derive.h | Encoding leaks into payload | MODERATE |

All fixed. The fixes came from reading mind/mind and asking
"what does the lattice say this should be?"

### The Antimatter Fix
derive.h could not produce negative coordinates. The bond_default_regions
table was all-positive. The position cycle was all-positive. 51 of 81
bins were structurally unreachable.

Fix: antimatter principle. When a dimension CHANGES (bond active), the
new word takes the OPPOSITE sign. π̄ is the antimatter of π. After
positive → negative. After negative → positive.

Result: 21 bins → 38 bins. Entropy 3.62 → 4.66. Phase 0 gate PASSED.

### The Equation
Implemented f(a,b,c,d) = 2ᵃ·3ᵇ·5ᶜ·7ᵈ (Gödel numbering with prime
bases). Used for candidate ranking in generation. Float-precision
attention via float4 type + causal_sparse_attention_f().

### Bond-Typed Generation
Added force patterns to every word (DeriveResult.forces). During
generation, predict expected forces → score candidates by force
compatibility. The 16 bond patterns constrain which words can follow.

### Query Understanding
Force-typed response modes: short queries → RETRIEVE from field.
Long queries → GENERATE by walking the polynomial. No English string
matching — the coordinates determine the mode.

### Retrieval That Works
Context fragments stored per reverse index entry (64 chars of the
source sentence). Retrieval searches for the target word, ranks by
relevance (word position + frequency in context), deduplicates, returns
top 3. Pulls from Austen, Mises, Rothbard, Plato, Bible, Darwin,
Lakoff, Bitcoin Standard, Webster's dictionary.

### Polynomial Generation
Walk as running polynomial sum. Next word = residual (shift away from
centroid). Hylo self-referential refinement: generate draft, re-derive
each word with bidirectional context, replace mismatches, converge.
3 iterations max.


## Corpus

**Curriculum designed** (curriculum.md): 10 language domains + 7 lattice-
specific domains from schism corpus analysis.

**Data ingested** (10.8M words, 63MB):
- 16 synthetic domain files (24K words)
- 13 Gutenberg classics: Pride & Prejudice, Huckleberry Finn, Origin of
  Species, The Republic, Wealth of Nations, Journey to Center of Earth,
  Frankenstein, Art of War, Meditations, Problems of Philosophy, War of
  the Worlds, Alice in Wonderland, The Prince
- 3 Rothbard: Anatomy of State, What Has Govt Done, Ethics of Liberty
- 3 Mises: Human Action, Economic Calculation, (Socialism attempted)
- Hayek: Road to Serfdom, Use of Knowledge in Society
- Hoppe: Theory of Socialism and Capitalism
- Pirsig: Lila
- Lakoff: Metaphors We Live By
- Pinker: How the Mind Works
- Rand: Atlas Shrugged
- Ammous: Bitcoin Standard
- Webster's 1913 Unabridged Dictionary (4.5M words)
- Roget's Thesaurus
- King James Bible
- Tao Te Ching
- Joyce: Ulysses
- Clean Code, Expert C Programming

After pruning (min_count=3): 288K entries across 468/625 bins.


## Theory Documents

**integration.md** — How WIT integrates with nomos:
- The walker IS the wit (same mechanism, different data)
- ρ IS the polynomial (pipeline accumulation = attention)
- Bond dispatch IS query understanding (native, not if/else)
- Predictions 18-23 (6 new from walker-wit unification)
- 8 "attention is all you need" moments

**predictions-deep.md** — Deep chaining:
- 13 new predictions (18-30) from following every dependency
- Prediction 24: NLP tasks ARE bond patterns (the big one)
- Prediction 25: LSTM gates = ρ arithmetic
- Prediction 28: sentence boundaries = polynomial convergence
- All chains converge to 3 fixed points: ρ, ƒ, walk format
- 30 total predictions, 0 contradictions, 0 unused lattice properties


## Files Created

```
wit/src/
├── wave.h            ported (.d→.x)
├── tokenize.h        ported
├── distance.h        ported + audit-fixed (L∞, RC, C=|μ|)
├── bonds.h           ported + audit-fixed (magnitude, directed names)
├── attention.h       ported + audit-fixed (Chebyshev, float4 output)
├── morpheme.h        ported
├── derive.h          NEW (primes, morpheme, forces, antimatter, confidence)
├── field.h           NEW (density, transitions, reverse index, dark energy)
├── main.c            NEW (ingest, retrieval, generation, query understanding)
├── test_spread.c     NEW (Phase 0 gate — PASSES)
├── check_bins.c      NEW (bin distribution diagnostic)
├── build             build script
├── plan.md           build plan (1196 lines, 30 predictions)
├── integration.md    nomos integration theory
├── predictions-deep.md  deep prediction chains (18-30)
├── data/             curriculum + books + dictionary
│   ├── curriculum.md
│   ├── 16 domain .txt files
│   └── books/ (27 books, ~63MB)
└── wit.field         the ingested field (~100MB)
```


## Key Decisions

- No vocab table. Ever. Words live in coordinate bins. No word→anything map.
- derive() is a pure function. No state. Confidence prevents cascade.
- The field IS the model. Density + transitions + reverse index with context.
- Retrieval = coordinate lookup. Generation = walk extension.
- The equation f = 2ᵃ·3ᵇ·5ᶜ·7ᵈ is evaluated. Not just stored. Computed.
- Bond patterns determine response mode. Not English string matching.
- The walker IS the wit. Same mechanism. Different data.


## What's Next

The C prototype proves the architecture. The theory documents lay out
30 predictions. The integration design shows how it maps to nomos.

Immediate improvements:
- Longer context fragments (64→256 chars)
- Coordinate-based retrieval (use field_reverse_lookup, not linear scan)
- Higher prune threshold (min_count=5 or 10)
- Multiple contexts per word (not just first encounter)

Bigger:
- Walk format for language (literal .w files for sentences)
- Bond-classified control bytes in language walks
- ρ as 4D float (the polynomial state)
- Full walk extension generation (O(1) per token)

The prototype dissolves when the wit runs on nomos. The walker, the
equation, and the walk format are the irreducible primitives.
Everything else is scaffolding.
