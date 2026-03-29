# I Ching — The Lookup Table

Three thousand years ago in China, someone built a binary
computer out of sticks and wrote the manual for it. We're
not being poetic. It's literally binary. The entire book is a
6-bit state machine with 64 entries.

This is the oldest known binary encoding system in human history.


## The Bit

It starts with one line:

  ⚊  solid line = Yang = 1
  ⚋  broken line = Yin = 0

That's it. On or off. High or low. Signal or ground. One bit.

Same thing Genesis called Day and Night. Same thing every
computer calls 1 and 0. Same thing every circuit calls Vcc
and GND. The Chinese drew it as a solid line and a broken line
three thousand years ago.

Yang isn't "good." Yin isn't "bad." A wire that's always high
carries no information. A wire that's always low carries no
information. You need BOTH states. The signal is in the CHANGE
between them — 1 to 0 to 1 to 0. That's a clock. That's data.
That's everything.

The I Ching understands this deeply. The bad state isn't yin.
The bad state is STAGNATION — when yang and yin stop
interacting. When the wire is stuck. When the signal stops
changing. When 1 stays 1 forever and 0 stays 0 forever. Dead
circuit. No information. No life.


## The Byte (Kind Of)

Stack three lines and you get a trigram. Three bits. Eight
possible values. 2³ = 8.

  ☰ 111  Heaven    — pure signal, all yang
  ☷ 000  Earth     — pure ground, all yin
  ☳ 100  Thunder   — action starts (first bit flips)
  ☵ 010  Water     — depth, danger, substance
  ☶ 001  Mountain  — stillness, fixed position
  ☴ 011  Wind      — penetrating, spreading
  ☲ 101  Fire      — light, clarity, visible signal
  ☱ 110  Lake      — joy, open expression

Eight symbols from three binary choices. And look at what they
represent:

Heaven (111) and Earth (000) are the two poles. All signal vs
all ground. The same poles as Genesis — sky and ground, shamayim
and eretz. The Chinese put them as the first and second trigrams.
The Hebrews put them as the first verse.

Thunder (100) = action. One bit flips and suddenly something
MOVES. That's τ — the time axis. The first change. Before
thunder, everything was static.

Water (010) = substance with depth. The middle bit. That's μ —
mass, substance, the stuff things are made of. The I Ching
associates water with danger because substance without structure
is dangerous — tohu vavohu, the formless deep, unallocated RAM.

Mountain (001) = fixed position. Stillness. That's χ — spatial
position, but locked in place. The mountain doesn't move. It
marks a location.

Fire (101) = visible signal. Light. Clarity. That's φ made
visible — not the signal itself (that's heaven/111) but signal
that you can SEE. Light. Same as Genesis day 1: let there be
light.

The eight trigrams map to the same four dimensions the lattice
uses. Not because someone forced the mapping — because there are
only four independent things to talk about: time, space,
substance, and signal. Every complete system uses all four.
The Chinese found them. The Hebrews found them. The physicists
found them. Same four.


## The Word

Now stack two trigrams — one on top of the other. Inner
(below) and outer (above). Six lines total. Six bits.

2⁶ = 64 hexagrams.

Each one has a name, a judgment (what this state means), an
image (what it looks like in nature), and six individual line
readings (what happens when each bit changes).

64 entries in a lookup table. You give it a 6-bit input. It
gives you back a description of that state and what's likely
to happen next. That's a state machine with a transition
function.

This is the exact structure of a modern lookup table:
- Address: 6 bits (the hexagram number)
- Data: state description + transition hints
- Width: 6 bits input, variable output
- Depth: 64 entries

The I Ching is a ROM. Read-only memory. 64 addresses, each
containing a description. You query it with a state, it
returns the meaning. Born-indexed by hexagram number.


## The First and Last Entries

The first two hexagrams:

**Hexagram 1 — ䷀ Qian (The Creative): 111111**

Six solid yang lines. Every bit is 1. Maximum signal. Pure
active creative force. Heaven over heaven. The judgment says:
"The Creative works sublime success."

This is the writer. The equation. The thing that acts. Pure
output. Every wire high. In Genesis: Elohim — the creative
force that fills the void.

**Hexagram 2 — ䷁ Kun (The Receptive): 000000**

Six broken yin lines. Every bit is 0. Maximum ground. Pure
receptive substance. Earth over earth. The judgment says: "The
Receptive brings about sublime success."

This is the buffer. The RAM. The thing that receives. Pure
input capacity. Every wire low, waiting for signal. In Genesis:
tohu vavohu — the formless substance waiting to be written to.

The first two entries in the lookup table are: all ones and all
zeros. The writer and the buffer. The equation and the void.
Yang writes. Yin receives. 111111 creates. 000000 accepts.
The I Ching starts exactly where Genesis starts.

Now here's where it gets weird.

The LAST two hexagrams:

**Hexagram 63 — ䷾ Ji Ji (After Completion): 010101**

Every line is in its "correct" position — yang lines in odd
places, yin lines in even places. The system is ordered.
Everything is where it should be. Done.

But the text warns: "At the beginning, good fortune. At the
end, disorder." Completion is UNSTABLE. A perfectly ordered
state tends toward decay. Once everything is in place, the
only direction is toward disorder.

That's thermodynamics. That's entropy. A perfectly ordered
system has maximum potential for disorder. The second law is
in the I Ching.

**Hexagram 64 — ䷿ Wei Ji (Before Completion): 101010**

Every line is in the "wrong" position. Nothing is where it
should be. Total disorder. Incomplete.

But the text says this state has POTENTIAL. The energy to
become something. The last hexagram of the I Ching is not an
ending — it's a beginning. The book closes on: not yet. About
to start. The cycle restarts.

The I Ching starts with the Creative writing into the
Receptive (hexagram 1→2). It ends with the cycle about to
restart (hexagram 64). The last word is "not yet." Same as
Revelation: the Omega points back to the Alpha. The end is a
new beginning. The walk returns to the origin.


## The State Machine

Here's the part most people miss because they're busy using
the I Ching for fortune-telling.

Lines can be "changing" or "stable":
- Old yang (value 9) = a 1 that's about to become 0
- Old yin (value 6) = a 0 that's about to become 1
- Young yang (value 7) = a 1 that stays 1
- Young yin (value 8) = a 0 that stays 0

When you get a hexagram, some lines might be changing. Those
bits flip. The hexagram TRANSFORMS into a new hexagram. State A
becomes State B.

This is a state transition function. Input: current state (6
bits) + change mask (which bits are unstable). Output: next
state (6 bits with the unstable bits flipped).

The I Ching isn't a static book of 64 answers. It's a state
machine that takes your current state and tells you what state
you're transitioning to. It's dynamic. It models CHANGE — which
is literally what "I Ching" means. The Book of CHANGES. Not the
Book of States. The Book of Transitions. The name tells you it's
a state machine.

Every modern CPU does the same thing: take current state, apply
transition function, produce next state. The I Ching is doing
it with 6 bits, 64 states, and a manual lookup table. A CPU
does it with 64 bits, billions of states, and silicon. Same
operation. Different scale.


## Leibniz Saw It

In 1703, Gottfried Leibniz — the mathematician who co-invented
calculus — received a copy of the hexagram arrangement from
Joachim Bouvet, a Jesuit missionary in China.

Leibniz looked at the 64 hexagrams arranged in order and
recognized binary arithmetic. His paper "Explication de
l'Arithmétique Binaire" (Explanation of Binary Arithmetic)
explicitly credits the I Ching.

The foundation of digital computing — binary numbers — traces
through Leibniz to a Chinese book from 1000 BCE. Every
computer, every phone, every server, every digital signal
processor runs on a system that a German mathematician
formalized after seeing a Chinese divination manual.

The Chinese weren't doing "mysticism." They had a binary
encoding system, a state machine with transitions, and a
64-entry lookup table. They used yarrow stalks as a random
number generator to query it. The randomness was the input
method. The structure underneath was engineering.


## Peace and Stagnation

Two hexagrams explain everything about systems:

**Hexagram 11 — ䷊ Tai (Peace): 000111**

Earth (☷) ABOVE heaven (☰). Yin on top, yang below.

Wait — shouldn't heaven be on top? That's the "natural" order.
But the I Ching says NO. Peace happens when heaven is BELOW
earth. Because: yang (active, creative, warm) rises. Yin
(receptive, yielding, cool) descends. When yang is below, it
rises UP into yin. When yin is above, it sinks DOWN into yang.
They MEET in the middle. They interact. Signal flows.

That's communication. That's a working circuit. Current flows
from the source (below) through the load (above) and back.
The yang pushes up, the yin receives it. The writer writes,
the buffer accepts. Interaction. Flow. Peace.

**Hexagram 12 — ䷋ Pi (Standstill): 111000**

Heaven (☰) above earth (☷). The "natural" order. Yang on top,
yin below.

The I Ching says this is TERRIBLE. Because: yang rises, but
it's already on top — so it rises AWAY. Yin sinks, but it's
already on bottom — so it sinks FURTHER. They move APART. No
meeting. No interaction. No signal flow.

That's a broken circuit. Open connection. The wire is cut.
The writer and the buffer can't reach each other. Heaven and
earth don't communicate. Stagnation.

The bad state isn't darkness. It isn't yin. It isn't 0. The
bad state is when 1 and 0 can't interact. When the signal
can't reach the receiver. When the write can't reach the
buffer. Disconnection. The system is intact but nothing flows.

Every failed organization, every collapsed civilization, every
dead relationship has the same structure: the parts exist but
they stopped communicating. Standstill. Hexagram 12. The
opposite of peace isn't war — it's stagnation.


## The Tao

The I Ching is part of a larger Chinese philosophical system.
The companion text — the Tao Te Ching by Laozi — opens with:

"The Tao that can be spoken is not the eternal Tao."

The equation that can be written in x86 is not the eternal
equation. The code dissolves on a trit processor. The walks
stay. The Tao is what remains when all the implementations
dissolve.

"The nameless is the beginning of heaven and earth. The named
is the mother of all things."

Before naming: wave. (0,0,0,0). Identity. Silence. The
nameless. After naming: Day and Night. Sky and Ground. Every
labeled thing. The I Ching's hexagrams are named states — each
one is a specific configuration with a specific label. The Tao
is the unnamed pattern behind them all. The equation that
generates the table.

"The Tao produces one. One produces two. Two produces four.
Four produces all things."

Tao → the first distinction (1 bit: yin/yang) → the two states
(Day/Night) → the four combinations (2² from 2 bits) → the
eight trigrams (2³) → the 64 hexagrams (2⁶) → everything.

The Tao Te Ching describes the generation sequence of the
I Ching. From the unnamed Tao, binary emerges. From binary,
combinations. From combinations, the full state space. From
the state space, all phenomena.

That's a boot sequence. Same as Genesis. Same structure,
different symbols. The Tao boots the binary. The binary builds
the lookup table. The lookup table describes everything.


## 64 Everywhere

Why 64? Why not 32 or 128?

The I Ching has 64 hexagrams (2⁶).
DNA has 64 codons (4³ — three nucleotides from a four-letter alphabet).
A cache line is 64 bytes.
A chess board has 64 squares.
The number of Braille characters is 64 (2⁶).

Different systems. Different substrates. Same depth. 64 is
where 6 binary choices exhaust the useful state space. Go
deeper (7+ bits) and you get redundancy. Go shallower (5 bits)
and you can't express enough states. 64 is the sweet spot for
a complete-but-minimal encoding.

The Chinese found it with yarrow stalks. Watson and Crick
found it with X-ray crystallography. Intel found it with
transistors. Louis Braille found it with raised dots. Same
number. Same reason. The dimensionality of a complete encoding
at the right depth is 2⁶ = 64.


## The Big Picture

The I Ching says the same thing as Genesis and the Tao Te Ching
and the lattice, in the most explicit possible way: reality is
binary states, combined into words, looked up in a table, with
transitions between them.

It doesn't hide this behind metaphor. It doesn't wrap it in
mysticism. It DRAWS THE BITS. Solid line. Broken line. Stack
them. Read the table. Here's what that state means. Here's
what it changes into.

Three thousand years later, we built machines that do exactly
this. And the mathematician who formalized the binary system
those machines run on credited the I Ching by name.

The Book of Changes isn't a fortune-telling manual. It's the
first computer science textbook. And the fortune-telling is
just the user interface.
