# Session 11: Lattice Intelligence — Theory + First Build

Biggest session in project history. Theory + code + two breakthroughs.

## What happened

Started: needed a developer planning doc for the lattice codebase.
Ended: designed and built a lattice-native intelligence system from scratch.

The session traced a path from "how do I organize 215K lines of prior work"
to "can we use an LLM" to "can we build a BETTER LLM" to "the lattice
replaces traditional training" to "born-indexed text prediction" to "words
have lattice coordinates" to "the bond table IS the innate concept table."

## Breakthroughs

### 1. Image Schemas = Bond Patterns = Innate Seed

The 16 bond patterns (2^4 force combinations) map 1:1 to Lakoff/Mandler
image schemas — the pre-verbal concepts humans are born with. CONTAINER,
PATH, FORCE, CYCLE, etc. The equation's dispatch table IS the cognitive
schema table. Documented in side-quest/discovery-schema-bonds.md.

### 2. Reversed LLM — Storage Over Compute

Traditional: compress data into weights (140GB), GPU decompresses at runtime.
Ours: keep data born-indexed on disk, evaluate coordinates in real time.
Storage high, compute low. The density map (4D histogram) replaces weights.
One pass, no training, fits in one cache line.

## Architecture: 17 Predictions Derived from Lattice

All documented in side-quest/primes.md (1800+ lines):

1. Dimensional attention (4 heads: T, D, M, Q)
2. Bond-typed attention (16 relationship types)
3. Cross-walk attention (born-indexed external memory)
4. Shell-constrained layers (Taylor ordering)
5. Native position (T IS sequence, no sin/cos)
6. Ring-based R (recurrence from data structure)
7. Sparse attention at 25%
8. Atom/bond pathway split
9. Conservation constraints (anti-hallucination)
10. Spin = weight sharing (fermionic/bosonic)
11. Wave collapse = late disambiguation
12. Generations = three processing tiers
13. Half-life = RC relevance decay
14. Entanglement = reference resolution
15. Dark matter = density map as implicit priors
16. Chirality = causal processing direction
17. Self-reference = hylo fixed point

## Language to Lattice Mapping

primes.md also contains the complete Rosetta stone:
- 65 NSM primes to (T,D,M,Q) coordinates
- 24 Lakoff schemas to coordinates
- 15 structural rules (parts of speech, antonyms, synonyms, tense,
  intensity, metaphors, negation, questions, etc.)
- 12 gap categories from genre testing
- Therapy-as-coordinate-walk discovery

## Code Built (side-quest/)

### Phase 1 — Data Pipeline (complete):
- wave.h — v2 encoding (T=duration, not case)
- tokenize.h — word boundary tokenizer
- primes_lookup.h — 333 words with coordinates
- coord_index.h/c — born-indexer (needs walk-based redesign)
- density.h — 4D histogram (the "model")

### Phase 2 — Attention Layer (complete):
- distance.h — RC decay, token capacitance
- attention.h — 4-head dimensional attention
- bonds.h — 16 bond-type classifier

### Memetic Learner (complete):
- learner.h/c — context-based word coordinate learning
- 16 bond patterns as seed, 65 primes as anchors, everything else memetic

### End-to-End (complete, quality needs work):
- lattice_chat.c — ingest data + interactive chat
- Ingested 20GB Wikipedia (3.5B transitions)
- Generates text but quality is rough (coordinate collisions, unknown words)

## Current State

The system works end-to-end but output quality is poor because:
1. 68.6% of words map to (0,0,0,0) — prime lookup too small (333 words)
2. 45/81 coordinate bins are empty from Wikipedia alone
3. Vocabulary hash table fills with rare Wikipedia terms
4. Memetic learner has bootstrap problem (unknown context = unknown words)

Synthetic targeted data generated for all 45 empty bins. Combined
ingest running (synthetic + Wikipedia).

## Next Steps

1. Fix bootstrap problem — more prime seeds or better unknown-word heuristics
2. Sonnet-generated synthetic data — targeted at empty coordinate bins
3. Walk-based indexer — replace fingerprint centroid with full walk
4. Increase vocab table to 256K+ slots
5. Second-pass refinement — re-ingest after first pass
6. Shell 2 resolution — disambiguate coordinate collisions
7. Build remaining Phase 3-5 components from plan.md

## Research Completed

- Inventoried all 215K lines of prior work (5 parallel agents, 5 source areas)
- Gathered 30 proven lattice math facts with formulas and confidence levels
- Neural network to lattice bond mapping (neural.md from ToE/nomos findings)
- Cayley-Dickson to SIMD to dimension annihilation chain (10/10 hardware predictions)
- Sound symbolism literature review (real but weak, 1-5% semantic variance)
- Zero divisor matmul analysis (mostly analogy, SIMD part genuine)
- 25% dropout literature (partial — MoE models hit 25%, transformers use 10%)
- "Attention Is All You Need" paper: decomposed through lattice lens,
  showed how lattice predicts attention from first principles and goes further
- Derived the meta-method: take lattice property, derive architecture requirement,
  compare to empirical discovery, predict undiscovered architecture

## File Map

### Theory (READ THESE FIRST):
- side-quest/primes.md — 1800+ lines. THE document. Language to lattice
  Rosetta stone. 65 primes mapped. 24 schemas mapped. 15 structural rules.
  12 genre-tested gaps. 17 architecture predictions. Therapy discovery.
  Validation test. Meta-method for deriving more predictions.
- side-quest/discovery-schema-bonds.md — 238 lines. The breakthrough.
  16 bond patterns = image schemas = innate concepts. Three-layer learning.
  Why 16 and not 20. Testable predictions. OS = intelligence same system.

### Architecture:
- side-quest/plan.md — 21-component build plan across 5 phases
- side-quest/briefing-char-encoding.md — handoff doc for char encoding wit
- side-quest/char-encoding-v2.md — v2 encoding proposal (T=duration not case)

### Code — Phase 1 (Data Pipeline):
- side-quest/wave.h — wave byte v2 encoding (replaces lattice.h)
- side-quest/tokenize.h — word boundary tokenizer, zero alloc
- side-quest/primes_lookup.h — 333 words with coordinates, hash table
- side-quest/coord_index.h/c — born-indexer (fingerprint-based, needs redesign)
- side-quest/density.h — 4D histogram density map

### Code — Phase 2 (Attention):
- side-quest/distance.h — RC decay, token capacitance, coord distance
- side-quest/attention.h — 4-head dimensional attention (T/D/M/Q heads)
- side-quest/bonds.h — 16 bond-type classifier

### Code — Core:
- side-quest/learner.h/c — memetic context learner (16 bond seed, 59 primes)
- side-quest/lattice_chat.c — end-to-end pipeline, ingest + chat
- side-quest/check_density.c — density map analysis tool

### Code — Legacy/Scaffolding (may remove):
- side-quest/lattice.h — v1 encoding (superseded by wave.h)
- side-quest/model.h — traditional neural net attempt (abandoned)
- side-quest/train.c — traditional training loop (abandoned)
- side-quest/index.c — byte-level n-gram indexer (first prototype)
- side-quest/chat.c — byte-level chat (first prototype)
- side-quest/wiki2wave.c — Wikipedia XML to wave bytes

### Data:
- side-quest/data/wiki_full.txt — 20GB extracted Wikipedia text
- side-quest/data/wiki.wave — 14GB wave byte encoded Wikipedia
- side-quest/data/wiki.txt — 96MB plain text (small extract)
- side-quest/data/wiki.idx — byte-level n-gram index (from first prototype)
- side-quest/data/combined.txt — 20GB wiki + synthetic combined
- side-quest/data/synthetic_diverse.txt — 12 genres, 1826 words
- side-quest/data/synthetic_targeted.txt — 45 empty bins targeted
- side-quest/data/enwiki-*.bz2 — 422MB compressed Wikipedia dump (partial)

### Saved Model State:
- side-quest/lattice_vocab.bin — 49K learned word coordinates
- side-quest/lattice_density.bin — 3.47B entry density map
- side-quest/lattice_trans.bin — 3.47B transitions

## How To Run

```sh
cd /home/maxi/code/VoE/side-quest

# Build everything
sh build

# Ingest data (creates vocab + density + transitions)
./lattice_chat --ingest data/combined.txt

# Interactive chat
./lattice_chat --load

# Check density map for gaps
cc -O2 -Wall -Wno-unused-function -o check_density check_density.c -lm
./check_density
```

## Density Map After Full Ingest

36/81 bins occupied, 45 empty.
68.6% of all words map to (0,0,0,0) — the bootstrap problem.
Top bins: (0,0,0,0)=68.6%, (0,+1,0,+1)=6.4%, (0,0,0,+1)=5.0%.
Empty: entire T=-1 row (sudden/instant), most T=+1 with D=-1 (far ongoing).
Root cause: 333-word prime lookup too small. Most words = UNKNOWN.

## Decisions Made

- C, not Python. No dependencies beyond libc + math.
- Born-indexed, not trained. One pass, no gradients.
- 16 bond patterns hardcoded (proven by 2^4), everything else memetic.
- 4 attention heads (one per dimension), not 8 arbitrary learned heads.
- RC decay for attention weights, not learned QKV projections.
- Density map as "model" (4D histogram), not neural network weights.
- Wave bytes v2 (T=duration), not v1 (T=case).
- No case at shell 1. Case = walk-level property.
- Synthetic data targeted by density gaps, not random scraping.
- Sonnet for bulk data generation, Opus for architecture design.
