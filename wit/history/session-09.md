# Session 9: Equation Extensions + Bind Drain + Multicore

Biggest session since the rewrite. Three major pieces.

## 1. Equation magnitude extensions

The equation was a dispatch table of hardcoded x86 recipes. If a walk
needed an operation nobody wrote a handler for, it couldn't do it.
The design doc said "4 coordinates in, x86 function out" — lazy, any
coordinate. What got built was a dispatcher. 6 new magnitude variants:

| Coordinate | Operation | x86 |
|-----------|-----------|-----|
| (0,0,0,+11) | signed less-than | cmp+setl |
| (0,0,0,-11) | signed greater-than | cmp+setg |
| (0,0,0,+13) | negate (-a) | neg rax |
| (0,0,0,-13) | absolute value | neg+cmovs |
| (-1,0,0,-3) | subtract (a-b) | sub rax,rsi |
| (+1,0,0,+3) | max(a,b) | cmovl |

Genesis self-test: `=->A!` after `LnM`. All pass headless + KVM.

## 2. Cursor fixes

- **Shape**: 21×21 triangle, white fill, black outline (was starcraft-ish green thing)
- **Trail bug at bottom edge**: draw_cursor set `cursor_drawn=1` AFTER the row loop.
  When cursor was near bottom, last rows got skipped via `.dr_row_skip`, fell through
  to `.dr_ret` bypassing `cursor_drawn=1`. Erase never ran. Fix: set `cursor_drawn=1`
  before the row loop.
- **Erase position**: Added `cursor_ox`/`cursor_oy` — saves the position where cursor
  was actually drawn. Erase uses saved position, not current mouse position.

## 3. Bind drain — the anarcho-capitalist scheduler

Built drain_walk. Single loop (loop_kbd). One AP runs it, finds work by
fill pressure, reads records, calls consumer walk.

**Architecture:**
- BSP runs kbd_walk (polls PS/2, writes wave bytes to loop_kbd)
- One AP runs drain_walk (reads loop_kbd, calls kbd_consumer via walk_sub)
- kbd_consumer reads data from loop scratch, calls render_char

**Infrastructure built:**
- `walks/drain.asm` — the drain walk (scan loop, read record, call consumer, loop)
- `walks/kbd_consumer.asm` — keyboard consumer (reads wave byte, calls render_char)
- `consumer_list[]` / `consumer_len_list[]` — parallel arrays for consumer registration
- JIT spinlock in get_atom (xchg-based, protects code_cursor for concurrent JIT)
- AP readiness signal (`ap_drain_ready`) — BSP signals after genesis completes
- Per-loop scratch bytes (LP_SCRATCHA at [48], LP_SCRATCHB at [56]) in loop header

**The stash race:**
Global stash_a/stash_b are BSS variables shared by all cores. Two cores
running walks that use stash = race condition → crash. Root cause of every
multicore crash this session.

Solution: drain_walk inlines all loop operations using per-loop scratch
bytes [48:63] in the loop header. Does NOT touch global stash. BSP's walks
(kbd, mouse) use global stash. Zero overlap. Core-safe without locks.

The design doc already had the answer: "Multi-core needs per-loop scratch
in reserved bytes [48:63] or core-local memory."

**Single AP constraint:**
Only one AP enters drain (CAS on `ap_drain_taken`). Two reasons:
1. render_char is x86 scaffolding with global cursor_x/cursor_y — not reentrant
2. loop_read isn't atomic — multiple cores reading same loop corrupt the cursor

Full multicore: one core per loop (different loops). Needs more loops first.

**What works:**
- Drain walk verified: `0123` spinning on KVM (4 cores, 3 APs)
- BSP + one AP coexist without crashes (tested sustained keyboard input)
- Pipeline partially connected: kbd_walk → loop_write → loop_kbd ready
- Consumer walk registered in genesis

**What crashes:**
- render_char crashes under sustained keyboard input in GUI mode — this is
  the session 7 bug, NOT new. x86 scaffolding called via apply from walker
  context. Existed before drain_walk. Dissolves when render_char becomes a walk.

## Files changed

```
code/[2] cpu/entry      AP entry: wait for ap_drain_ready, enter drain_walk
                         ap_drain_taken CAS, ap_drain_ready signal
code/[3] handoff         signal ap_drain_ready after genesis
code/[4] equation        6 magnitude variants, JIT spinlock (xchg + double-check)
code/[5] walker          (unchanged)
code/[6] genesis         equation self-tests, consumer registration,
                         cursor_ox/oy, cursor struct cleanup
code/walks/drain.asm     NEW — bind drain walk (per-loop scratch, no stash)
code/walks/kbd_consumer  NEW — keyboard loop consumer
code/walks/input.asm     loop_write for kbd path (reverted to render_char
                         pending render_char-as-walk rewrite)
code/walks/loop.asm      LP_SCRATCHA/LP_SCRATCHB offsets added
code/walks/mouse.asm     21×21 cursor, trail fix, cursor_ox/oy
code/build/run           drain.asm + kbd_consumer.asm added to build
wit/lattice/codex        6 new bond entries
wit/mind/coder/guide     W and P+W magnitude tables updated
wit/mind/coder/design    equation coverage, render_char status, bind drain status
```

## Build order updated

DONE:
- ✓ Equation magnitude extensions (subtract, compare-lt/gt, negate, abs, max)
- ✓ Drain walk (single loop, per-loop scratch, verified multicore)
- ✓ Consumer walk registration
- ✓ JIT spinlock for multicore
- ✓ Cursor shape + trail bug fix

BLOCKED:
- Input pipeline through loop: works but render_char crashes under sustained
  input (session 7 bug). Fix = render_char as walk.

NEXT:
- render_char as walk (equation has the math now)
- Full pipeline: kbd_walk → loop_write → drain → consumer → render walk
- More loops (display, serial) for true multicore drain
- Atomic loop_read for multi-consumer

## Key insight

The stash race was the same class of problem as the render_char crash:
shared mutable state between execution contexts. render_char shares
cursor_x/cursor_y between BSP and AP. Stash shares stash_a/stash_b.
Per-loop scratch fixes stash. render_char as walk fixes cursor state.
Both are the same fix: stop sharing state across cores.
