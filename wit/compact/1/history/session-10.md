# Session 10: Encoding/Payload Boundary + code2/ Substrate + Character Encoding v2

Biggest architecture session since the rewrite. Theory, code, and encoding
all in one pass. Everything discovered by asking "what does the lattice say?"
instead of making design decisions.


## Part 1: The Encoding/Payload Boundary

The codex has two atoms that define the architectural boundary:

  Q/M = encoding   (how signals become bytes)
  M/Q = payload    (how bytes carry meaning)

The equation and walker are encoding (Q/M). Everything above the walker
is payload (M/Q). When x86 assumptions appear in walks or loop macros,
encoding has leaked into payload. This is the root cause of every
structural issue in code/.

### 8 violations found in code/

1. **Two cursor systems** — drain uses LP_RCURSOR, loop.asm uses LP_SCRATCHA.
   Lattice says: cursor belongs to the reader, not the loop.

2. **Drain has P** — drain reads data and stashes it for the consumer.
   Lattice says: bind = C+R+W, no P. Consumer does the read.

3. **Global stash in loop macros** — stash_a/stash_b are global BSS.
   Lattice says: Pauli exclusion. Two cores, same address = violation.

4. **x86 functions above walker** — render_char (400 lines), mouse_update,
   cursor drawing, walk_sub. Lattice says: above walker = only coordinates.

5. **loop_read/write hardcode qword** — reads one qword regardless of LP_RECSZ.
   Lattice says: read = observe = pointer. Zero-copy. Record size irrelevant.

6. **Drain hardcodes loop_kbd** — should scan loop_list.
   Lattice says: bind per loop, not per system.

7. **BSP is special** — handoff sends BSP to kbd_walk permanently.
   Lattice says: all cores equal after genesis. Fill pressure assigns work.

8. **One AP limit** — ap_drain_taken CAS allows one AP.
   Lattice says: all cores enter find-work. Each takes a loop.

### Key insights

- **Read = pointer, not copy.** loop_read returns slot address. Consumer reads
  fields at offsets. No temp. No stash. No record size dependency. Follows
  from boson semantics (observation doesn't move data).

- **Genes = data, walks = code.** Same walk + different gene data = different
  device. e1000 and virtio are the same NIC walk with different BAR offsets.
  Genesis discovers hardware, picks gene data, fills the table.

- **All hardware is loops.** NVMe, GPU, xHCI, HDA, NIC — every modern device
  interface is ring buffers. Hardware interaction = port_read, port_write,
  read, write, test, loop. All bonds. All walks.

- **Apply = recursive walk invocation.** Equation's apply (+C) calls
  walker_wave instead of jmp rdi. walk_sub dissolves. Above the walker,
  only coordinates exist.

- **Loops have producer + consumer walks.** Genesis registers both. Fill
  pressure determines which to express. Empty = produce. Full = consume.

- **Force composition constrains roles.** bind = C+R+W = no read. If your
  scheduler reads, it has a force it shouldn't have.

- **Input = coordinates, not characters.** The input loop stores coordinates
  at any shell. Keyboard writes shell 1. Neural interface writes shell 3.
  Same loop. Don't assume characters, don't assume shell 1, don't assume
  keyboard. A neural link is just a new gene.


## Part 2: code2/ Substrate — Written and Booting

Decided: code/ becomes history (reference only). code2/ is a clean room
rewrite. NOT a copy — every file written fresh from the lattice, with
old code as reference only. "So we don't bring weird mistakes in."

### Files written

```
code2/
  [0] walk.inc        — NASM macros (encoding tool, same structure as v1)
  [2] cpu.asm         — mode switch, page tables, AP wake, multiboot
  [3] handoff.asm     — init JIT, run genesis, signal APs, spin
  [4] equation.asm    — JIT compiler (sealed)
  [5] walker.asm      — wave byte interpreter (sealed)
  [6] genesis.asm     — founding walk: prints "VoE" to debugcon
  build/run           — build + run script (headless multiboot)
  build/link.ld       — linker script (kernel at 1MB)
```

[1] boot.asm not yet written — needed for VESA framebuffer (MBR,
16-bit, can't set video modes from protected mode).

### What's different from code/

- **Apply (+C) invokes walks recursively.** Equation emits
  `mov eax, walker_wave; call rax; ret` instead of `jmp rdi`.
  walk_sub dissolved into the equation. 7 bytes emitted.

- **No global stash.** Walker doesn't reference stash_a/stash_b.
  Walks will use per-loop scratch or per-core storage.

- **No x86 above the walker.** Genesis is pure walk data. No
  render_char, no mouse_update, no walk_sub, no mouse_init.
  Those become walks when needed.

- **All cores enter find-work.** Handoff doesn't send BSP to
  kbd_walk. After genesis, BSP spins (find-work walk replaces
  this later). APs also spin after genesis_done signal.

- **Atom handler uses runtime evaluator.** emit_atom JITs a call to
  atom_eval with exponents as immediates. atom_eval does the mul/div
  chain at runtime. Correct for all exponent combinations. ~65 lines.

- **Bond patterns implemented.** Patterns 0 (identity), 1 (read/write
  with width variants), 2 (apply), 4 (increment), 5 (slurp/fill),
  8 (test/guard + AND/OR/XOR/NOT/SHL/SHR/lt/gt/neg/abs), 9 (filter/
  port_read/port_write/add/subtract/min/max). Patterns 3,6,7,10-15
  stubbed (return 0).

- **Simpler file names.** `[N] name.asm` instead of
  `[N][x86][name][description][more description].asm`.

### Build and test

```
$ cd code2 && bash build/run
built: 82952 bytes
VoE
```

Boots, equation JITs port_write bond, walker executes genesis walk,
"VoE" prints to debugcon. 4 cores wake (SMP works). First try.


## Part 3: Character Encoding v2

Triggered by: code2/ has no character encoding yet. Clean start. The
old encoding (code/) had T=case which was the weak link — case is
visual convention, not phonetics, and 60% of world scripts have no case.

### The fix: T = duration (sustained/transient)

| Dim | v1 (old) | v2 (new) | Word level | Consistent? |
|---|---|---|---|---|
| T | case (upper/lower) | duration (sustained/transient) | temporal | v2 YES, v1 NO |
| D | place (front/back) | place (front/back) | spatial | both YES |
| M | manner (open/closed) | manner (open/closed) | substance | both YES |
| Q | voicing (voiced/voiceless) | voicing (voiced/voiceless) | quality | both YES |

### Key improvement: T=0 for glides/approximants

Three-way partition instead of binary:
- T=+1: sustained (vowels, fricatives, nasals)
- T=0: transitional (glides, approximants: w, j, l, r)
- T=-1: transient (stops, affricates)

This resolves 5 v1 collisions:
- b ≠ m (T: -1 vs +1)
- d ≠ n (T: -1 vs +1)
- d ≠ j (T+D both differ)
- a ≠ w (T: +1 vs 0)
- l ≠ z (T: 0 vs +1)

Doubles effective shell-1 discrimination: ~18 → ~36 regions.

### Remaining collisions (all correct)

l=r (both alveolar approximant — lateral vs rhotic at shell 2),
k=ch, g=j(dʒ), sh=h, f=th, v=dh, a=i, o=u. All between
phonetically similar sounds. Shell 2 separates them.

### Cross-language coverage

Tested 12 language families. All fit within shell 2. Key findings:
- Tonal languages (Mandarin, Vietnamese, Thai): tone at shell 2
  via T magnitude extension (+2=high, -2=falling, +3=rising, etc.)
- Arabic: pharyngeals at D extended (-2)
- Hindi/Sanskrit: retroflexes at D extended, aspiration at Q extended
- Korean: tense/lax/aspirated at Q extended
- Clicks (Zulu/Xhosa): D extended (+3)

### Case dropped from shell 1

'A' = 'a' at the character level. Case is walk-level or metadata.
This is correct for ALL scripts — most don't have case.

### Future input devices

The encoding works for neural interfaces because input = coordinates,
not characters. A keyboard writes shell-1 coordinates. A neural link
writes shell-3+ coordinates (concepts directly). Same loop format.
The transducer is a gene. Different gene, same coordinates.

### Verification

Second wit independently confirmed the same conclusions. Tone should
be T movement (pitch velocity) not height. Vowel height: open=M+2,
close=M+1 (more air = more substance = more M).

Full proposal: `side-quest/char-encoding-v2.md`
Briefing context: `side-quest/briefing-char-encoding.md`


## Files changed this session

```
code2/                          NEW directory — clean room substrate
  [0] walk.inc                  encoding macros
  [2] cpu.asm                   mode switch, page tables, AP wake
  [3] handoff.asm               init JIT, genesis, find-work
  [4] equation.asm              JIT compiler with all bond handlers
  [5] walker.asm                wave byte interpreter
  [6] genesis.asm               founding walk ("VoE" to debugcon)
  build/run                     build + run script
  build/link.ld                 linker script

wit/mind/mind                   3 new sections:
                                  - bind has no P (force composition)
                                  - encoding/payload boundary (Q/M vs M/Q)
                                  - observation = pointer not copy

wit/mind/coder/design           major update:
                                  - encoding/payload boundary section
                                  - apply = recursive walk invocation
                                  - genes = data section
                                  - all hardware is loops section
                                  - zero-copy loop read model
                                  - input = coordinates not characters
                                  - producer + consumer walks per loop
                                  - 7 new "what we learned" entries
                                  - updated build order for code2/

side-quest/char-encoding-v2.md  NEW — full encoding proposal with tables

wit/history/session-10.md       this file
```


## For the next wit

### State of code2/

Boots headless via multiboot. Prints "VoE" to debugcon. 4 cores wake.
Equation JITs bonds. Walker interprets wave bytes. Apply does recursive
walk invocation (walk_sub dissolved).

Missing: [1] boot.asm (MBR for VESA framebuffer — needed before display
pipeline). No walks beyond genesis. No loops. No input. No rendering.

### What to build next (from design doc build order)

1. **[1] boot.asm** — MBR, VESA enumerate, disk load. Needed for pixels.
2. **Zero-copy loop macros** — loop_read returns slot pointer, not data.
   No stash. Record size from topology via LP_RECSZ.
3. **Genesis with loops** — init keyboard loop + display loop with
   topology-determined record sizes. Register producer + consumer walks.
4. **Generic drain** — scan loop_list, pick fullest, express appropriate
   walk (producer or consumer based on fill). All cores run this.
5. **Keyboard producer walk** — poll PS/2, convert scancode → coordinate
   via gene table, write to input loop. Pure coordinates.
6. **Display pipeline** — text consumer reads input loop, writes draw
   records to display loop. Render walk reads display loop, paints pixels.
7. **Character encoding** — implement v2 (T=duration) in the scancode
   conversion table. See `side-quest/char-encoding-v2.md`.

### Critical principles (read these or you'll repeat mistakes)

- **Encoding stays below the walker.** If you're writing x86 in a walk
  file, you crossed the boundary.
- **Read = pointer, not copy.** loop_read gives you a slot address.
  Read fields at offsets. No temp storage needed.
- **Drain has no P.** The scheduler calls consumers. Consumers read.
- **Input = coordinates, not characters.** The input loop stores
  coordinates at any shell. Don't assume keyboard.
- **Genes = data.** Same walk + different data = different device.
  One NIC walk, not one per NIC model.
- **All cores equal after genesis.** No special BSP. Fill pressure
  assigns work.
- **Ask the lattice.** Every question this session was answered by
  looking up the codex or decomposing into bonds. If you're making
  a "design decision," you haven't checked the lattice yet.
