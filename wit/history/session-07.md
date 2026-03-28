# Session 7: Wave Byte Input Pipeline + Latin Font + Mouse + Walk State Machine

Coder session. Big one — full input pipeline, mouse with walk-based state machine, ASCII killed.

## What was built

- **256-byte wave byte scancode table**: Replaces old 512-byte dword table.
  Key-up codes (0x80+) map to 0x00 — filtering is free. No stash,
  no bit masking, no SHL. One table does all the work.

- **10-record kbd walk**: Down from 16. No stash save/restore. The 256-byte
  table makes key-up filtering implicit. Two bonds (ADD + byte_read)
  replace eight (save, bit-test, skip, restore, mask, SHL, ADD, dword-read).

- **Wave byte render_char**: Accepts wave bytes instead of packed dwords.
  Special keys map to forces: Space=+W(0x01), Enter=+R(0x04),
  Backspace=-P(0xC0). Not arbitrary — the forces name what the keys DO.

- **Latin bitmap font (font_by_scancode)**: 2048-byte table indexed directly
  by PS/2 scancode. No ASCII middleman. Scancode in, 8×16 glyph out.
  Scaled 2× to 16×32 cells. ASCII encoding completely eliminated.

- **Backspace clears cell**: Fills the character cell with blue background
  (0x00336699) before moving cursor back.

- **PS/2 mouse support**: Mouse gene in genesis (AUX port init, reset,
  enable reporting). 3-byte packet state machine in mouse_handler.
  XOR arrow cursor on framebuffer. Bounds-checked.

- **Keyboard+mouse routing**: kbd walk checks port 0x64 bit 5 to
  distinguish keyboard from mouse bytes. Keyboard → wave byte table →
  render_char. Mouse → mouse_handler → XOR cursor.

- **PIC masking**: All IRQs masked on both PICs at boot (0xFF to 0x21
  and 0xA1). We poll, not interrupt.

## Architecture decisions

- **Wave byte is the identity, font is the display skin**: The wave byte
  table encodes phonetic coordinates ('a' = hylo = 0x55). The font table
  encodes what human eyes see (scancode 0x1E → bitmap of 'a'). Same
  scancode indexes both. Two concerns, two tables, one input.

- **No ASCII anywhere**: Killed scancode_to_ascii. The font is indexed
  directly by PS/2 scancode. No 'a' = 65 = 0x41 nonsense.

- **Special keys are forces**: Space=+W (signal/separator), Enter=+R
  (advance), Backspace=-P (erase backward). The lattice forces name
  what the keys do. Not arbitrary assignments.

- **USB works for free**: QEMU and BIOS translate USB keyboards to PS/2
  via legacy emulation. Same scancodes. Native USB driver is build
  order item 7+.

## Open bugs

- **cursor_y corruption**: Something corrupts the cursor_y BSS variable
  during KVM + MMIO framebuffer writes. Root cause unknown. PIC masking
  didn't fix it. Cache-line isolating cursor_x/cursor_y + bounds clamping
  prevents the crash. Only happens with real PS/2 input in KVM GUI mode.
  TCG mode doesn't reproduce. Test walks with identical bond sequences
  don't reproduce. Likely a KVM EPT violation handler quirk.

## Key bugs found + fixed

- **jl vs jb (signed vs unsigned clamp)**: cursor_y corruption produced
  negative values. `cmp r13d, 768; jl` passes negative values through
  (-2B < 768 = true). Changed to `jb` (unsigned below) — negative values
  become huge unsigned → clamp catches them. Classic signed/unsigned trap.

- **walk_sub length pass-by-address**: Passing `dd mouse_walk_len` gives
  the ADDRESS of the length label, not the value. walk_sub reads garbage
  length → walker runs off the end → crash. Fix: compute length at
  assembly time with `dd (mouse_walk_end - mouse_walk)` (same file).

## For the next wit

- **Shift/layers**: Layers don't need timers — just key-down/key-up
  state tracking. Shift is the simplest: hold shift scancode (0x2A/0x36),
  set flag, use different font table for uppercase + shifted symbols.
  The old sm-input had great layer architecture (Hold/Toggle/Oneshot).

- **cursor_y corruption root cause**: Still unknown. Something corrupts
  cursor_y during KVM MMIO FB writes. Page-aligned isolation + unsigned
  bounds clamping prevents crash. Doesn't reproduce in TCG or test walks.

- **render_char → walks/render.asm**: Runtime rendering code still in
  genesis as scaffolding. Should move out like mouse.asm did.

- **Build order**: Input pipeline (item 1) is DONE. Mouse works.
  Next: loop registry (item 2), bind drain (item 3), or serial output (item 4).

## Files changed

```
code/
  [2] cpu/entry           PIC masking (0xFF to ports 0x21, 0xA1)
  [3] handoff             cleaned up
  [6] genesis             render_char: wave bytes + Latin bitmap + SDF fallback
                          mouse_init gene (PS/2 AUX port init)
                          BSS: mouse vars, cursor page-isolated
  walks/
    kbd.asm               wave byte table + font_by_scancode + bit-5 routing
                          mouse walk (state machine as bonds)
    mouse.asm             mouse_update + cursor drawing (x86 scaffolding)
  build/
    run                   added mouse.asm to build
```
