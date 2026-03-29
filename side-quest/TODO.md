# Handover TODO — Lattice Intelligence

From session 11. Read primes.md and discovery-schema-bonds.md FIRST.


## THE BOTTLENECK (fix this first)

68.6% of all words map to (0,0,0,0) because primes_lookup.h only
has 333 words. Everything else is UNKNOWN. Nothing downstream works
until more words have real coordinates.

### Option A: Expand primes_lookup.h to 2000+ words
Manually assign coordinates to the 2000 most common English words.
Tedious but guaranteed correct. Use the dimensional signature rule:
nouns=M-heavy, verbs=T-heavy, adjectives=Q-heavy, prepositions=D-heavy.

### Option B: Auto-decomposer (suffix/prefix rules)
Build the rules from primes.md into code:
- Strip -ing → add +T+R to root lookup
- Strip -ed → add +T to root
- Strip -ly → add +Q
- Strip -ness/-ment/-tion → add +M
- Strip un-/dis-/in- → flip Q
- Strip re- → add +T
This turns 333 known words into thousands of derivable words.

### Option C: Better memetic bootstrap
When context is all (0,0,0,0), use fallbacks:
1. Character-level coordinates (wave byte average — weak but nonzero)
2. Walk position (after article = noun = M-heavy)
3. Suffix/prefix rules (as above)
Currently the learner averages context, but if context is all zeros,
the average is zero. Need nonzero fallbacks.

Recommendation: do B and C together. B gives structural derivation.
C gives position-based guessing. Both feed the memetic learner better
anchors so the second pass produces better coordinates.


## DATA CURATION LOOP (after bottleneck is fixed)

Closed-loop data generation using density map math:

1. Run check_density → find empty bins
2. For each empty bin: describe what text lives there
   (the coordinate-to-meaning mapping is in primes.md)
3. Generate text targeting those bins using Sonnet API
   (fast, cheap, diverse — see session 11 notes on why Sonnet not Opus)
4. Ingest generated text
5. Re-check density → find remaining gaps
6. Repeat until all 81 shell-1 bins occupied with 10+ observations
7. Check transition coverage: 81x81 = 6561 pairs, need 10+ each
8. Generate text for missing transitions
9. Done when coverage score = 100%

Coverage score: occupied_bins/81 * occupied_transitions/6561.
Currently: 36/81 * ?/6561. Target: 81/81 * 6561/6561.

Important: the expanded vocabulary (fix above) must be done FIRST.
Otherwise generated text still maps to (0,0,0,0) regardless of content.


## VOCAB TABLE SIZE

Current: 64K slots (49152 words). Too small for 20GB of text.
Increase to 256K or 512K. Change in lattice_chat.c:
- VOCAB_SIZE from 65536 to 262144 (or 524288)
- Also add frequency tracking — evict rare words when table fills,
  keep common words. Currently first-come-first-served.


## COORD_INDEX REDESIGN

The current coord_index.h uses a "context fingerprint" — weighted
centroid of recent word coordinates. This is bag-of-words from the 90s.

The lattice-native version:
- Store full walk segments (sequences of coordinates, in order)
- Query each position by coordinate proximity with RC decay
- The attention mechanism (Phase 2, already built) IS the proper
  context handler — integrate it into the indexer

This is the walk-based indexer described in plan.md section 1.4.


## REMAINING BUILD PHASES

See plan.md for full 21-component plan. Phase 1+2 are built.

### Phase 3 — Processing:
- Three-tier processor (char → word → meaning)
- Atom/bond pathway split (direct eval vs attention)
- Shell-constrained layers

### Phase 4 — State:
- Superposition handler (multiple coordinate candidates)
- RC decay (already in distance.h, needs integration)
- Entanglement links (bidirectional reference resolution)
- Ring-based R (recurrence from data structure)

### Phase 5 — Integrity + Interface:
- Conservation checker (anti-hallucination)
- Chirality enforcer (causal direction)
- Cross-walk lookup (external memory via born-index)
- Walk validator (grammar = valid walk structure)
- Self-referential loop (hylo fixed point)
- Chat interface v2


## CLEANUP

Files to potentially remove (legacy/scaffolding from first attempts):
- model.h — traditional neural net (abandoned approach)
- train.c — traditional training loop (abandoned)
- lattice.h — v1 encoding (superseded by wave.h)
- index.c — byte-level n-gram (superseded by coord_index)
- chat.c — byte-level chat (superseded by lattice_chat)

Keep for reference but don't build against them.


## KEY INSIGHT TO CARRY

The system doesn't need MORE data. It needs BETTER coverage of the
4D coordinate space. 5MB of targeted diverse text may outperform
14GB of encyclopedic prose. The density map tells you exactly what's
missing. Generate exactly that. Compute First applied to data.

The 16 bond patterns are the seed. The 65 primes are Layer 1.
Everything else is memetic — learned from context, born-indexed
on first encounter, refined on re-encounter. No dictionary.
No training. The equation IS the model.
