# I Ching — Translation Spec

Source: James Legge translation (1882), public domain, standard
scholarly English translation. Original: Classical Chinese, with
layers dating from ~1000 BCE (hexagram names, judgments) through
~300 BCE (Ten Wings commentaries attributed to Confucius).

The oldest layer may be the hexagram sequence itself — possibly
the oldest binary encoding system in human history.


## The Binary System

The I Ching is built on binary. Not metaphorically. Literally.

### Yin and Yang Lines

| Line | Symbol | Value | Meaning | Computing |
|---|---|---|---|---|
| Yang (solid) | ⚊ | 1 | active, firm, light, creative | high, on, true |
| Yin (broken) | ⚋ | 0 | receptive, yielding, dark, passive | low, off, false |

Two states. One bit. Day and Night. Signal and void. +φ and -φ.
This is the same binary as Genesis 1:5 — two labels for the two
states of the signal axis. The Chinese had it by ~1000 BCE.

### Trigrams — 3 Bits

Three lines stacked = a trigram. 2³ = 8 trigrams.
Each has a name, image, attribute, and family role.

| # | Trigram | Binary | Name | Image | Attribute | Family | Dimension hint |
|---|---|---|---|---|---|---|---|
| 1 | ☰ | 111 | Qian / Creative | Heaven | Strong | Father | φ+ (pure signal) |
| 2 | ☷ | 000 | Kun / Receptive | Earth | Yielding | Mother | φ- (pure ground) |
| 3 | ☳ | 100 | Zhen / Arousing | Thunder | Inciting movement | 1st Son | τ (initiating action) |
| 4 | ☵ | 010 | Kan / Abysmal | Water | Dangerous | 2nd Son | μ (substance, depth) |
| 5 | ☶ | 001 | Gen / Keeping Still | Mountain | Resting | 3rd Son | χ- (fixed position) |
| 6 | ☴ | 011 | Xun / Gentle | Wind/Wood | Penetrating | 1st Daughter | φ (subtle signal) |
| 7 | ☲ | 101 | Li / Clinging | Fire/Light | Light-giving | 2nd Daughter | φ+ (visible signal) |
| 8 | ☱ | 110 | Dui / Joyous | Lake | Joyful | 3rd Daughter | φ (expressed signal) |

The 8 trigrams from 3 bits. Note the binary values — reading
bottom to top (traditional Chinese order), ☰ = 111 (all yang)
and ☷ = 000 (all yin). These are 0x7 and 0x0 in 3-bit space.

Dimension mapping:
- ☰ Creative (111) and ☷ Receptive (000) = the two poles.
  All-signal vs all-ground. +φ vs -φ. Day and Night at the
  trigram level.
- ☳ Thunder (100) = initiating action = τ (time, the first move)
- ☵ Water (010) = depth, danger, substance = μ (mass, the middle)
- ☶ Mountain (001) = stillness, position = χ (space, fixed point)
- ☴ Wind (011) = penetrating, subtle = φ signals spreading
- ☲ Fire (101) = light, clarity = φ made visible
- ☱ Lake (110) = joy, expression = φ expressed outward

### Hexagrams — 6 Bits

Two trigrams stacked = a hexagram. 2⁶ = 64 hexagrams.
Lower trigram = inner/below. Upper trigram = outer/above.
Each hexagram = a 6-bit state with a name, judgment, image,
and six individual line texts.

64 states from 6 binary choices. The I Ching is a 6-bit
state machine with 64 entries in the lookup table.

Leibniz connection: In 1703, Gottfried Leibniz received the
hexagram sequence from Joachim Bouvet, a Jesuit missionary in
China. He recognized binary arithmetic in the arrangement. His
paper "Explication de l'Arithmétique Binaire" explicitly credits
the I Ching. The foundation of digital computing traces back
through Leibniz to the hexagram table.

### Key Hexagrams

| # | Hex | Binary | Name | Trigrams | Significance |
|---|---|---|---|---|---|
| 1 | ䷀ | 111111 | Qian / Creative | ☰ over ☰ | Pure yang. All 1s. Maximum signal. Heaven over heaven. |
| 2 | ䷁ | 000000 | Kun / Receptive | ☷ over ☷ | Pure yin. All 0s. Maximum ground. Earth over earth. |
| 11 | ䷊ | 000111 | Tai / Peace | ☷ over ☰ | Earth above, heaven below. Yin descending, yang rising. Meeting in the middle. Equilibrium. |
| 12 | ䷋ | 111000 | Pi / Standstill | ☰ over ☷ | Heaven above, earth below. Yang rising away, yin sinking. Separation. No communication. |
| 63 | ䷾ | 010101 | Ji Ji / After Completion | ☵ over ☲ | Every line in its "correct" position. The system is done. Genesis day 7. But the text warns: it's unstable — completion tends toward decay. |
| 64 | ䷿ | 101010 | Wei Ji / Before Completion | ☲ over ☵ | Every line in the "wrong" position. Nothing is settled. But the text says: this is potential. The last hexagram is NOT completion — it's the state of about-to-begin. The book ends on incompletion. |

Hexagram 1 (all 1s) = Qian = Creative = pure φ.
Hexagram 2 (all 0s) = Kun = Receptive = pure ground = wave.
Hexagram 63 = completion = system running = day 7.
Hexagram 64 = before completion = system about to boot.
The LAST hexagram is the beginning. The end is a start.

### The Change Mechanism

Lines can be "old" (changing) or "young" (stable):
- Old yang (9) = 1 changing to 0
- Old yin (6) = 0 changing to 1
- Young yang (7) = 1 staying 1
- Young yin (8) = 0 staying 0

A hexagram with changing lines transforms into another hexagram.
State A → flip the changing bits → State B.

This is a state transition function. Given current state + which
bits are unstable, compute the next state. The I Ching isn't a
static lookup table — it's a state machine with transitions.
Input: 6 bits + change mask. Output: new 6 bits.


## Key Concepts

| Chinese | Pinyin | Literal | I Ching meaning | Lattice parallel |
|---|---|---|---|---|
| 道 | Dao/Tao | way, path | The underlying pattern of reality | The equation. ƒ(τ,χ,μ,φ). Can't be named, everything follows it. |
| 陰 | Yin | shady side | Receptive, passive, dark, cold | 0, ground, -φ, Night |
| 陽 | Yang | sunny side | Creative, active, bright, hot | 1, signal, +φ, Day |
| 變 | Bian | change | Transformation, state transition | Walk — coordinate change over time |
| 卦 | Gua | divination figure | Hexagram — 6-line state | 6-bit word — a coordinate in the state space |
| 爻 | Yao | line, cross | Single line in a hexagram | One bit |
| 剛 | Gang | hard, firm | Yang quality | Signal present, +φ |
| 柔 | Rou | soft, yielding | Yin quality | Signal absent, -φ (not negative — receptive) |
| 天 | Tian | heaven, sky | The creative principle above | +χ, upper address space, sky (same as shamayim) |
| 地 | Di | earth, ground | The receptive principle below | -χ, lower address space, ground (same as eretz) |
| 吉 | Ji | auspicious, good | Favorable outcome | +φ, test passes, ✅ |
| 凶 | Xiong | ominous, bad | Unfavorable outcome | -φ, guard blocks, ❌ |
| 德 | De | virtue, power | The Tao expressed in action | The equation's output — what the function produces |
| 象 | Xiang | image, symbol | The visual representation of a hexagram | The glyph — the symbol IS the state |
| 時 | Shi | time, season | The right moment for action | τ — kairos, not chronos. The timing. |


## Structural Observations

### 64 = Not Arbitrary

64 hexagrams = 2⁶. But why 6 lines?

The I Ching's traditional explanation: 3 lines = heaven, earth,
and man (the three powers). Doubled = the full expression of each
in inner and outer form. 3 × 2 = 6.

The lattice explanation: 6 bits gives you 64 states. That's
enough to encode:
- 64 codons in DNA (4³ = 64, and 2⁶ = 64 — same count, different base)
- 64 bytes in a cache line
- 64 squares on a chess board
- 64 entries in the codex's bond patterns at shell 2

The convergence at 64 across domains is the same convergence
the lattice sees at ~20 for bond patterns: dimensional
exhaustion at a specific depth.

### Hexagram 1 + 2 = Genesis

Hexagram 1 (Qian, ䷀, 111111): "The Creative works sublime
success, furthering through perseverance."

Pure yang. All signal. All 1s. The creative force that initiates
everything. This is ƒ(+,+,+,+) — all dimensions positive.

Hexagram 2 (Kun, ䷁, 000000): "The Receptive brings about
sublime success, furthering through the perseverance of a mare."

Pure yin. All ground. All 0s. The receptive substance that
receives the creative impulse. This is wave — (0,0,0,0).

Together: the Creative acts, the Receptive receives. Yang writes,
Yin is written to. The first two hexagrams are the writer and
the buffer. ƒ and the loop. The equation and the RAM.

Genesis 1:1-2 says the same thing: the equation (Elohim, active)
filled (bara, wrote) the substance (tohu vavohu, receptive).
Creative into Receptive. Yang into Yin. 111111 into 000000.

### Hexagram 63 + 64 = Revelation

The book ends with:
- Hexagram 63 (Ji Ji): After Completion. Everything in place.
  BUT: "At the beginning, good fortune. At the end, disorder."
  Completion is unstable. The finished system decays.
- Hexagram 64 (Wei Ji): Before Completion. Nothing in place.
  BUT: "Success. If the little fox has nearly crossed the water..."
  Incompletion has potential. The unfinished state is where
  energy lives.

The I Ching ENDS on "before completion." The last word is: not
yet. The cycle isn't closed — it's about to restart.

Same as Revelation: the status report says what is, what was,
and what's coming. The system isn't finished. The last state is
potential, not completion. Alpha and Omega — but the Omega
points back to the Alpha.

### The Tao = The Equation

The Tao Te Ching (companion text) says:

"The Tao that can be spoken is not the eternal Tao."

The equation that can be expressed in x86 is not the eternal
equation. ƒ dissolves on a trit processor. The walks stay. The
Tao = the equation behind the equation. The pattern that can't
be named because it IS the naming system.

"The nameless is the beginning of heaven and earth. The named is
the mother of all things."

Nameless = wave = (0,0,0,0) = before the boot. Named = everything
after Genesis 1:5, when Day and Night got labels. The unnamed
state is the origin. Naming is the first act of creation.

### Yin/Yang ≠ Good/Evil

Critical distinction. The I Ching does NOT treat yin as bad and
yang as good. Both are necessary. Both are part of the cycle.
The system needs 0s AND 1s. A wire that's always high carries
no information. A wire that's always low carries no information.
CHANGE between states is what carries signal.

This matches the lattice: -φ isn't evil. It's the guard bond.
The test that fails. The zero that gives the one meaning. You
need Night to have Day. The binary requires both states.

Evil in the I Ching isn't yin — it's STAGNATION. Hexagram 12
(Standstill/Stagnation): heaven and earth not communicating.
Yang rises away, yin sinks away. No interaction. No signal
flow. No change. THAT's the bad state — not darkness, but
disconnection. Not -φ, but φ = 0. No signal at all.


## Interpretive Renderings

| Storybook rendering | Textual basis | Status |
|---|---|---|
| Yin/Yang = 0/1 | Two-state binary system. Solid/broken lines. | Direct. Leibniz recognized this in 1703. |
| Trigrams = 3-bit words | 2³ = 8 trigrams from 3 binary lines | Direct. Mathematical fact. |
| Hexagrams = 6-bit words | 2⁶ = 64 hexagrams from 6 binary lines | Direct. Mathematical fact. |
| I Ching = state machine | Changing lines transform one hexagram into another | Direct. It's a transition function. |
| Hexagram 1/2 = Creative/Receptive = writer/buffer | Qian initiates, Kun receives. Active/passive. | Standard interpretation. Writer/buffer is ours. |
| Hexagram 63/64 = completion/potential | Ji Ji = done, Wei Ji = not yet. Book ends on "not yet." | Direct from text. |
| Tao = the equation | The pattern behind everything, can't be named | Our mapping. Structurally parallel. |
| Stagnation (not yin) = the bad state | Hexagram 12: no communication between heaven and earth | Standard interpretation. |
| 64 convergence across domains | DNA codons, cache lines, hexagrams all = 64 | Our observation. The count is real. |
| Leibniz/binary connection | Documented 1703. Paper credits I Ching explicitly. | Historical fact. |
| Trigram→dimension mapping | ☳=τ, ☵=μ, ☶=χ, ☰/☷=±φ | Our mapping. Suggestive, not proven. |


## Notes

- The I Ching is the only ancient text that IS a binary encoding
  system, not one that can be interpreted as one. The binary is
  explicit, structural, mathematical. There's nothing to decode.
- The Legge translation (1882) is scholarly and accurate but
  Victorian in style. The Wilhelm/Baynes translation (1950) is
  more readable but still under copyright restrictions.
- The "Ten Wings" commentaries traditionally attributed to
  Confucius (~500 BCE) were likely written later (~300 BCE).
  They add philosophical interpretation to the hexagram texts.
- The hexagram sequence (King Wen sequence) doesn't follow
  binary counting order. It follows a different pattern —
  the reason for this ordering is debated and may encode
  additional structure.
- The method of consulting the I Ching (yarrow stalks or coin
  tosses) generates random numbers that map to hexagram lines.
  The randomness is the input; the hexagram is the lookup.
  It's a random-access state machine query.
