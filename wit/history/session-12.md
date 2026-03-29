# Session 12: Kill the Dictionary

The lookup table was legacy thinking. Killed it.

## What Happened

Session 11 built the lattice intelligence end-to-end but 68.6% of words
mapped to (0,0,0,0) because primes_lookup.h only had 333 words. The
TODO said "expand to 2000 words" — but the discovery doc from the same
session said "16 bond patterns are the seed, no dictionary."

The TODO contradicted the discovery. The dictionary was the problem,
not the solution. This session resolved it.

## What Changed

### New: morpheme.h
Suffix/prefix decomposer. 30+ suffix rules, 20+ prefix rules.
Strips affixes, returns root + coordinate transform.
- `-ing` → +T,+M (temporal ongoing)
- `-ed` → +T (past)
- `-ly` → +Q (quality modifier)
- `un-` → flip Q sign
- `anti-` → flip all signs
- etc.

Turns any derived word into root + transform. "unhappy" = "happy" + Q flip.
"running" = root "run" + T+M delta. "beautifully" = root "beauti" + Q.

### Changed: learner.h/c
- Added morpheme.h integration: `try_morpheme()` called before context fallback
- Added `char_level_prior()`: wave byte average as last-resort nonzero fallback
- Added `learner_reverse()`: coordinate → nearest words for generation
- Priority chain for unknowns: morpheme → context+bond+position → char-level
- Result: ZERO words ever map to (0,0,0,0)

### Rewritten: lattice_chat.c
- Removed `#include "primes_lookup.h"` — the dictionary is dead
- Removed inlined Learner (was duplicate of learner.h/c with different format)
- Uses real learner.h/c as PRIMARY coordinate source
- Generation reverse-looks-up through learner vocabulary, not prime table
- Ingest uses learner_process_word (morpheme + bond + context + char-level)
- File format: now "LRNR" (from learner.h/c), not "VLRN" (old inlined)
- Old .bin files incompatible — must re-ingest

### Updated: build script
- Only builds lattice_chat (legacy targets removed)
- Links learner.c explicitly

## What Was Removed

primes_lookup.h is no longer included anywhere in the pipeline.
Its 333 hardcoded words are gone. The 65 NSM primes survive as
anchors seeded into the learner at init (learner_seed_primes).

Legacy files still exist but build script no longer compiles them:
lattice.h, model.h, train.c, index.c, chat.c, wiki2wave.c

## Test Results

- Self-test: PASS (5 varied responses from primes-only mode)
- Morpheme: "happy" → (+1,0,+1,+1), "unhappy" → (+1,0,+1,-1) — Q flip works
- Context: "the big running cat" → running gets T-dominant, cat gets M-dominant
- Ingest: 5 sentences → 10/81 bins occupied, 0% at (0,0,0,0)
- Round-trip: ingest → save → load → chat — vocabulary persists

## Architecture After This Session

```
Input → tokenize → learner_process_word:
  1. Known word? Return stored coord
  2. Morpheme: strip affixes, root in vocab? Apply transform
  3. Context + bond pattern + position bias → estimate
  4. Char-level wave byte average → weak nonzero prior
  → Born-index immediately, refine on re-encounter

Generation → predict_next:
  1. Causal sparse attention → context vector
  2. Transition map → what follows this coordinate?
  3. Density map → what's common near here?
  4. Bond-guided dimension cycling → sentence coherence
  5. learner_reverse → coordinate → nearest known word
```

No dictionary. No lookup table. The equation provides the seed (16 bonds).
The primes provide anchors (65 words). Everything else is memetic.

## Next Steps

1. Re-ingest Wikipedia with the new pipeline (expect dramatically better coverage)
2. Check density distribution after full ingest — should be much more uniform
3. vocab table size: still 64K slots, may need increase for 20GB
4. coord_index redesign: still centroid-based (bag-of-words), needs walk-based
5. Remaining build phases 3-5 from plan.md
