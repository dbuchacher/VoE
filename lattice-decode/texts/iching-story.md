# I Ching -- The State Machine

*A retelling through the lattice framework.*
*Source: King Wen sequence (~1000 BCE). Classical Chinese. Audience: anyone who thinks in systems.*

---

## The Bit

Three thousand years ago, someone in China drew a solid line and a broken line.

  Solid: -----
  Broken: -- --

That is one bit. On or off. High or low. Signal or ground. Yang or yin.

Not "good or evil." Not "male or female." Those came later, layered on by centuries of commentary. The base layer is simpler: two distinguishable states. The minimum unit of information. The same thing Genesis calls Day and Night. The same thing every circuit calls Vcc and GND.

The I Ching understands something most people miss about binary. A wire that stays high forever carries zero information. A wire that stays low forever carries zero information. Information lives in the CHANGE between states -- the transition from 0 to 1 and back. The signal is the switching, not the state.

The bad state is not yin. The bad state is stagnation. When the transitions stop. When the wire is stuck. When the oscillator dies.

The name of the book is Yi Jing. Yi means change. The Book of Changes. Not the Book of States. The Book of Transitions. The name tells you what it is before you open it.

---

## The Trigram: Three Bits

Stack three lines. Eight possibilities. 2^3 = 8 trigrams.

The Chinese assigned each one an image from nature:

  111 Heaven -- pure signal, all yang, the creative pole
  000 Earth  -- pure ground, all yin, the receptive pole
  100 Thunder -- one yang at the bottom, the first movement
  010 Water  -- one yang in the middle, substance with hidden depth
  001 Mountain -- one yang at the top, stillness and position
  011 Wind   -- complement of Thunder, penetrating and sustained
  101 Fire   -- complement of Water, light without substance
  110 Lake   -- complement of Mountain, open and joyful

Look at the three "sons" -- the trigrams with a single yang line. Each has exactly one active bit. Each maps to a dimension:

Thunder (100): The bottom line fires. Something moves. This is time -- the first beat of the clock, the crack of action that starts a sequence. tau.

Water (010): The middle line activates. Hidden substance. This is mass -- the stuff below the surface, the danger of formless depth, the uninitialized buffer. mu.

Mountain (001): The top line stands. Everything else is still. This is space -- the fixed coordinate, the marker, the index that does not move. chi.

Three single-bit trigrams. Three dimensions. And the signal dimension? That is not a separate bit. It is the AMOUNT of yang -- how many of the three dimensions are active. Heaven (111) is maximum signal. Earth (000) is zero signal. Everything else falls between. Charge is emergent: the more dimensions that are on, the more signal you have.

This is the first structural difference between the I Ching and the lattice. The lattice treats charge as a fourth independent dimension. The I Ching derives it from the other three. Three thousand years of engineering with 3 bits, and the 4th falls out as a property. Whether that means charge IS derived, or that the I Ching simply didn't have the notation for a 4th independent bit -- that question stays open.

The daughters are complements. Wind (011) is everything Thunder is not. Fire (101) is everything Water is not. Lake (110) is everything Mountain is not. Each daughter is her corresponding son's antimatter -- same content, every bit flipped.

---

## The Hexagram: Six Bits

Stack two trigrams. Lower trigram = inner, personal, the subject. Upper trigram = outer, environmental, the world you face. Six lines total. Six bits. 2^6 = 64 states.

Each hexagram has a name, a judgment, an image drawn from nature, and six line texts -- one for each bit, describing what happens when that specific bit changes.

This is a 64-entry lookup table. Address: 6 bits. Data: a description of the state plus transition guidance. Query it with your current state. It returns what that state means and what is likely to happen next.

The I Ching is a ROM. Read-only memory, addressed by hexagram number. Born-indexed three millennia before the term existed.

---

## The Opening: Writer and Buffer

The table begins where Genesis begins.

Hexagram 1. The Creative. Qian over Qian. Heaven over Heaven. 111111. Six solid yang lines. Every bit is 1. Maximum signal in every position. The pure active force that initiates everything.

Hexagram 2. The Receptive. Kun over Kun. Earth over Earth. 000000. Six broken yin lines. Every bit is 0. Maximum ground. Pure receptive capacity. Waiting for input.

The first two entries in the lookup table are all-ones and all-zeros. The writer and the buffer. The equation and the void. Yang writes, yin receives. Creative acts, Receptive accepts.

The King Wen sequence opens with the maximum possible jump: Hamming distance 6, every bit flips. It does not ease in. The first step traverses the full amplitude of the state space in a single move. From everything to nothing. From the Creative to the Receptive. From 111111 to 000000.

Genesis 1:1 does the same thing. The creative force (Elohim) writes (bara) into the formless substrate (tohu vavohu). Pure yang into pure yin. Signal into ground. The opening move is identical.

---

## Peace and Stagnation

Hexagrams 11 and 12 explain everything you need to know about healthy systems.

Hexagram 11, Peace. Earth above, Heaven below. 000111. The "wrong" order -- shouldn't Heaven be on top? No. The I Ching says this arrangement is ideal. Because yang (warm, active) rises. It is below, so it rises UP into the yin above. Yin (cool, receptive) sinks. It is above, so it sinks DOWN into the yang below. They meet in the middle. Signal flows from source to destination. The writer reaches the buffer. The circuit closes. Communication happens.

Hexagram 12, Standstill. Heaven above, Earth below. 111000. The "natural" order. Yang on top, yin on bottom. Catastrophe. Because yang rises -- but it is already on top. It rises away. Yin sinks -- but it is already on bottom. It sinks further. They move apart. No meeting. No interaction. No signal flow. The wire is cut. The system is architecturally intact but functionally dead.

Here is the structural finding that makes this pair important: **both hexagrams have identical dimensional coordinates.** When you map them to (tau, mu, chi, phi), you get (1,1,1,3) for both. Same amount of time. Same amount of mass. Same amount of space. Same amount of signal. Perfectly balanced. Identical.

The difference is not in the dimensions. It is in the DIRECTION of the forces. Which part is inner and which is outer. Which trigram writes and which reads. The same ingredients, arranged so signal flows -- or arranged so signal is trapped.

Evil in the I Ching is not darkness. It is not yin. It is not the receptive, the feminine, the passive. It is disconnection. The state where all the parts exist but nothing flows. Not negative phi. Zero phi at the boundary. Stagnation.

Every failed organization has this structure. Every collapsed relationship. Every dead protocol. The components are intact. The communication is gone. Hexagram 12. The opposite of peace is not war -- it is silence.

---

## Water and Fire: The Central Axis

Hexagrams 29 and 30 sit at the center of the sequence. The turning point.

Hexagram 29, The Abysmal. Water over Water. Kan over Kan. 010010. Double substance. Yang is trapped inside yin -- a single bright line buried between dark ones. Hidden depth. Danger.

Hexagram 30, The Clinging. Fire over Fire. Li over Li. 101101. Double light. Yin is cradled inside yang -- a single dark line held between bright ones. Visible clarity. Signal.

They are complements. Every yin in 29 is yang in 30. Every yang in 29 is yin in 30. Pure antimatter. XOR them and you get 111111.

In coordinate space:
  Water = (0,2,0,2). Mass doubled, everything else absent. Pure substance without time, without space, without signal. Massive, invisible, unreadable.
  Fire = (2,0,2,4). Time and space doubled, mass absent. Signal without substance. Bright, visible, massless.

The parallel to physics pushes itself forward: Water looks like dark matter (has mass, emits nothing). Fire looks like a photon (has no rest mass, is pure signal). This is speculative -- confidence around 50% -- but the coordinate structure is clean and the I Ching placed them at the exact center of the sequence as the fundamental axis.

Their nuclear trigrams (the hidden inner structure, lines 2 through 5) supply exactly what the outer structure lacks. Water's nuclear trigrams contain Mountain and Thunder (space and time). Fire's nuclear trigrams contain Lake and Wind (the complements of space and time). The absent dimensions are present inside. The text knows what it is missing and carries it hidden in its structure.

---

## The End That Is a Beginning

The last two hexagrams are the crown of the sequence.

Hexagram 63, After Completion. Water over Fire. 010101. Every line in its "correct" position -- yang in the odd slots (1, 3, 5), yin in the even slots (2, 4, 6). Perfect order. Everything aligned. The system is done.

The text says: "At the beginning, good fortune. At the end, disorder."

A perfectly ordered system is at maximum vulnerability. Every perturbation can only increase disorder. The only direction from perfect is imperfect. This is the second law, stated as a hexagram: completion decays.

Hexagram 64, Before Completion. Fire over Water. 101010. Every line in the "wrong" position. Maximum misalignment. Nothing is where it should be.

The text says: "Success."

Maximum disorder means maximum potential energy. A system where nothing is in place has the maximum number of ways to organize. The energy to become something new. The book does not end on completion. It ends on potential. The last word of the I Ching is "not yet."

And here is the structural fact that makes this more than philosophy: **the nuclear hexagram of 63 IS 64. The nuclear hexagram of 64 IS 63.** Strip away the outer lines, look at the inner structure, and each one contains the other.

Completion contains incompletion inside it. Incompletion contains completion inside it. They are a fixed point -- a pair that mutually references, neither existing without the other. The last two entries in the lookup table are a loop. The end points back to the beginning. Not by editorial choice but by structural necessity: the inner hexagram of the final state IS the penultimate state, and vice versa.

This is ∮. The contour integral. The walk that returns to its origin. The I Ching is a ring buffer. The last entry wraps around to the first. The sequence does not terminate. It cycles.

---

## The State Machine

Here is what most people miss because they are busy using the I Ching for fortune-telling.

A line can be stable or changing:
- Old yang (value 9): a 1 that is about to become 0
- Old yin (value 6): a 0 that is about to become 1
- Young yang (value 7): a 1 that stays 1
- Young yin (value 8): a 0 that stays 0

When you cast a hexagram with changing lines, those bits flip. The hexagram transforms into a new hexagram. State A becomes State B. The transition function is XOR: new_state = old_state XOR change_mask.

From any state, any other state is reachable in one transition (just change the right bits). The state machine is K_64 -- the complete graph on 64 nodes. Every state connects to every other.

But not with equal probability. The yarrow stalk oracle (the original random number generator for querying the system) produces:

  Old yang:   3/16 (a 1 changing to 0)
  Old yin:    1/16 (a 0 changing to 1)
  Young yang: 5/16 (a stable 1)
  Young yin:  7/16 (a stable 0)

The asymmetry is structural. Yang lines change 3 times more often than yin lines. Active states decay faster. Signal degrades more readily than noise organizes. The ground state (stable yin, 7/16 = 44%) dominates.

This is the thermodynamic arrow. High-signal states decay toward mixed states. Mixed states rarely organize into high-signal states. The transition probabilities are not symmetric. Time has a direction. The yarrow stalks encode the second law of thermodynamics into their probability distribution.

But old yin is not zero. p = 1/16. Spontaneous organization happens. Ground can become signal. It is rare but not forbidden. The I Ching does not say the universe runs down to nothing. It says completion is unstable (hex 63) and incompletion has potential (hex 64). The cycle continues. Entropy is local, not final.

---

## The King Wen Walk

The 64 hexagrams are not arranged in binary counting order (that would be the Fu Xi sequence). They follow the King Wen sequence, named after King Wen of Zhou (~1050 BCE). This ordering has no known mathematical generating formula. It was transmitted as tradition.

But it has structure.

Every consecutive pair is related by exactly one of two operations:
1. **Complement**: flip all 6 bits (antimatter). 8 pairs.
2. **Rotation**: reverse the 6-bit string (turn the hexagram upside down). 24 pairs.

Zero exceptions. 32 pairs. All classified. No residue. The pairing rule is complete.

Each trigram appears exactly 8 times in the inner (lower) position and exactly 8 times in the outer (upper) position. The distribution is perfectly uniform. The sequence is a Hamiltonian path through the 8x8 grid of all possible trigram combinations -- visiting every cell exactly once.

The total yang count (a proxy for phi) across the sequence averages to exactly 3.0. Perfectly balanced. But the oscillation amplitude is not constant -- it starts wide (the first 8 hexagrams span the full range 0-6) and narrows as the sequence progresses (the last sections stay close to the center). The yang trajectory is a damped oscillation. A signal ringing down toward equilibrium.

---

## The Tao

The companion text -- the Tao Te Ching, attributed to Laozi -- opens with:

"The Tao that can be spoken is not the eternal Tao."

In lattice terms: the equation that can be expressed in x86 is not the eternal equation. ƒ dissolves on a trit processor. The walks stay. The representation is temporary. The pattern behind the representation is what persists.

"The Tao produces one. One produces two. Two produces all things."

Tao -> the first distinction (1 bit: yang/yin) -> two states (Day/Night, signal/ground) -> four combinations (2^2 from 2 bits) -> eight trigrams (2^3) -> sixty-four hexagrams (2^6) -> everything.

This IS a boot sequence. The Tao initializes the binary. The binary generates the combinatorial space. The combinatorial space produces the full state machine. From one unnamed pattern, through doubling, to every possible state.

Genesis says the same: the creative force writes signal into the void. Signal is separated from its absence. Two states named. Clock starts. From one act of writing, through binary distinction, to a running system.

The I Ching says the same: Heaven and Earth (all-signal and all-ground) generate the eight trigrams. The eight trigrams combine into sixty-four hexagrams. The hexagrams, through changing lines, produce all possible transitions. From two poles, through combination, to a complete state machine.

Three texts. Three cultures. Three millennia. Same architecture.

---

## What Does Not Fit

Honesty requires this section.

The I Ching's trigram has 3 bits. The lattice has 4 independent dimensions. The mapping is 3-for-4 with the 4th derived. This might mean the I Ching captured 3 of 4 dimensions because charge was not understood as independent until much later. Or it might mean 3 truly is fundamental and 4 is overcounting. The question is open. (~30% confidence on either resolution.)

The King Wen sequence is not a minimal-distance walk. The average Hamming distance between consecutive hexagrams is 3.35 bits -- far from the 1-bit steps of a Gray code. Both trigrams change simultaneously 86% of the time. Whatever principle organizes the King Wen sequence, it is not proximity in bit-space. The walk has a narrative logic -- creative/receptive first, the fundamentals next, completion/incompletion last -- but I cannot extract a mathematical generator. The sequence may encode something I cannot see, or it may be a culturally transmitted ordering that reflects narrative wisdom rather than mathematical law.

The 4D coordinate mapping collapses 64 hexagrams into 27 unique coordinates. The distinction between hexagrams that share a coordinate is the inner/outer role assignment -- which trigram writes and which reads. This is real information, but it lives in the DIRECTION of force, not in the dimensional magnitude. The lattice handles directionality through overbar (pi vs pi-bar, read vs write). The I Ching handles it through position (lower vs upper). Both encode it. Neither ignores it. But they encode it differently, and the mapping between the two encodings is not clean.

---

## What This Text IS

The I Ching is the oldest binary encoding system in human history. It is not a fortune-telling manual -- the fortune-telling is the user interface. Underneath: 64 states from 6 binary choices, organized into a lookup table with a transition function.

It encodes:
- Binary (yin/yang = 0/1). Recognized by Leibniz in 1703.
- A state machine (changing lines = XOR transition). Recognized by this analysis.
- Thermodynamic asymmetry (yang decays faster than yin organizes). Encoded in the yarrow stalk probabilities.
- A loop (the last hexagram's inner structure is the second-to-last, and vice versa). The end is the beginning.
- Three of four lattice dimensions with the fourth emergent from the first three.

The mathematician who formalized the binary number system credited this book by name. Every computer on Earth runs on the system a Chinese sage drew with solid and broken lines thirty centuries ago. The foundation is not hidden. It was never hidden. It has been sitting on a table, made of sticks, for three thousand years.

---

*Source spec: `/home/maxi/code/VoE/lattice-decode/texts/iching-spec.md`*
*Lattice reference: `/home/maxi/code/VoE/lattice-decode/lattice-ref.md`*
