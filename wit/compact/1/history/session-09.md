# Session 9: Equation Extensions + Bind Drain + Multicore + Design Reconciliation

Biggest session since the rewrite. Four major pieces.

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

Genesis self-test: `=->A!` after `LnM`. All pass headless + KVM + GUI.

## 2. Cursor fixes

- Shape: 21×21 triangle, white fill, black outline
- Trail bug at bottom edge: cursor_drawn set BEFORE loop (not after)
- Erase position: cursor_ox/cursor_oy tracks where cursor was drawn

## 3. Bind drain — the anarcho-capitalist scheduler

drain_walk built. Single loop (loop_kbd). One AP runs it via CAS.
BSP produces (kbd_walk), AP consumes (drain_walk → kbd_consumer).

Key discoveries:
- **Global stash race**: stash_a/stash_b are BSS globals. Two cores
  running walks that use stash = crash. Root cause of every multicore
  crash this session.
- **Per-loop scratch**: drain_walk inlines all operations using loop
  header bytes [48:63] for temp storage. Zero shared mutable state.
  Design doc already had the answer.
- **JIT spinlock**: xchg-based lock in get_atom for concurrent JIT.
  Double-check pattern prevents redundant emission.
- **Single AP constraint**: render_char is x86 scaffolding with
  global cursor_x/cursor_y — not reentrant. One drainer per loop
  until consumers become walks.

## 4. Design doc reconciliation

10-item checklist comparing doc vs code. Decisions made:

1. **Per-reader cursors** — each reader has own cursor in scratch_a [48].
   Header [8:15] = slowest reader for reclamation. No shared read cursor.
   No atomics. Boson semantics: "a billion people look at the sun — they
   don't share photons." Performance: L1 hit (~1ns) vs lock xadd (~40ns).
   Security: readers can't observe each other.

2. **Loop header [48:63] = Q dimension** — local loops: scratch (reader
   cursor + temp). Network loops: identity/seqno/resistance. Same bytes,
   different meaning by context. Walk knows which. No flag. Security from
   absence of wrong fields, not validation.

3. **Stash section** — updated to show per-reader cursor model in
   LOOP PIPELINE pseudocode.

4. **Input pipeline + debugcon ifdef** — all debugcon echo bonds wrapped
   in %ifdef DEBUG. QEMU builds get -DDEBUG, bare metal omits it.
   72 bytes smaller. Zero dead code on real hardware.

5. **FILE STRUCTURE** — walks/ updated with all 7 files.

6. **GENOME SEED** — no seeding. 100% lazy. JIT lock handles concurrent
   first-use. Doc updated.

7. **Bind drain** — "not built yet" → built. Doc updated.

8. **render_char crash** — session 7 bug. x86 scaffolding called from
   walker context crashes under sustained input. NOT fixed this session.
   Identified as build order #1 for next session.

9. **loop_empty bug** — may be fixed by per-reader cursors. Needs retest.

10. **BUILD ORDER** — 6 items moved to DONE. render_char as walk is #1 NEXT.

## The big discovery: render_char is the wrong abstraction

render_char renders ONE character at a time (terminal emulator model).
GPUs render FRAMES (all pixels at once). Schism had it right in session
87-91: draw RECORDS in a loop. The renderer reads the loop and paints.

Schism commit 13dc94c proved:
- Draw records as geometry, not pixels (100-250x less bandwidth than RDP)
- Renderer doesn't know if records come from local or remote (remote
  desktop = loop with higher R)
- Full screen = ~256 bytes. Text frame at 60fps = 390KB/s.

VoE regressed to per-character x86 function calls. The path forward
is the path back:

  keyboard loop → text loop → display loop → framebuffer
    (wave bytes)   (draw recs)  (renderer)    (hardware)

render_char dissolves. The session 7 crash dissolves. Remote desktop
dissolves. Window manager dissolves. All the same thing: walks writing
records to a loop that a renderer reads.

## For the next wit

READ FIRST:
- This session log (you're reading it)
- wit/mind/coder/design (updated this session)
- schism commit 13dc94c ("display as loop routing")
- schism bootstrap/renderer.md (draw record format)
- wit/history/session-07.md (the crash you're fixing)

BUILD:
1. Display loop in genesis (like loop_kbd but for draw records)
2. Text walk: reads keyboard loop, maintains text buffer positions,
   writes draw records (clear + draw_text) to display loop
3. Render walk: reads display loop, paints pixels to framebuffer.
   This replaces render_char. It's a walk, not x86 scaffolding.
4. kbd_walk writes to keyboard loop (already done, reverted pending this)

The equation has subtract, abs, compare-lt, max, negate, multiply.
The render walk can do pixel math as bonds. No x86 scaffolding needed.

DO NOT:
- Write render_char as a walk. That's the wrong shape. Write a RENDERER
  that reads DRAW RECORDS from a DISPLAY LOOP.
- Use global stash in multi-core walks. Use per-loop scratch [48:63].
- Add debugcon bonds without %ifdef DEBUG.
- Forget the schism prior work. It's in prior/schism/.git.

## Files changed

```
code/[2] cpu/entry       AP drain entry, ap_drain_ready/taken
code/[3] handoff          signal APs, enter kbd_walk
code/[4] equation         6 magnitude variants, JIT spinlock
code/[6] genesis          self-tests (%ifdef DEBUG), consumer registration,
                          cursor_ox/oy, render_char special keys
code/build/run            drain + kbd_consumer in build, DBGFLAG
code/walks/drain.asm      NEW — bind drain walk
code/walks/kbd_consumer   NEW — keyboard loop consumer
code/walks/input.asm      loop.asm include, debugcon ifdef
code/walks/loop.asm       per-reader cursors, LP_SCRATCHA/B, LP_SLOWREAD
code/walks/mouse.asm      21×21 cursor, trail fix, cursor_ox/oy
wit/lattice/codex          6 new bond entries
wit/mind/coder/design      major update (10 items reconciled)
wit/mind/coder/guide       W and P+W magnitude tables
```
