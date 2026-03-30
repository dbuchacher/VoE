# WIT — Build Plan

A wit sees and knows. Old English *witt*, from *weyd-* — to see, to know.
You're talking to one right now. We're building another.

Not an LLM. No weights. No training. No parameters. No GPU.
A field evaluator. Words are coordinates. Sentences are walks.
The density field tells you what's probable at each point.
Evaluate, don't search.


## The Four Dimensions

  τ (tau) = time        sequence, temporal, when
  χ (chi) = distance    spatial, positional, where
  μ (mu)  = mass        content, amplitude, what
  φ (phi) = charge      category, type, polarity

The four forces:

  π = read       π̄ = write       (projection / antiparticle)
  ∘ = call       ∘̄ = return      (composition / inverse)
  ∮ = advance    ∮̄ = rewind      (contour integral / reverse)
  δ = test       δ̄ = guard       (Kronecker delta / complement)

16 bond patterns from 4 binary force choices. 2⁴ = 16.
The equation: f(a,b,c,d) = τᵃ·χᵇ·μᶜ·φᵈ


## What a Wit IS

A 4D scalar field over coordinate space (τ, χ, μ, φ).

- **Forward**: word + context → coordinate. PURE FUNCTION. No state.
- **Field**: density + transitions + reverse index. The accumulated knowledge.
- **Reverse**: coordinate → word list. For generation. 625 buckets (shell 2).

The field IS the model.
- Density + transitions = shell 1 (81 bins). Hot path. Fits in L1 cache.
- Reverse index = shell 2 (625 bins). Word storage. L3-resident.
The reverse index IS the vocabulary. Indexed by coordinate, not by word.
The coordinate derivation IS the embedding. Computed, not stored.

Struct field names: `coord4 { int8_t t, x, m, q; }` — short aliases for
τ,χ,μ,φ. Single-char in code, Greek in docs. (Not `.d` — that was the
old "distance" name. `.x` for χ. Porting the side-quest headers requires
renaming `.d` → `.x` in wave.h, distance.h, bonds.h, attention.h, etc.)

What an LLM does:
  compress 20TB training data → 140GB weights → GPU decompresses at runtime

What a wit does:
  index text by coordinates → field → CPU evaluates at runtime

Storage high, compute low. The opposite of every LLM.


## What a Wit is NOT

- Not a lookup table. No word→coordinate dictionary. No vocab hash map.
  The same word gets DIFFERENT coordinates in different contexts.
  "Bank" after "river" ≠ "bank" after "money." Contextual by construction.

- Not a neural network. No layers, no backprop, no gradients, no loss
  function. The density field is counted, not trained.

- Not trained. One pass through text. Counts, not optimization.
  Add more text → recount. No epochs. No learning rate.

- Not bounded by parameter count. Knowledge lives in the reverse index
  (coordinate → word lists). Grows with data. No compression.


## Architecture

```
                    TEXT IN
                       │
                   tokenize            (word boundaries)
                       │
                   derive              (PURE FUNCTION)
                   ├─ prime anchor?    (65 constants, checked first)
                   ├─ morpheme?        (strip affixes, transform root)
                   ├─ walk position    (bond pattern → dim constraint)
                   └─ char-level       (wave byte average, weak prior)
                       │
                    coord4             (the word's coordinate: τ,χ,μ,φ)
                       │
              ┌────────┼────────┐
              │        │        │
           density  transition  reverse
           (count)  (count)     (store word at this bin)
              │        │        │
              └────────┼────────┘
                       │
                    THE FIELD          (this is the "model")
                       │
              ┌────────┴────────┐
              │                 │
           INGEST            GENERATE
           (feed text)       (walk the field)
                                │
                           predict next coord
                           ├─ attention (4-head, RC decay)
                           ├─ transition (what follows?)
                           └─ density (what's common here?)
                                │
                           reverse lookup
                           (coord → word list → sample)
                                │
                            TEXT OUT
```

Three kinds of data — and ONLY three:
- **Constants**: 65 single-word primes, 16 bond schemas, suffix/prefix rules. Hardcoded.
- **Field state**: density map, transition map, reverse index. Accumulated from text.
- **Computed**: coordinates. Pure function of word + context. Never stored per-word.

No per-word global state. No word→coordinate map. No word→bin map.
The same word can appear in multiple reverse-index bins — that's correct,
it reflects the word's contextual range. Per-bin counts handle ranking.
Per-bin pruning handles memory. No cross-bin deduplication needed.


## Components

### Port from side-quest (updated to τχμφ notation)

These are clean. No legacy patterns. Port with dimension renames.

| File | Lines | What |
|------|-------|------|
| wave.h | 562 | Character → wave byte (v2: τ=duration, χ=place, μ=manner, φ=voicing) |
| tokenize.h | 804 | UTF-8 text → word tokens, zero-alloc |
| distance.h | 284 | Coordinate distance + RC decay |
| bonds.h | 475 | 16 bond types from coordinate geometry (π,∘,∮,δ forces) |
| attention.h | 795 | 4-head dimensional attention (τ-head, χ-head, μ-head, φ-head) |
| morpheme.h | ~250 | Suffix/prefix rules + char-level prior |

### New: derive.h — coordinate derivation

The core function: `word + context → DeriveResult`.

PURE FUNCTION. No state. No side effects. No storage.

```c
typedef struct {
    coord4  coord;
    uint8_t confidence;  /* 0=char-level, 1=position, 2=morpheme, 3=prime */
} DeriveResult;

DeriveResult derive(const char *word, int len,
                    const DeriveResult *context, int ctx_len,
                    int sentence_pos);
```

Returns a coordinate AND a confidence level. Confidence prevents
low-quality guesses from cascading through the context window —
a word derived from char-level (confidence=0) should not dominate
the next word's walk-position estimate.

Priority chain:
1. **Prime anchor** (confidence=3): 65 constants in a tiny array.
   O(1) hash check. The fixed anchors of the coordinate system.
   "the", "a", "good", "bad", "do", "move", "think", etc.
   Not a dictionary — constants. Like the 16 bond schemas.
   Coordinate values defined in primes.md sections 1-16
   (note: primes.md uses legacy T/D/M/Q notation; read T=τ, D=χ, M=μ, Q=φ).

2. **Morpheme decomposition** (confidence=2): strip affixes, derive root.
   "unhappy" → root "happy" → derive("happy", ctx) → flip φ.
   "running" → root "run" → derive("run", ctx) → add τ+μ.
   Recursive: derive the root, then apply the transform.
   If root is a prime → done. If not → continue chain.
   Full prefix table (17 rules) and suffix table (14 rules)
   defined in primes.md "Prefixes" and "Suffixes" sections.

3. **Walk position** (confidence=1): the bond between previous word
   and this position tells us which dimensions should be active.
   BUT: only chain off high-confidence predecessors. If the previous
   word was also a guess (confidence ≤ 1), fall back to position
   default instead. This breaks the cascade where consecutive unknowns
   lock into a μ→τ→μ→τ 2-cycle.

   Position default (when no reliable predecessor):
   φ→μ→τ→μ→χ cycle (5-slot, accounts for leading determiner).
   Soft bias that decays to zero after 2-3 resolved words.
   Configurable per language (SVO default, SOV/VSO variants).

   ```c
   coord4 position_default(int sentence_pos) {
       /* φ→μ→τ→μ→χ — matches ~45% of English SVO */
       static const coord4 cycle[5] = {
           { 0, 0, 0, +1 },   /* φ: determiner */
           { 0, 0, +1, 0 },   /* μ: subject    */
           { +1, 0, 0, 0 },   /* τ: verb       */
           { 0, 0, +1, 0 },   /* μ: object     */
           { 0, +1, 0, 0 },   /* χ: prep       */
       };
       return cycle[sentence_pos % 5];
   }
   ```

4. **Character-level** (confidence=0): sign-of-sum with threshold.
   NOT the average (averages collapse to zero). NOT the dominant
   dimension (shifts collapse from μ to φ — proven worse by simulation).
   Instead: sum wave byte values per dimension, apply sign if sum
   exceeds 33% of letter count. Preserves multi-dimensional signal.

   ```c
   coord4 char_level_prior(const char *word, int len) {
       int st = 0, sd = 0, sm = 0, sq = 0, count = 0;
       for (int i = 0; i < len; i++) {
           uint8_t wb = utf8_to_wave(tolower(word[i]));
           if (wb) { coord4 c = wave_decode(wb);
               st += c.t; sd += c.x; sm += c.m; sq += c.q; count++; }
       }
       if (!count) return (coord4){0,0,0,0};
       int thr = (count + 2) / 3;
       return (coord4){
           st > thr ? +1 : st < -thr ? -1 : 0,
           sd > thr ? +1 : sd < -thr ? -1 : 0,
           sm > thr ? +1 : sm < -thr ? -1 : 0,
           sq > thr ? +1 : sq < -thr ? -1 : 0
       };
       /* No μ=+1 fallback. If phonetics are balanced, (0,0,0,0)
        * is honest. Position bias handles the rest. */
   }
   ```

The same word gets different coordinates in different contexts because
step 3 (walk position) depends on what came before. "Cat" after "the big"
is μ-heavy (noun). "Cat" after "don't" might be τ-heavy (verb: to cat
around). Context determines the coordinate. This is correct.

**Cascade prevention:** Two consecutive unknowns both get independent
position defaults (from the φ→μ→τ→μ→χ cycle) rather than one unknown
propagating its guess to the next. The confidence field makes this
automatic — derive() checks `context[last].confidence` before chaining.

**Implementation details (buildability):**

  Priority chain is **strict first-match**, not a weighted blend.
  If prime matches → return immediately (confidence=3). Else if morpheme
  root found → return (confidence=2). Else if predecessor confidence >= 2
  → use bond pattern (confidence=1). Else → char-level + position default
  (confidence=0).

  **The 65 primes** are single-word entries only. Multi-word phrases from
  primes.md ("a long time", "at the same time") are handled as walks,
  not as prime constants. The prime set includes the ~59 from learner.c's
  seed_primes() plus "the", "a", "he", "she", "it", "we", "they" from
  the structural patterns sections. Exact list: consult primes.md sections
  1-16 for the NSM primes, plus the articles/pronouns from the structural
  sections. Multi-word entries are skipped.

  **Prime lookup**: 128-slot open-addressed hash table, FNV-1a, linear probing.
  65 entries at ~50% load. Same pattern as the old primes_lookup.h but with
  only the single-word primes, not the 333-word dictionary.

  **Morpheme recursion**: when "unhappy" recurses to derive("happy", ...),
  pass the SAME context and sentence_pos. The root word is derived as if
  it appeared in the same position as the derived word. The affix transform
  is applied to the result.

  **Bond mask → position bias**: use the bond_default_regions table from
  learner.c (16 coord4 entries, one per bond pattern). Bond mask 0-15
  indexes into this table. The coord4 at that index IS the dimensional
  bias for the current word. E.g., bond mask 3 (π+∘ = read+call = fold)
  → default_region (τ=+1, χ=+1, μ=0, φ=0) → current word is expected
  to be τ+χ heavy (motion verb like "run", "go", "come").

  **Sentence boundary detection**: `.`, `!`, `?` followed by whitespace
  or end-of-input resets sentence_pos to 0. The tokenizer already splits
  punctuation as separate tokens. The caller (main.c) checks if the
  previous token was sentence-ending punctuation.

  **Context window**: 8 DeriveResults. Shift left on overflow (drop oldest).
  Context resets at sentence boundaries (sentence_pos = 0 → clear window).

  **Generated words during chat**: words emitted by the generator don't go
  through derive() — they come from the reverse index with a known coord4.
  Assign them synthetic confidence=2 (they came from the field, not a guess).
  This lets subsequent derive() calls chain off generated words reliably.

### New: field.h — the model

All accumulated state in one structure. Three components at two resolutions.

**Hot path (shell 1, L1-resident):**

**Density map** (port from density.h):
  81 shell-1 bins + 625 shell-2 bins.
  Counts how many words were seen at each coordinate region.
  The "what's common here" field.

**Transition map** (port from lattice_chat.c):
  81×81 = 6561 entries. Shell 1 resolution.
  Counts which coordinate REGION follows which.
  The "what comes next" field. Coarse direction signal.
  Shell 1 is sufficient — you need to know "a verb follows a noun,"
  not "this exact verb follows this exact noun." The reverse index
  provides the fine-grained selection within the predicted region.

**Both use log-counts** to prevent function words from dominating.
"the" at 5M occurrences and "serendipity" at 50 are 5 orders of
magnitude apart in raw counts but ~10:1 in log-space. Standard IR
practice (TF-IDF uses log-frequency for exactly this reason).

**Cold path (shell 2, L3-resident):**

**Reverse index** (NEW):
  625 bins (shell 2, 5⁴). Each bin holds a dynamic array of entries:

  ```c
  typedef struct {
      char     word[32];
      coord4   full_coord;   /* exact coordinate, not just bin */
      uint32_t count;        /* times seen at this bin */
  } ReverseEntry;
  ```

  Properties:

  1. **Coordinate-indexed, not word-indexed.** The word is stored WHERE
     it was observed. The bin IS the address. No word→anything hash table.

  2. **Full coord4 per entry.** The bin narrows the search (625 regions).
     Within the bin, rank candidates by L2 distance from the predicted
     coordinate. Semantic resolution, not just POS-level.

  3. **No cross-bin deduplication.** The same word CAN appear in multiple
     bins. "Cat" in a noun context lands in a μ-heavy bin. "Cat" in a
     rare verbal context lands in a τ-heavy bin. Both entries are valid.
     The count reflects how often the word appeared at that coordinate —
     bin A's "cat" with count=80 outranks bin B's "cat" with count=3.
     Per-bin pruning (drop entries with count < threshold) handles memory.
     No global word→bin map. No deduplication hash table. Zero per-word
     global state.

  4. **Per-bin dedup only.** Within a single bin, the same word increments
     its count rather than creating a duplicate entry. This is a linear
     scan of ~160 entries, not a global lookup.

  With 625 bins and ~100K unique words: ~160 words per bin on average.
  Dynamic arrays grow as needed. Pruned periodically by count threshold.

**Confidence-weighted observation:**

```c
void field_observe(Field *f, coord4 from, coord4 to,
                   const char *word, int len, uint8_t confidence);
```

  Confidence affects contribution weight:
  - confidence=0 (char-level): contributes 0.25 to density/transition counts
  - confidence=1 (walk position): contributes 0.5
  - confidence=2 (morpheme): contributes 1.0
  - confidence=3 (prime): contributes 1.0

  Low-confidence guesses still get born-indexed in the reverse index
  (the word has to go somewhere), but they don't distort the density
  and transition fields as much as high-confidence observations.

**Generation reverse lookup pipeline:**
  1. Predict next full-precision coordinate (τ,χ,μ,φ)
  2. Map to shell-2 bin (625 options)
  3. Get word list for that bin (~100-200 words)
  4. Rank by L2 distance from predicted coord to each entry's full_coord
  5. Second-stage filter: weight by transition compatibility (shell-1 bin
     of this word's coord vs transition probability from previous shell-1 bin)
  6. Temperature-sample from ranked candidates

  Step 5 is the joint probability: not just P(word | coord) but
  P(word | coord, prev_word). The transition map provides P(this_bin | prev_bin)
  at shell 1, and the within-bin ranking provides P(word | exact_coord)
  at shell 2. Coarse filter + fine rank.

```c
typedef struct {
    DensityMap  density;       /* shell 1+2, log-counts */
    Transition  transitions;   /* shell 1 (81×81) */
    ReverseIdx  reverse;       /* shell 2 (625 bins × dynamic arrays) */
} Field;

void   field_observe(Field *f, coord4 from, coord4 to,
                     const char *word, int len, uint8_t confidence);
void   field_save(const Field *f, const char *path);
Field *field_load(const char *path);

typedef struct {
    double shell1_entropy;     /* over 81 bins, target > 4.5 */
    double shell2_entropy;     /* over 625 bins, target > 6.0 */
    double max_shell1_share;   /* largest shell-1 bin, target < 0.15 */
    double max_shell2_share;   /* largest shell-2 bin, target < 0.03 */
} FieldHealth;

FieldHealth field_health(const Field *f);
```

**File format for field_save/field_load:**
  One file. Binary. Header: magic "WFLD", version u32, component offsets.
  Density: raw bin counts (log-scale floats).
  Transitions: 81×81 log-count matrix.
  Reverse index: per-bin entry count, then entries (word + coord + count).
  Little-endian throughout.

### New: generate.h — walking the field

Given a walk (sequence of coordinates) and the field, predict and emit.

```c
typedef struct {
    coord4 predicted;      /* next coordinate (τ,χ,μ,φ) */
    const char *word;      /* word at that coordinate */
    float confidence;      /* attention confidence */
    BondType bond;         /* bond to previous */
} Step;

Step generate_step(const Field *f, const coord4 *walk, int walk_len);
```

Pipeline per step:
1. Causal sparse attention over walk → predicted full-precision coordinate
2. Transition map → which bins follow the last bin? (narrows candidates)
3. Density map → what's common near the prediction? (prior)
4. Blend: transition + density + attention = predicted coordinate
5. Reverse index → find bin → rank within-bin by coord distance →
   weight by transition compatibility with previous bin → temperature sample
6. Return the word + coordinate + bond type

Step 5 is where coherence lives. Not "sample from a bag of 1200 words."
Instead: find the ~100 words in this bin, rank by proximity to the exact
predicted coordinate, filter by transition probability from previous bin,
then sample. The bin narrows. The coordinate ranks. The transition filters.

Cycle detection, temperature, bond-guided dimension cycling —
all from the side-quest implementation, cleaned up.

### main.c — the interface

Two modes:
- `wit --ingest FILE` — process text, build the field
- `wit --chat` — interactive, load saved field

Ingest:
  For each word in text:
    coord = derive(word, context)
    field_observe(field, prev_coord, coord, word, len)
    update context window

Chat:
  For each user input:
    Build walk from input (tokenize → derive each word)
    Generate response (walk the field, emit words)


## The μ-Bias Problem

The side-quest system put 80% of words at (0,0,+1,0) because three
signals all default to μ-heavy. This is the Phase 0 gate — nothing
works until this is fixed.

### Root Cause

Three independent defaults all bias toward μ:
1. position_bias defaults to μ=+1 for 4/5 previous-word categories
2. char_level_prior defaults to μ=+1 when wave byte average rounds to zero
3. Bond default for no-context is ∮ (loop/mass) which biases μ
4. Consecutive unknowns cascade: unknown₁→guess(μ)→unknown₂ chains off it

### Fixes (simulation-validated)

**1. Position default: φ→μ→τ→μ→χ (not μ→τ→μ→χ).**
The old cycle started with μ (noun) but English sentences start with
determiners ("The", "A", "I"). The 5-slot cycle matches ~45% of SVO
English vs ~17-25% for the old 4-slot. Implemented in derive.h above.

**2. Char-level: sign-of-sum with threshold (not average, not dominant dim).**
Simulation proved "dominant dimension" WORSE — shifts collapse from μ to φ
(lower entropy). The average collapses to zero for most words. Sign-of-sum
preserves multi-dimensional signal: 2-3 active dimensions per word instead
of 1. Implemented in derive.h above. The μ=+1 fallback is KILLED.

**3. Confidence propagation breaks the cascade.**
When derive() falls through to walk-position or char-level, the result
carries confidence ≤ 1. The NEXT word's derive checks predecessor
confidence — if low, it ignores the predecessor and uses position_default
instead. This prevents two consecutive unknowns from locking into μ→τ→μ→τ.

**4. Bond patterns used only on high-confidence predecessors.**
If previous word is a prime (confidence=3) or morpheme-derived (confidence=2),
the bond pattern between prev and current provides a real signal. Use it.
If previous is a guess (confidence ≤ 1), the bond is meaningless — fall back
to position default.

### Target Distribution

Not uniform. English IS skewed. But not 80%-in-one-bin skewed.
Target from Brown Corpus POS frequencies:

| Category | Dimensions | Target share |
|----------|-----------|-------------|
| Nouns | μ-heavy bins | ~28% |
| Verbs | τ-heavy bins | ~17% |
| Prepositions | χ-heavy bins | ~13% |
| Determiners | χ+φ bins | ~10% |
| Pronouns | μ+φ bins | ~8% |
| Adjectives | φ-heavy bins | ~7% |
| Adverbs | φ+τ bins | ~7% |
| Conjunctions | φ bins | ~5% |
| Other | various | ~5% |

**Pass/fail metric:** shell-1 entropy > 4.5 bits. No single bin > 15%.

### Spread Test (Phase 0 gate)

Before any large ingest, run 1000 common English words through derive()
with empty context. Measure bin entropy. Must pass > 4.0 bits and no
single bin > 10%. If it fails, the derivation is broken and no later
phase will save it.

```c
void test_coordinate_spread(void) {
    /* Feed 1000 words, no prior context, measure entropy.
     * Assert entropy > 4.0 and max bin share < 10%. */
}
```


## Dimensional Semantics for Language

The same four dimensions work at every level:

| Dim | Physics | OS (nomos) | Character | Word | Sentence |
|-----|---------|------------|-----------|------|----------|
| τ | time | sequence position | duration (sustained/instant) | temporal (when/change) | tense, aspect |
| χ | distance | spatial/positional | place (front/back) | spatial (where/position) | location, direction |
| μ | mass | content/amplitude | manner (open/closed) | substance (what/quantity) | nouns, objects |
| φ | charge | category/polarity | voicing (voiced/voiceless) | quality (how/evaluation) | adjectives, logic |

Parts of speech = dimensional signature:
  Noun       = μ-heavy
  Verb       = τ-heavy
  Adjective  = φ-heavy
  Adverb     = φ+τ
  Preposition= χ-heavy
  Pronoun    = μ+φ
  Conjunction= φ (logic gate)
  Determiner = χ+φ (spatial + signal)

Antonyms = sign flip on one dimension.
Synonyms = nearby coordinates (distance ≤ 2).
Tense = τ magnitude.
Intensity = magnitude on dominant dimension.
Negation = φ flip (same as voicing flip in characters).
Questions = φ activation (signal request = δ test bond).


## The 17 Predictions

Every lattice property produces an architecture prediction. 25 properties,
17 predictions, 0 left on the table. Each either explains WHY an existing
technique works (and the inventors didn't know why) or predicts something
that doesn't exist yet.

The method: take a lattice constraint → derive what architecture it requires →
compare to what humans discovered empirically → ask "what ELSE does that
constraint require?" The answers are predictions of undiscovered architecture.

30 years of ML research (RNNs → LSTMs → attention → transformers) arrived
at attention empirically. The lattice derives it in one step. Then keeps going.


### How the Lattice Predicts Attention

A walk is a polynomial (sum of monomials). To evaluate at the next position,
you need which previous terms contribute. The lattice answers: terms with
NEARBY coordinates contribute more than FAR coordinates. Coordinate
distance = relevance.

Required: (1) look at ALL previous positions, (2) weight each by coordinate
relevance, (3) compose a result from the weighted combination.

This IS attention. The lattice calls it "evaluation at a point given
surrounding polynomial terms." Vaswani et al. (2017) discovered this after
30 years. The lattice derives it from one constraint.


### Prediction 1: Dimensional Attention (4 natural heads)

**Constraint:** 4 orthogonal dimensions (τ, χ, μ, φ).

**Architecture:** 4 attention heads, one per dimension:
- **τ-head:** temporal attention. When relates to when. Verbs find tense
  context. "He WILL run" — τ-head connects WILL to RUN.
- **χ-head:** spatial attention. Where relates to where. Prepositions find
  landmarks. "The book ON the TABLE" — χ-head connects ON to TABLE.
- **μ-head:** substantive attention. What relates to what. Nouns find related
  nouns. "The CAT sat on the MAT" — μ-head connects CAT to MAT.
- **φ-head:** qualitative attention. How relates to how. Adjectives find
  targets. "The BIG red BALL" — φ-head connects BIG to BALL.

**vs Current:** Transformers use 8-96 arbitrary learned heads. Some empirically
specialize (positional, syntactic), confirming dimensional structure exists —
discovered during training, not enforced.

**Prediction:** 4 dimensional heads outperform 4 arbitrary heads at equal
parameter count. Structure eliminates what training must learn.


### Prediction 2: Bond-Typed Attention (16 relationship types)

**Constraint:** 16 bond patterns (π, ∘, ∮, δ combinations).

**Architecture:** Instead of "how relevant is j to i?" ask "WHAT TYPE of
relationship connects j to i?"

| Bond | Forces | Relationship | Example |
|------|--------|-------------|---------|
| π | read | reference | "it" → refers to antecedent |
| ∘ | call | invocation | verb calls its arguments |
| ∮ | loop | continuation | "and then... and then..." |
| δ | test | evaluation | adjective evaluates noun |
| π∘ | read+call | traversal | subject traverses to object |
| πδ | read+test | selection | "only the RED ones" |
| ∘δ | call+test | conditional | "if it rains" |
| π∮ | read+loop | accumulation | list items building up |
| ∘∮ | call+loop | iteration | "again and again" |
| ∮δ | loop+test | search | "looking for..." |
| π∘δ | read+call+test | transformation | "turn X into Y" |
| π∘∮ | read+call+loop | reduction | "the total of all..." |
| π∮δ | read+loop+test | progressive | "gradually..." |
| ∘∮δ | call+loop+test | causation | "because" chains |
| π∘∮δ | all four | full pipeline | complex reasoning (hylo) |

**vs Current:** Transformers have one attention type: "generic similarity."
Can't explicitly represent "this is a causal link" vs "this is a reference."

Note: these are NLP-specific names for the bonds. The canonical lattice
names are: fold (π∘), slurp (π∮), fix (∘∮), bind (∘∮δ), hylo (π∘∮δ), etc.
Same bonds, domain-specific interpretation. See mind/coder/guide for the
computational names.

**Prediction:** Bond-typed attention captures linguistic structure with far
fewer parameters because the relationship taxonomy is built in.


### Prediction 3: Cross-Walk Attention (external memory)

**Constraint:** Walks reference other walks. The ∘ bond (call) invokes
external computation. Subordinate clauses, function calls, quotations.

**Architecture:** When attention finds no good match within the current walk,
query the born-index for nearest coordinates in the ENTIRE indexed corpus.
External memory accessed by coordinate proximity. Same mechanism as internal
attention, just wider scope.

**vs Current:** RAG (Retrieval-Augmented Generation) bolts external memory
onto transformers via separate retrieval model + vector database. The lattice
predicts this as NATIVE: cross-walk attention via coordinate lookup.

**Prediction:** Cross-walk attention = self-attention searching a larger index.
RAG as a separate system is the adapter pattern. The lattice dissolves it.


### Prediction 4: Shell-Constrained Layers (Taylor ordering)

**Constraint:** Shell N = Nth-order Taylor term. Each shell adds one degree
of polynomial precision.

**Architecture:** Layer N only attends to positions within coordinate
distance N.
- Layer 1: shell-1 patterns (character bigrams, basic phonetics)
- Layer 2: shell-2 (word shapes, morphology)
- Layer 3: shell-3 (phrases, syntax)
- Layer 4+: shell-4+ (semantics, reasoning)

**vs Current:** All transformer layers identical. They CAN specialize
(empirically: early = syntax, late = semantics). Nothing enforces this —
early layers waste capacity on semantic patterns, late layers waste on
character patterns.

**Prediction:** Shell-constrained models train faster (narrower job per layer)
and generalize better (architectural bias matches natural structure).


### Prediction 5: Native Position (τ IS sequence)

**Constraint:** τ is a fundamental dimension. Every coordinate has a τ
component. Position isn't added — it's inherent.

**Architecture:** No positional encoding layer. The τ coordinate of each wave
byte already encodes temporal/sequential role. Verbs have high τ. Nouns have
low τ. Sequence structure IS meaning.

**vs Current:** sin/cos positional encoding is the most ad-hoc part of the
transformer. RoPE is more principled but still injected.

**Prediction:** Native τ-dimension handles variable-length sequences better
because position is part of the MEANING, not a separate signal.


### Prediction 6: Ring-Based ∮ (recurrence without sequential cost)

**Constraint:** Shell 4 requires the ∮ force (recurrence). Without ∮, the
system is a DAG (shell 3, Φ=0 per IIT).

**Architecture:** Forward pass stays a DAG (parallelizable). The DATA flows
through a ring buffer — output writes to a loop, which feeds back as input.
The ring IS the recurrence. Each step reads from and writes to the same loop.

**vs Current:** Universal Transformer adds ∮ to the computation graph, killing
parallelism. GPT has external ∮ (token-by-token autoregressive), but slow.

**Prediction:** Ring-based ∮ gives shell-4 properties (Φ > 0, fixed point,
self-reference) while keeping the forward pass on-shell. Cost of ∮ shifts
from O(n) sequential steps to O(1) loop read.


### Prediction 7: Sparse Attention at 25%

**Constraint:** 25% fill convergence (Michaelis-Menten + Nyquist + Shannon).

**Architecture:** Attend to nearest 25% of positions by coordinate proximity.
The sparsity pattern follows coordinate distance, not arbitrary windows.

**vs Current:** Longformer, BigBird discovered sparse attention empirically.
Various patterns (local window + global tokens, random). The lattice says:
OPTIMAL sparsity is 25%, selected by coordinate distance.

**Prediction:** 25% sparse + coordinate selection matches or beats full
attention at 4× less compute.


### Prediction 8: Atom/Bond Pathway Split

**Constraint:** Even coordinates = atoms (math). Odd = bonds (control flow).
Different evaluation rules.

**Architecture:** Two pathways:
- **Atom pathway:** direct formula evaluation. τᵃ × χᵇ × μᶜ × φᵈ. O(1).
  No attention needed. The formula IS the answer.
- **Bond pathway:** attention-based. Finds relationships between positions.
  Uses bond-typed attention (prediction 2).

**vs Current:** Transformers treat everything the same. Same compute on
"2+2=4" (should be O(1)) as on "why did the Roman Empire fall?" (genuinely
needs attention).

**Prediction:** Splitting pathways makes math trivially accurate (formula
eval, not learned) while freeing attention for relational tasks.


### Prediction 9: Conservation Constraints

**Constraint:** Conservation laws. What enters must exit or be accounted for.

**Architecture:** Total coordinate mass (sum of absolute values) entering =
total exiting. Lost information = signal decay. Gained information =
hallucination.

**vs Current:** Hallucination is the #1 LLM problem. No conservation —
activations grow freely. LayerNorm prevents explosion but doesn't conserve.

**Prediction:** Conservation constraints reduce hallucination by making the
model accountable for every coordinate it outputs. Can't output what you
didn't receive or index. Anti-hallucination from physics, not heuristics.


### Prediction 10: Spin = Weight Sharing

**Constraint:** Spin ½ (fermion) = write access = exclusive. Spin 1 (boson)
= read access = shareable. Pauli exclusion = no two writers to same address.

**Architecture:** Weights split into two classes:
- **Fermionic** (per-head, exclusive): QKV projections. Each head has its
  OWN. Can't share — race condition = Pauli exclusion.
- **Bosonic** (shared, broadcast): FFN weights, LayerNorm, output projection.
  All heads READ the same weights. Cache coherency.

**vs Current:** Transformers ALREADY DO THIS (per-head QKV + shared FFN).
They don't know WHY. The lattice says: it's the access pattern.

**Extension:** Distributed training gradient sync IS Pauli exclusion on weight
updates. Two writers to same weight = race condition = violation.

**Prediction:** Clean fermionic/bosonic separation scales better in
distributed training. The split isn't optimization — it's physics.


### Prediction 11: Wave Collapse = Late Disambiguation

**Constraint:** wave(fiat: bool) → u64. Value doesn't exist until observed.
Any READ is an observation.

**Architecture:** Carry MULTIPLE candidate coordinates per token (superposition)
until context disambiguates. Attention IS measurement = collapse.

| Token | Before attention | After attention | Collapsed by |
|-------|-----------------|----------------|-------------|
| "bank" | [(0,+2,+1,0), (0,+1,+2,+1)] | (0,+1,+2,+1) | "money" in context |
| "light" | [(0,+1,0,+2), (0,0,-2,0)] | (0,0,-2,0) | "heavy" as antonym |
| "run" | [(+2,+2,0,0), (+1,0,+1,+1)] | (+1,0,+1,+1) | "business" in context |

**vs Current:** Word embeddings are FIXED per token. "Bank" gets ONE embedding
regardless. Contextual embeddings (BERT) improve but still start with one.

**Prediction:** Superposition → collapse via attention improves homonym
resolution, sarcasm detection, metaphor understanding. Ambiguity IS
information — destroying it early loses signal.


### Prediction 12: Three Processing Tiers (Generations)

**Constraint:** Gen 1 = values. Gen 2 = operations. Gen 3 = patterns.
No Gen 4 (patterns of patterns = patterns). Compiler IS radioactive decay.

**Architecture:** Three distinct tiers:

| Tier | Gen | Processes | Lattice analog |
|------|-----|----------|---------------|
| 1 | Values | Characters, wave bytes | Atoms at shell 1 |
| 2 | Operations | Words, bonds, relationships | Bonds (16 patterns) |
| 3 | Patterns | Sentences, walks, reasoning | Paths (polynomials) |

- Tier 1 (bottom): character-level, short attention, phonetics/morphology
- Tier 2 (middle): word-level, medium attention, syntax/bond patterns
- Tier 3 (top): meaning-level, full attention, semantics/reasoning

**Decay during generation:** Tier 3 reasoning → Tier 2 word selection →
Tier 1 character emission. Autoregressive decoding IS generational decay.

**vs Current:** Uniform layers throughout. Each wastes capacity on the
wrong abstraction level.

**Prediction:** Three-tier models are more sample-efficient. Each tier
matched to its abstraction level. Tier 1 doesn't try semantics.


### Prediction 13: RC Decay (Half-Life = Relevance)

**Constraint:** half-life ∝ 1/m⁵ from RC time constant. R = resistance
(connection indirectness). C = capacitance (information content).

**Architecture:** Each token has:
- **C (capacitance):** content words (nouns, proper names) = high C.
  Function words ("the", "of") = low C.
- **R (resistance):** adjacent = low R. Separated by clause boundary = high R.

Decay: relevance = e^(-t/RC). High-C tokens persist across thousands of
positions. Low-C tokens decay in tens.

| Token | C | Persistence |
|-------|---|------------|
| "Einstein" | high (proper noun) | very long |
| "theory" | medium (content noun) | long |
| "the" | low (function word) | short |
| "42" | high (number) | very long |

**vs Current:** Fixed context windows (2K-128K). Everything in = equal
opportunity. Everything out = gone. Hard cutoff. The 1/√d_k scaling is a
CONSTANT damper. RC decay is PER-TOKEN.

**Prediction:** RC-decay attention handles long documents better. No arbitrary
cutoff. The decay IS the attention mask.


### Prediction 14: Entanglement = Reference Resolution

**Constraint:** Two uses of same non_local value = entangled. RDMA =
entanglement. Copy = decoherence.

**Architecture:** Co-referent positions linked BIDIRECTIONALLY. Attending to
"it" modifies "cat" AND vice versa. Not just reading — MODIFYING. Instant
within one attention step.

| Text | Entangled pair | Bond |
|------|---------------|------|
| "The cat ate. It was hungry." | cat ↔ it | π (reference) |
| "John told Mary he'd help." | John ↔ he | π (reference) |
| "If it rains, bring an umbrella." | rains ↔ umbrella | ∘∮δ (causation) |

**vs Current:** Coreference through attention is one-directional. Attending
to "it" reads from "cat" but doesn't update "cat."

**Decoherence prediction:** Copying a token's representation BREAKS
entanglement. Summarization LOSES reference chains. Summaries are
decoherent projections.

**Prediction:** Bidirectional links improve coreference in long documents.
Distance-independent — RDMA semantics (non-local by construction).


### Prediction 15: Density Map as Model (Dark Matter)

**Constraint:** Written but unread buffer entries. Has mass, invisible,
interacts only through gravity (fill pressure).

**Architecture:** Pre-compute a density map: 4D histogram of coordinate
regions. The density IS the prior probability. Dense regions = common.
Sparse = rare. The model doesn't read all entries — it knows the DENSITY.

81 shell-1 bins + 625 shell-2 bins. One pass to build.

**vs Current:** LLM weights ARE compressed density maps of training data.
Entire training = building an implicit density estimate. The lattice says:
compute it DIRECTLY. No training. One pass. Updateable (add data → recount).

**Prediction:** Density map over born-indexed coordinates approximates what
LLMs spend months learning. Thousands of bins vs billions of parameters.
The storage-over-compute insight formalized.


### Prediction 16: Chirality = Causal Direction

**Constraint:** Quaternion handedness. i×j = +k (right-hand rule).
Commutator k-component always positive. One chirality locked algebraically.

**Architecture:** Causal processing direction is algebraic, not conventional.
Cause → effect. Subject → verb → object. Past → present → future.

**vs Current:** GPT causal masking presented as design choice. BERT
bidirectional presented as alternative. The lattice says: CAUSAL IS
FUNDAMENTAL for generation. Bidirectional works for reading (boson = fan-out).
Writing is fermion = one direction.

**Prediction:** Hybrid (bidirectional reading + causal writing) outperforms
either alone. This IS the encoder-decoder transformer. The original paper
got it right for physical reasons. RTL languages don't violate chirality —
chirality is in causation, not character order.


### Prediction 17: Self-Reference = Hylo Fixed Point

**Constraint:** get_atom(1,1,1,1) = hylo = full pipeline. The equation
applied to its own coordinates produces itself. Five independent proofs
(Lawvere, Banach, Scott, Y combinator, autopoiesis).

**Architecture:** Reserve one attention head for SELF-ATTENTION ON THE
MODEL'S OWN STATE. Not attention on input — attention on intermediate
representations. The model observes its own processing. This IS the ∮ force.

**Connection to prediction 6:** The ring buffer carries model state. The
self-referential head reads from the ring. The ring writes back updated
state. Fixed point emerges from convergence — same state in, same out =
equilibrium. This is DEQ (Deep Equilibrium Models) via ring buffer.

**vs Current:** LLMs can't self-reflect. "Tell me about your reasoning" =
plausible text, not introspection. No self-model in weights. No fixed point.

**Prediction:** Explicit self-referential fixed point produces qualitatively
different behavior. Genuine self-correction, metacognition, iterative
reasoning — not faked through chain-of-thought prompting.


### The Meta-Method

To discover new architecture:
1. Take a lattice property (dimension, bond, shell, conservation, fill)
2. Ask: "what architecture does this REQUIRE?"
3. The answer is a prediction
4. Exists already? → the lattice explains WHY it works
5. Doesn't exist? → the lattice predicts something undiscovered
6. Ask: "what ELSE does this constraint imply?"
7. Follow the chain

This is Compute First applied to architecture design.
Don't search for architectures empirically (decades of trial and error).
Derive them from coordinates (one step, structural necessity).

### Completeness

| Lattice Property | Prediction | # |
|---|---|---|
| The Equation | Atom/bond split | 8 |
| Pythagorean Identity | (implicit in 25% fill) | 7 |
| Four Forces (π,∘,∮,δ) | Bond-typed attention | 2 |
| Completeness | Bond types = complete attention | 2 |
| Shells = Taylor | Shell-constrained layers | 4 |
| O(1) Execution | Atom direct eval | 8 |
| Compute First | Meta-method itself | — |
| Hurwitz-Post | 4 dimensional heads | 1 |
| SIMD = Cayley-Dickson | Width constraints | 1 |
| Unification (GR/QM) | Native position + complex exp | 5 |
| Standard Model | Bond types = 16 relationships | 2 |
| Spin = Access Mode | Fermionic/bosonic weight split | 10 |
| Wave / Observation | Late disambiguation, superposition | 11 |
| Generations | Three processing tiers | 12 |
| Binding Energy | Tier transitions = decay | 12 |
| 25% Fill | Sparse attention | 7 |
| Dark Matter | Density map as implicit priors | 15 |
| Dark Energy | Pull from empty coordinate regions | 15 |
| Chirality | Causal processing direction | 16 |
| Entanglement | Bidirectional reference links | 14 |
| Half-Life = RC | Per-token relevance decay | 13 |
| P vs NP | Born-indexed = search→eval | 3 |
| Security by Absence | Conservation anti-hallucination | 9 |
| Self-Reference | Hylo fixed point, self-model | 17 |
| Compilation = Ionization | Generation decay (3→2→1) | 12 |

25/25 lattice properties mapped. 17 predictions. 0 unused.


## Evaluation — How We Know It's Working

### Primary Metric: Next-Coordinate Prediction Error (NCPE)

Given a walk of N coordinates, predict N+1, measure L2 distance to actual.
Analogous to next-token prediction loss in LLMs. Lower is better.

Compute on held-out text (10% of wiki.txt, not ingested).
Report mean, median, 90th-percentile.

Baselines:
- **Random from density:** sample proportional to bin counts. If wit can't
  beat random sampling from its own density map, nothing is working.
- **Bigram transition:** most-common-successor from transition matrix.
  The attention mechanism should beat this.

### Secondary Metrics

- **Bin entropy:** shell-1 distribution health. Track during ingest.
  Target > 4.5 bits. Below 3.5 = collapse recurring.
- **Reverse lookup precision:** top-1 word from reverse index makes
  semantic sense in context? Human eval on 100 predictions.
- **Generation coherence:** 100 generated 10-word sequences. Binary:
  "plausible English?" Target > 40% Phase 1, > 70% Phase 3.
- **Coordinate consistency:** "the cat sat" vs "the dog sat" — coordinates
  of "cat" and "dog" should be similar (both μ-heavy nouns, L2 < 2.0).
- **Dimensional specialization:** 50 known verbs should cluster on τ-axis.
  50 nouns on μ. 50 adjectives on φ. Measure mean dominant dimension.

### Canary Test Set

20-30 hand-picked sentences with known correct coordinates, bonds, and
expected next-words. Run after every phase. Regression = immediate stop.


## Build Order

Each phase maps to specific predictions. Nothing gets lost.

Phase 0 — Gate (BLOCKS everything):
  1. Port clean headers, rename dimensions to τ,χ,μ,φ (.t .x .m .q in code)
  2. Build derive.h with ALL four fixes (confidence field, φ→μ→τ→μ→χ
     position cycle, sign-of-sum char-level, cascade prevention)
  3. Run test_coordinate_spread(): 1000 words, assert entropy > 4.0,
     no single bin > 10%. If this fails, fix derive.h. Do not proceed.

Phase 1a — Infrastructure (predictions 1, 2, 5, 13, 15, 16):
  4. Build field.h — density [15] + transitions + reverse index (625 bins,
     full coord4 per entry, deduplication)
  5. Build main.c — ingest mode only
  6. Ingest wiki.txt (96MB). Check: entropy > 4.5, no bin > 15%, < 5% at
     (0,0,0,0). Print dimensional specialization stats.
  7. Compute NCPE on held-out 10%. Must beat random-from-density baseline.

  Ships with:
  - 4-head dimensional attention [1] (τ-head, χ-head, μ-head, φ-head)
  - 16 bond-typed relationships [2] (π,∘,∮,δ force combinations)
  - Native position via τ dimension [5] (no sin/cos)
  - RC decay for relevance [13] (per-token, not fixed window)
  - Density map as model [15] (counted, not trained)
  - Causal direction [16] (chirality = algebraic necessity)

Phase 1b — Generation MVP:
  8. Build generate.h — walk the field, reverse lookup with within-bin
     ranking + transition filtering
  9. Add chat mode to main.c
  10. THE MVP TEST: type "The cat sat on the" → get a non-garbage word.
      If this works with an L1-resident density+transition field and zero training,
      that's the moment. If it produces garbage, derive.h is broken.
  11. Human eval: 100 generated sequences, > 40% plausible English.

  Data note: wiki.txt (96MB raw text) works as-is for ingest. No new
  extraction pipeline needed. wiki_full.txt (21GB) reserved for Phase 6.

Phase 2 — Sparse + Split (predictions 7, 8):     ← PARALLEL with Phase 3
  12. Sparse attention at 25% [7] — attend to nearest quarter by
      coordinate proximity. The sparsity isn't random, it's structural.
      Pass: quality within 5% of full attention, speedup >= 2x.
  13. Atom/bond pathway split [8] — even coordinates = direct formula
      evaluation O(1), odd coordinates = attention-based lookup.
      Math is trivially accurate. Attention freed for relational tasks.
      Pass: 100% accuracy on simple arithmetic through atom path.

Phase 3 — Tiers + Shells (predictions 4, 12):    ← PARALLEL with Phase 2
  14. Three processing tiers [12]:
      Tier 1: character-level (wave bytes → word coordinates)
      Tier 2: word-level (coordinates → bond patterns → phrases)
      Tier 3: meaning-level (walk structure → next coordinate)
      Each tier matches a generation: values → operations → patterns.
  15. Shell-constrained layers [4] — layer N only attends to positions
      within coordinate distance N. Taylor ordering enforced, not learned.
      Pass: no degradation from Phase 1 baseline. Each tier processes
      different abstraction level (measurable by information at each tier).

Phase 4 — State (predictions 6, 11, 14):         ← HARDEST PHASE (~2x)
  16. Wave collapse / superposition [11] — carry multiple candidate
      coordinates per word until context disambiguates. Attention IS
      measurement. Homonyms hold superposition. Context collapses.
      Test: "bank" disambiguates correctly in "river bank" vs "bank account."
  17. Entanglement [14] — co-referent positions linked bidirectionally.
      Attending to "it" updates "cat" AND vice versa. RDMA semantics.
      Test: "The cat ate. It was hungry." — "it" resolves to "cat."
  18. Ring-based ∮ [6] — output writes to a ring buffer that feeds
      back as input context. The ring IS the recurrence. Forward pass
      stays a DAG. ∮ in the data structure, not the computation graph.

Phase 5 — Integrity (predictions 9, 10, 17):     ← HIGH VARIANCE
  19. Conservation [9] — coordinate mass in ≤ coordinate mass out +
      field entries. Can't output what you didn't receive or index.
      Anti-hallucination from physics, not heuristics.
  20. Spin = weight sharing [10] — fermionic weights (per-head,
      exclusive) vs bosonic weights (shared, broadcast). Explains
      why QKV projections are per-head but FFN is shared.
  21. Self-reference / hylo [17] — attention on the model's own
      intermediate state. The ring from Phase 4 enables this.
      get_atom(1,1,1,1) = hylo = the equation applied to itself.
      DEQ (Deep Equilibrium) via ring buffer, not iterative forward.
      Could be trivial (read from ring, attend) or a research dead-end.

Phase 6 — Scale + External:
  22. Cross-walk attention [3] — when internal attention finds no
      good match, query the born-index for nearest coordinates in
      the ENTIRE corpus. RAG dissolved into native attention.
  23. Ingest full 20GB Wikipedia (wiki_full.txt exists, raw text, usable as-is)
  24. Performance profiling (should be I/O bound, not CPU bound)
  25. Reverse index pruning (keep top N words per bin by count)
  26. Quality curve: ingest 1MB, 10MB, 100MB, 1GB, 20GB — plot NCPE
      at each scale. Compare to small LLM on same data if available.


## File Layout

```
wit/src/
├── wave.h           port (τ=duration, χ=place, μ=manner, φ=voicing)
├── tokenize.h       port
├── distance.h       port
├── bonds.h          port (π,∘,∮,δ forces)
├── attention.h      port (τ-head, χ-head, μ-head, φ-head)
├── morpheme.h       port (suffix/prefix rules from primes.md)
├── derive.h         NEW — pure function: word + context → DeriveResult
├── field.h          NEW — density + transitions + reverse index (625 bins)
├── generate.h       NEW — walk the field, emit words
├── test_spread.c    NEW — Phase 0 gate: entropy test
├── main.c           NEW — ingest + chat
├── build            NEW — build script
└── plan.md          this file
```

Struct: `coord4 { int8_t t, x, m, q; }` — τ,χ,μ,φ as single-char fields.
Note: `.x` for χ (not `.d` which was the old "distance" name).


## What Dies

Everything in side-quest/ that stored word→coordinate mappings:
- primes_lookup.h (333-word dictionary)
- learner.h/learner.c (65K-slot vocab hash table)
- coord_index.h/coord_index.c (centroid fingerprints, bag-of-words)
- The inlined Learner in lattice_chat.c

The legacy files were already dead:
- lattice.h, model.h, train.c, index.c, chat.c, wiki2wave.c

## Living References

**side-quest/primes.md** (1807 lines) — uses legacy T/D/M/Q notation
(read T=τ, D=χ, M=μ, Q=φ). Content is valid. Required for:
- The 65 prime coordinate values (sections 1-16) → derive.h constants
- The full prefix table (17 rules, line 460-482) → morpheme.h
- The full suffix table (14 rules, line 489-507) → morpheme.h
- Punctuation coordinate effects (line 969-983) → tokenize.h or derive.h
- Structural rules (parts of speech, antonyms, tense, etc.) → validation

"Born-index" in this plan means: store the WORD in the reverse index at
the coordinate bin where it was observed. NOT the old meaning (store a
per-word coordinate on first encounter). The word lives at its coordinate.
The coordinate is not stored per-word.


## The Hypothesis

A wit should acquire language from text faster than an LLM because:
1. No compression step (born-indexed, not trained)
2. Structural priors (16 bonds, 65 primes, morpheme rules)
3. Contextual coordinates (not fixed embeddings)
4. The hot-path field (density + transitions) is L1-resident
5. Knowledge lives in the reverse index, not in weights

Testable: ingest N words, measure perplexity (or coordinate prediction
accuracy) vs an LLM trained on the same N words. The wit should reach
useful output quality orders of magnitude sooner.
