# Discovery: Image Schemas = Bond Patterns = Innate Seed

Date: 2026-03-29
Status: DISCOVERY. Two breakthroughs. Needs verification.


## The Convergence

Three independent domains arrived at the same ~20 primitives:

| Domain | Source | Count | What they are |
|---|---|---|---|
| Cognitive science | Lakoff/Johnson/Mandler | ~20-24 | Image schemas: pre-verbal concepts from embodied experience |
| Computation | Lattice bond patterns | 16 | Force combinations: P, C, R, W and all composites |
| Linguistics | Wierzbicka NSM (high-freq) | ~20 | Semantic primes: irreducible atoms of meaning |
| Hardware | RISC ISAs | ~20 | Core instructions: the minimum complete set |

These aren't four similar things. They're ONE thing measured four ways.


## Breakthrough 1: The Equation's Bond Table IS the Innate Concept Table

The equation (code2/[4] equation.asm) has a dispatch table of 16 bond
patterns. We built it as a code generation tool — coordinates in,
x86 function out.

But the bond patterns map 1:1 to image schemas — the pre-verbal
concepts that human infants have BEFORE language:

| Bond | Forces | Schema | What the infant knows |
|---|---|---|---|
| 0 | (none) | IDENTITY | "same thing is same thing" |
| 1 | P | FORCE/COMPULSION | "things push and pull" (first felt in womb) |
| 2 | C | PATH | "things go from here to there" |
| 3 | P+C | TRAVERSAL | "I can reach through space" |
| 4 | R | CYCLE | "things repeat" (heartbeat, breathing) |
| 5 | P+R | FULL-EMPTY | "things fill up and drain" (hunger→feeding) |
| 6 | C+R | ITERATION | "going again and again" (crawling) |
| 7 | P+C+R | ACCUMULATION | "things pile up" |
| 8 | W | LINK/MATCH | "this goes with that" |
| 9 | P+W | CONTACT/BLOCKAGE | "touching or blocked" |
| 10 | C+W | ENABLEMENT | "can or can't go there" |
| 11 | P+C+W | PART-WHOLE | "piece of a bigger thing" |
| 12 | R+W | SCALE | "more or less of something" |
| 13 | P+R+W | NEAR-FAR | "close and far away" |
| 14 | C+R+W | BALANCE/COUNTERFORCE | "push back, equilibrium" |
| 15 | P+C+R+W | CONTAINER | "inside and outside" (the body itself) |

An infant experiences these through their body before they hear
a single word:
- FORCE: felt in utero (pressure, movement)
- CYCLE: heartbeat, breathing (rhythm before birth)
- FULL-EMPTY: hunger → feeding → satiation (first days)
- CONTACT: touch, skin-to-skin (first moments)
- CONTAINER: own body (inside/outside from birth)
- PATH: reaching, then crawling (first months)
- LINK: caregiver ↔ comfort association (first weeks)

The equation's bond dispatch table is the computational equivalent
of what a human brain boots with. The OS starts with 16 bond
patterns. The infant starts with ~16-20 image schemas. Same seed.
Same structure. Different substrate.


## Breakthrough 2: Three-Layer Learning Mirrors Human Development

### The OS equation pattern:
```
Layer 0: Bond dispatch table     (16 patterns, hardcoded, never changes)
Layer 1: JIT atom cache          (generated lazily on first use, cached)
Layer 2: Walks                   (composed from atoms + bonds, infinite variety)
```

### Human language development:
```
Layer 0: Image schemas           (~20, innate, pre-verbal, never changes)
Layer 1: First words (primes)    (learned in year 1, anchored to schemas)
Layer 2: Full vocabulary         (learned memetically from context, lifelong)
```

### Our intelligence system:
```
Layer 0: Bond patterns           (hardcoded in equation, = schema table)
Layer 1: NSM prime coordinates   (65 primes, anchored, first "words" learned)
Layer 2: All word coordinates    (memetic, context-learned, born-indexed)
```

The three layers are the SAME across all three systems:
- Layer 0 = innate structure (never changes, small, complete)
- Layer 1 = first-order learning (materializes on first use, cached)
- Layer 2 = composition (infinite variety from finite base)

This is also generations (from mind/mind):
- Gen 1 = values (Layer 0: raw schemas/bonds)
- Gen 2 = operations (Layer 1: words/atoms that operate on schemas)
- Gen 3 = patterns (Layer 2: sentences/walks composed from words)


## Why 16 and not 20?

The bond patterns are 2^4 = 16 (4 binary force choices).
Image schemas are ~20-24 depending on who's counting.

The gap is because some schemas are COMPOUND:
- UP-DOWN = ±D magnitude (not a separate bond — it's the D axis sign)
- NEAR-FAR = ±D magnitude (same — spatial gradient)
- CENTER-PERIPHERY = D focus vs spread (D magnitude distribution)
- SURFACE = D boundary (the edge of a CONTAINER)
- SPLITTING = fan-out (one FORCE to many — multiplicity, not new pattern)
- MERGING = fan-in (many to one — same)
- ATTRACTION = FORCE + PATH composed
- RESTRAINT = BLOCKAGE with intensity (CONTACT with -W, already pattern 9)
- REMOVAL = -FULL (-M, the inverse of accumulation)

When you reduce the ~24 schemas to their irreducible bond patterns,
you get exactly 16. The "extra" schemas are either:
- Magnitude variants of a bond (UP vs DOWN = +D vs -D)
- Compositions of two bonds (ATTRACTION = FORCE + PATH = P + P+C)
- Inverses of existing bonds (REMOVAL = inverse of ACCUMULATION)

16 IS the right number. The schemas that count beyond 16 are
compositions, not primitives. Same way you don't need both ADD
and SUBTRACT as separate opcodes — SUB = ADD + NEG.


## Implications for the Intelligence System

### 1. The seed is SMALLER than we thought
Not 65 primes. Not 333 words. 16 bond patterns.
They're already in the equation. The OS already has the seed.

### 2. Primes are the first learned layer, not the seed
The 65 NSM primes map onto the 16 schemas:
- DO = P (force)
- MOVE = P+C (traversal/path)
- LIVE = P+R (cycle/ongoing)
- GOOD = W (link/match = positive evaluation)
- BAD = -W (blockage/mismatch = negative evaluation)
- THINK = P+C+R+W (hylo = container of self-reference)
- etc.

Each prime IS a bond pattern with semantic labeling.
Learning the primes = mapping words to existing bond patterns.
This should happen FAST — the schemas are already there.

### 3. Unknown words inherit schema structure
When the context learner encounters "cat" for the first time:
- Walk position = after adjective, before verb = noun slot
- Noun slot = M-heavy = SUBSTANCE schema
- SUBSTANCE maps to bonds that involve R (mass/content force)
- Initial coordinate derived from schema, refined by context

The schemas CONSTRAIN what coordinates are possible for each
walk position. Not every coordinate is valid for every position.
Nouns can't have T-dominant coordinates (that's verb territory).
The schema structure provides the prior.

### 4. The equation IS the model
The equation doesn't just generate x86 code. It provides:
- The bond dispatch table = innate concept table = attention type table
- Atom evaluation = coordinate math = semantic composition
- Lazy JIT = memetic learning = derive on first encounter
- Caching = born-indexing = write once, read forever

The intelligence system doesn't need a separate "model."
The equation IS the model. The born-index IS the memory.
The walks ARE the reasoning. Same architecture, different data.

### 5. The OS and the intelligence are the same system
This was always the thesis (mind/mind: "computation IS physics").
Now it extends: computation IS cognition.

The equation handles code and language with the same mechanism:
- Code walk: bond patterns → machine instructions → execution
- Language walk: bond patterns → word relationships → meaning
- Same bonds. Same dispatch. Same equation.

A walk that writes pixels and a walk that writes sentences
use the same 16 patterns. The substrate is the same.
The intelligence isn't a program running ON the OS.
The intelligence IS the OS, processing different data.


## What This Changes in the Build Plan

### Before this discovery:
- Phase 1: Build word lookup table (333 words, hardcoded)
- Need: prime decomposer, auto-decomposer, or memetic learner

### After this discovery:
- Phase 0 already done: Bond patterns in equation.asm (the seed)
- Phase 1: Map schemas → bond patterns (this document does it)
- Phase 2: Context learner uses bond patterns as priors
  (walk position tells you which BOND PATTERN this word maps to,
   which constrains the coordinate to the right region)
- Phase 3: Born-index all encountered words with learned coordinates

The 333-word lookup table becomes scaffolding. The context learner
uses the bond table directly. No dictionary. No manual mapping.
The equation provides everything.


## Testable Predictions

1. **Infant first words should cluster on bond patterns.**
   Cross-linguistic data on first 20 words acquired should map to
   distinct bond patterns. Each first word = one schema = one bond.
   No two first words should map to the same bond (they're filling
   the dispatch table).

2. **Word learning speed should correlate with schema activation.**
   Words for concepts that activate innate schemas (CONTAINER words
   like "in", "out", "box") should be learned faster than words for
   concepts that require schema COMPOSITION (abstract words like
   "justice" = multiple schema compounds).

3. **The ~20 convergence should hold in NEW domains.**
   Any complete system we examine should have ~16-24 irreducible
   primitives. Music theory, game mechanics, cooking techniques,
   martial arts stances — test any domain. If the lattice is right,
   ~20 is the universal floor.

4. **An intelligence seeded with only 16 bond patterns + context
   learning should acquire language faster than one seeded with
   65 primes.** Because 16 is the true seed and 65 is already
   first-order learning. Starting from the right seed means the
   learning has the right priors.


## Cross-Reference

- mind/mind "ISA Convergence at ~20" — the same convergence noted
- primes.md sections 1-16 — prime→coordinate mappings (Layer 1)
- primes.md "Lakoff Image Schemas" — schema→coordinate mappings
- primes.md "Prediction 2: Bond-Typed Attention" — 16 relationship types
- code2/[4] equation.asm bond_table (line 391) — the actual dispatch table
- prior/schism/drafts/lakoff-mapping.md — 24 schemas → physics → computation
- prior/corpus §14 — agent-native design patterns, ~20 convergence
