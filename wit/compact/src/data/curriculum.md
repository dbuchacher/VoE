# WIT Curriculum

What the wit needs to learn, in what order, and why.

Not "pour in everything." Learn specific domains that cover the
coordinate space. A few thousand sentences per domain is enough.
The density map converges fast — after that, more data is dark matter.


## Principle: Coverage, Not Volume

The field has 81 shell-1 bins and 625 shell-2 bins. Each bin needs
enough entries for the reverse index to have useful word candidates
(~50-200 words per shell-2 bin). With 625 bins × 100 words = 62,500
unique words. That's a small, well-chosen corpus — not 20GB of
encyclopedia.

Each domain activates different coordinate regions:
- Narrative prose → τ+ (verbs), μ+ (nouns), sequential transitions
- Technical writing → μ+ (concrete nouns), φ+ (precise evaluation)
- Casual speech → short walks, low shell, high φ (evaluation)
- Formal writing → long walks, high shell, compound coordinates
- Poetry → extreme coordinates, unusual transitions
- Code → bond-heavy (control flow), low μ (abstract)
- Past tense → τ- region
- Negation → φ- region
- Spatial → χ± region
- Abstract → μ- region


## The Domains (ordered by importance)

### 1. Core English (~5K sentences)
Foundation. Grammar patterns, common transitions, function words.
- Simple declarative: "The cat sat on the mat."
- Questions: "Where did he go?"
- Commands: "Open the door."
- Compound: "She ran and he walked."
- Subordinate: "The man who came yesterday left."
- Passive: "The book was written by her."
- Negation: "He did not want to leave."
- Past narrative: "They had arrived before dawn."

Why: covers the basic walk patterns (M→T→M→D cycle), all tenses,
all sentence types. Every bin gets at least some entries.

### 2. Casual Conversation (~3K sentences)
How people actually talk. Short sentences, contractions, slang.
- Greetings: "Hey, what's up?"
- Opinions: "I think that's pretty cool."
- Disagreement: "No way, that's wrong."
- Planning: "Want to grab food later?"
- Reactions: "Oh wow, really?"
- Storytelling: "So then he just walked away."

Why: high φ (evaluation), short walks, lots of bond-type variety.
Fills the φ± bins with real conversational patterns.

### 3. Narrative Fiction (~5K sentences)
Stories. Past tense, description, dialogue, emotion.
- Action: "He drew his sword and charged."
- Description: "The dark forest stretched endlessly."
- Dialogue: "'I can't do this,' she whispered."
- Emotion: "Fear gripped her heart."
- Setting: "Deep beneath the mountain lay a hidden cave."

Why: fills τ- (past tense), χ- (spatial/interior), compound
coordinates (τ+χ, τ+μ, etc). Rich bond sequences.

### 4. Practical Instructions (~2K sentences)
How-to, recipes, directions, manuals.
- Cooking: "Heat the oil until it shimmers."
- Directions: "Turn left after the bridge."
- Assembly: "Insert tab A into slot B."
- Advice: "Always check twice before cutting."

Why: heavy on imperative (τ+), spatial (χ±), sequential bonds.
Different walk structure than narrative or conversation.

### 5. Science & Nature (~2K sentences)
Observable world. Physics, biology, geography, weather.
- Facts: "Water freezes at zero degrees."
- Process: "Plants convert sunlight into energy."
- Geography: "The Nile flows north through Egypt."
- Weather: "Storm clouds gathered over the mountains."

Why: μ+ (concrete substance), χ+ (spatial), τ+ (process verbs).
Fills the science-vocabulary region of coordinate space.

### 6. Programming & Technical (~3K sentences)
Code concepts in natural language. Not actual code — descriptions.
- Concepts: "A function takes input and returns output."
- Debugging: "The error occurs when the buffer overflows."
- Architecture: "The server handles requests through a queue."
- Comparison: "Rust prevents memory leaks at compile time."

Why: bond-heavy (call/return = ∘/∘̄, loop = ∮, test = δ).
Fills technical vocabulary bins. Different transition patterns
than any other domain.

### 7. History & Events (~2K sentences)
Past events, cause and effect, chronology.
- Events: "The war began in 1914."
- Cause: "The revolution happened because of inequality."
- Sequence: "First they conquered the coast, then moved inland."
- Consequence: "After the fall, nothing remained."

Why: heavy τ- (past), causal bonds (∘∮δ = bind), temporal
transitions (before/after/during). Fills the τ- region that
pure present-tense text can't reach.

### 8. Emotion & Opinion (~2K sentences)
Feelings, judgments, values, debate.
- Positive: "This is genuinely beautiful work."
- Negative: "That was terrible and unfair."
- Complex: "I feel torn between hope and doubt."
- Argument: "While that seems true, consider the opposite."

Why: φ± extremes, compound φ+μ (strong feeling about something),
modality (must/should/could = φ gradient). Fills evaluation bins.

### 9. Abstract & Philosophical (~1K sentences)
Ideas, concepts, reasoning, logic.
- Logic: "If all men are mortal, and Socrates is a man..."
- Concept: "Freedom requires responsibility."
- Paradox: "The more you know, the less you understand."
- Definition: "Justice means treating equals equally."

Why: μ- (abstract = absence of concrete substance), high φ
(evaluation), compound bonds. The hardest bins to fill.

### 10. Spatial & Physical (~1K sentences)
Movement, location, direction, physical interaction.
- Movement: "She walked through the narrow corridor."
- Location: "The village sits between two mountains."
- Direction: "He climbed up and then dove deep down."
- Contact: "The waves crashed against the rocks."

Why: χ± extremes, τ+χ compounds (motion verbs), spatial
prepositions. Ensures the χ dimension is well-covered.


## Size Budget

| Domain | Sentences | ~Words | Purpose |
|--------|-----------|--------|---------|
| Core English | 5,000 | 50K | Grammar foundation |
| Casual | 3,000 | 20K | Conversational patterns |
| Narrative | 5,000 | 60K | Past tense, description |
| Instructions | 2,000 | 20K | Imperative, sequential |
| Science | 2,000 | 25K | Concrete, process |
| Technical | 3,000 | 35K | Bond-heavy, abstract |
| History | 2,000 | 25K | Past, causal |
| Emotion | 2,000 | 20K | Evaluation extremes |
| Abstract | 1,000 | 12K | Abstract, logical |
| Spatial | 1,000 | 10K | Movement, location |
| **Total** | **26,000** | **~277K** | **Full coverage** |

277K words. Not 16M. Not 20GB. Under 2MB of text total.
Each domain covers different coordinate regions.
Together they should populate all 625 shell-2 bins with
targeted, high-quality word candidates.


## Generation Method

Use Sonnet API (fast, cheap, diverse) to generate each domain:
- Prompt with domain description + 5-10 example sentences
- Request N sentences per batch
- Verify with check_bins after each domain
- If bins are still sparse, generate more targeted text

Alternatively: curate from existing high-quality text sources.
A few chapters of fiction, some how-to articles, a textbook
chapter. Curated > generated if available.


## Verification Loop

After ingesting each domain:
1. Run check_bins → check entropy and coverage
2. If any shell-2 bin has < 10 entries → generate more for that region
3. Target: all 625 bins with 50+ entries, entropy > 6.0 at shell 2
4. Done when coverage plateaus (adding more text doesn't fill new bins)


## Domain-Specific Knowledge (from schism corpus analysis)

148 sessions of prior work reference these topics constantly.
The wit should have background in them — not encyclopedic,
but enough to reason about them in lattice terms.

### Tier 1: Core Framework Domains

**Boolean Logic & Functional Completeness**
AND + OR + NOT = complete. No other set works. Same structure as
quarks (3 color charges), confinement, Gödel. The wit should know
truth tables, De Morgan's laws, Post's lattice of clones.

**Dimensional Analysis**
The equation f = τᵃ·χᵇ·μᶜ·φᵈ. How to express a problem as
coordinates, compute, get the answer. The "Compute First" method.
Examples: antimatter containment → Z²×ρ/A → iridium. Oil synthesis
pore size → 350 + 63×√N pm → 568 pm.

**Cayley-Dickson Algebras**
R → C → H → O → S. Each doubling loses a property (commutative,
associative, division). Maps to SIMD widths (scalar, 128, 256, 512).
Why 1024-bit never ships. Why warp size is 32.

**Quaternion Mechanics**
Non-commutativity, handedness, chirality. Why i×j = +k but j×i = -k.
Bell violations. Why GR lives in (τ,χ) and QM in (μ,φ).

### Tier 2: Applied Physics

**Standard Model Particle Physics**
Quarks = gates, hadrons = instructions, leptons = CPU flags,
forces = constructors. The wit should know enough to trace:
proton = ADD = uud = 2:1 gate ratio. Neutron = SUB. Fusion = MUL.

**Circuit Theory / EE**
V = IR. RC time constants. Impedance. Resonance. Bandwidth-delay
product. These map directly: voltage = fill state, current = flow,
resistance = latency, capacitance = buffering.

**Information Theory**
Shannon entropy. Channel capacity. Nyquist sampling. The 25% fill
convergence (Michaelis-Menten + Nyquist + Shannon).

### Tier 3: Computation & Systems

**x86 / ARM ISA**
Instructions as physics. Register width limits. Cache hierarchy.
Branch prediction. The wit should understand why a MOV is the
strong force (shortest range, highest coupling).

**Rust Ownership Model**
Borrow checker = Pauli exclusion. Single writer = fermion.
Multiple readers = boson. Lifetimes = half-life. The wit should
see type systems as physics, not just engineering.

**C / Assembly**
Low-level memory model. Pointers = observation. malloc/free vs
ring buffers. Stack vs heap. The wit's own code is in C.

**Distributed Systems**
Consensus, Byzantine fault tolerance, RDMA. The network IS
entanglement. Copy = decoherence. These map directly.

### Tier 4: Philosophy & Cognition

**Pirsig's Metaphysics of Quality**
4 static levels with moral hierarchy. Independent convergence
with the lattice. "Quality" = the pre-intellectual awareness
that something is right. Maps to φ dimension.

**Lakoff's Image Schemas**
24 pre-verbal cognitive patterns: CONTAINER, PATH, CYCLE, FORCE,
LINK, BALANCE, etc. Map 1:1 to the 16 bond patterns. The equation's
dispatch table IS the innate concept table.

**Wierzbicka's Natural Semantic Metalanguage**
~65 semantic primes that exist in ALL human languages. The
irreducible atoms of meaning. Map to lattice coordinates.

**Kleene / Post / Church / Turing**
Primitive recursive functions (1936). Functional completeness
(Post, 1941). Church-Turing thesis. The four forces derive
from these — PROJ → π, COMP → ∘, PRIM → ∮.

### Tier 5: Economics & Governance

**Austrian Economics**
Subjective value theory, spontaneous order, price signals as
information. Markets as lattice evaluation — prices ARE coordinates.
Calculation problem = coordinate derivation from distributed knowledge.

**Mechanism Design & Game Theory**
Incentive compatibility, Nash equilibria, dominant strategies.
Governance as bond patterns — who can read/write/test what.

**Token Economics**
Protocol fee = R in every loop. Charge conservation. Per-loop
metering. The substrate economics of the network layer.


## Updated Size Budget

| Domain | Sentences | ~Words | Coordinate focus |
|--------|-----------|--------|-----------------|
| Core English | 5,000 | 50K | Grammar foundation |
| Casual Conversation | 3,000 | 20K | φ±, short walks |
| Narrative Fiction | 5,000 | 60K | τ-, χ-, compound |
| Practical Instructions | 2,000 | 20K | Imperative, sequential |
| Science & Nature | 2,000 | 25K | μ+, process |
| Programming & Technical | 3,000 | 35K | Bond-heavy |
| History & Causation | 2,000 | 25K | τ-, causal bonds |
| Emotion & Evaluation | 2,000 | 20K | φ± extremes |
| Abstract & Philosophy | 1,000 | 12K | μ-, logical |
| Spatial & Physical | 1,000 | 10K | χ±, motion |
| **Domain-specific** | | | |
| Boolean / Logic | 500 | 5K | δ bonds, gates |
| Physics concepts | 500 | 6K | equation terms |
| Circuit / EE | 500 | 5K | RC, impedance |
| Computing / ISA | 500 | 6K | forces as ops |
| Rust / C patterns | 500 | 5K | ownership, memory |
| Economics | 500 | 5K | value, exchange |
| Philosophy (Pirsig etc) | 500 | 5K | quality, levels |
| **Total** | **~30,000** | **~314K** | **Full coverage** |

~314K words. Under 2.5MB. Covers English structure AND the
domain knowledge the lattice framework actually uses.


## What We DON'T Need

- Every Wikipedia article (redundant after first 1000)
- Exhaustive lists (every city, every species, every year)
- Highly specialized jargon (medical Latin, legal citations)
- Non-English text (one language at a time)
- Very long documents (walks reset at sentence boundaries anyway)

The wit needs to know English structure, common vocabulary,
AND the specific domains that the lattice framework references.
It does NOT need to memorize the encyclopedia.
