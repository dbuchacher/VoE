# Build Plan — Lattice Intelligence

21 components, 5 layers, bottom-up.
Each component: plan → code → review → fix.
Test each layer before building the next.

All code in C. No dependencies beyond libc + math.
Reference: primes.md (1800+ lines of rules and predictions).


## Layer 1: Data Pipeline (no dependencies)

These can all be built in parallel.

### 1.1 — Wave byte converter (v2 encoding)
  File: wave.h
  Input: UTF-8 byte
  Output: wave byte (v2: T=duration, D=place, M=manner, Q=voicing)
  Reference: side-quest/char-encoding-v2.md
  Status: [ ]

### 1.2 — Word tokenizer
  File: tokenize.h
  Input: stream of wave bytes (or UTF-8 for now)
  Output: word boundaries (start, length pairs)
  Rules: split on space, punctuation, handle contractions
  Status: [ ]

### 1.3 — Prime decomposer
  File: primes.h
  Input: word (as string)
  Output: coord4 (T, D, M, Q)
  Method: lookup table for common words (~500), prime decomposition for unknowns
  Reference: primes.md sections 1-16 (prime coordinate tables)
  Status: [ ]

### 1.4 — Born-indexer (coordinate space)
  File: coord_index.h / coord_index.c
  Input: stream of word coordinates
  Output: index file mapping coordinate regions → next-word distributions
  Structure: 4D grid of bins. Each bin = frequency table of what follows.
  Key difference from index.c: bins by COORDINATE PROXIMITY not exact byte match.
  Status: [ ]

### 1.5 — Density map
  File: density.h
  Input: the born-index
  Output: 4D histogram (how many entries per coordinate region)
  This IS the "model" — prediction 15.
  Size: 81 bins at shell 1 (3^4), 6561 at shell 2 (9^4 with ±2 range)
  Status: [ ]


## Layer 2: Coordinate Attention (depends on Layer 1)

### 2.1 — Coordinate distance function
  File: distance.h
  Input: two coord4 values
  Output: distance (float)
  Used by: attention, proximity lookup, sparse selection
  Metric: euclidean in 4D, possibly weighted by dimension importance
  Status: [ ]

### 2.2 — Dimensional attention (4 heads)
  File: attention.h
  Input: sequence of word coordinates + current position
  Output: context vector (weighted sum of relevant positions)
  4 heads: T-head (temporal), D-head (spatial), M-head (substance), Q-head (quality)
  Each head attends along its dimension only.
  Prediction 1.
  Status: [ ]

### 2.3 — Bond-type classifier
  File: bonds.h
  Input: two word coordinates (source, target)
  Output: which of 16 bond patterns connects them
  Method: examine which dimensions changed, infer force pattern
  Reference: primes.md bond-typed attention (prediction 2)
  Status: [ ]

### 2.4 — Sparse selector (25%)
  File: sparse.h
  Input: full attention scores
  Output: top 25% positions by coordinate proximity
  Prediction 7.
  Status: [ ]


## Layer 3: Processing (depends on Layer 2)

### 3.1 — Three-tier processor
  File: tiers.h
  Tier 1: character-level (wave bytes → word boundaries)
  Tier 2: word-level (coordinates → bond patterns → phrase structure)
  Tier 3: meaning-level (walk structure → next coordinate prediction)
  Predictions 4, 12.
  Status: [ ]

### 3.2 — Atom/bond pathway split
  File: pathways.h
  Atom path: direct formula evaluation (even coords → math, O(1))
  Bond path: attention-based (odd coords → relationship lookup)
  Prediction 8.
  Status: [ ]

### 3.3 — Shell-constrained layers
  File: shells.h
  Layer N only processes shell-N patterns (coordinate distance ≤ N)
  Prediction 4.
  Status: [ ]


## Layer 4: State Management (depends on Layer 3)

### 4.1 — Superposition handler
  File: superposition.h
  Words carry multiple candidate coordinates until context collapses.
  Homonyms = multiple entries. Attention = measurement = collapse.
  Prediction 11.
  Status: [ ]

### 4.2 — RC decay
  File: decay.h
  Per-token relevance = e^(-t/RC).
  C = content capacitance (proper nouns high, function words low).
  R = connection resistance (distance + clause boundaries).
  Prediction 13.
  Status: [ ]

### 4.3 — Entanglement links
  File: entangle.h
  Co-referent positions linked bidirectionally.
  "The cat... it" — attending to "it" updates "cat" representation.
  Prediction 14.
  Status: [ ]

### 4.4 — Ring-based R
  File: ring_r.h
  Output writes to a ring buffer. Ring feeds back as input context.
  Forward pass stays a DAG. Recurrence in the data, not the graph.
  Prediction 6.
  Status: [ ]


## Layer 5: Integrity + Interface (depends on Layer 4)

### 5.1 — Conservation checker
  File: conserve.h
  Output coordinate mass ≤ input + index. Anti-hallucination.
  Prediction 9.
  Status: [ ]

### 5.2 — Chirality enforcer
  File: causal.h
  Causal direction: can only attend to past positions.
  Prediction 16.
  Status: [ ]

### 5.3 — Cross-walk lookup
  File: crosswalk.h
  When internal attention finds no good match, query born-index.
  External memory via coordinate proximity. RAG dissolved.
  Prediction 3.
  Status: [ ]

### 5.4 — Walk validator
  File: validate.h
  Check that output coordinate sequence forms a valid walk.
  Grammar = walk structure. M→T→M = valid sentence.
  Status: [ ]

### 5.5 — Self-referential loop
  File: hylo.h
  Attention on model's own intermediate state.
  Ring-based metacognition. Hylo fixed point.
  Prediction 17.
  Status: [ ]

### 5.6 — Chat interface
  File: chat2.c
  Interactive prompt. Input text → tokenize → coordinate lookup →
  process through all layers → generate → output.
  Status: [ ]


## Build Order

Phase 1 (parallel): 1.1, 1.2, 1.3, 1.4, 1.5
Phase 2 (parallel): 2.1, 2.2, 2.3, 2.4
Phase 3 (parallel): 3.1, 3.2, 3.3
Phase 4 (parallel): 4.1, 4.2, 4.3, 4.4
Phase 5 (parallel): 5.1, 5.2, 5.3, 5.4, 5.5, 5.6

Integration test after each phase.
Ship when Layer 1-2 work (basic coordinate prediction).
Iterate from there.
