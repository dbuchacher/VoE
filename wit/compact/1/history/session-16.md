# Session 16: Loop Pipeline + find_work + Keyboard Echo

Coder session. Built the loop pipeline from scratch: genesis allocates
loops, registers them, find_work scans and drains. Then added keyboard
echo via PS/2 polling. Debugged MBR BSS corruption along the way.

## What We Built

### 1. Test loop consumer walk (walks/test_loop.w, NEW)
Self-contained drain walk: reads byte records from a loop, prints to
debugcon, stops when empty. Pattern: load cursors from header, compare
(empty check), compute slot = buf + (cursor & mask), read byte, print,
advance cursor, write back, loop_back.

### 2. Loop allocation in genesis (genesis.w, MODIFIED)
After boot confirmation, genesis allocates a test loop at fixed address
0x80000 (see BSS bug below). Writes header fields (write_cursor, buffer
address, depth, mask, record_size), pre-loads 3 test records ('H','I','\n'),
registers in loop registry.

### 3. Loop registry + find_work (x86.asm, MODIFIED)
BSS registry arrays: loop_list[16], consumer_walk_list[16],
consumer_wlen_list[16], loop_count.

Registry addresses stored at scratch 0x9A00-0x9A28 for genesis to write.
find_work scans registry, runs consumer of first non-empty loop.
BSP enters find_work after genesis. APs spin (Pauli violation on shared
scratch — per-core cursors needed).

Keyboard poll in find_work idle path: calls kbd.w every iteration.

### 4. Keyboard echo (walks/kbd.w, NEW)
PS/2 keyboard producer walk. Polls port 0x64 status, reads scancode
from port 0x60, looks up ASCII in 256-byte scancode table (rodata in
x86.asm), prints to debugcon.

Proven: type keys in QEMU window → ASCII appears on terminal.

### 5. Build changes (build/run, MODIFIED)
Assembles test_loop.w and kbd.w alongside genesis.w.

## Bugs Found and Fixed

### 1. Stale MBR disk image
`bash build/run headless` doesn't rebuild the MBR disk image. Running
QEMU with an old .build/nomos.img caused confusion — output came from
old genesis code. Wasted significant debug time.

**Lesson**: always rebuild the MBR image before testing MBR mode.

### 2. BSS loop_region corrupted in MBR mode
loop_region (BSS at 0x16F000) gets overwritten between genesis writing
test data and the consumer reading it. Headless (multiboot) works fine.
MBR produces JIT code bytes (0xC3 0x48 0x8D...) when reading from
loop_region. Root cause NOT found — possibly page table, BSS zeroing,
or ATA DMA interaction.

**Fix**: use fixed physical address 0x80000 instead of BSS loop_region.
Works in both modes. Root cause investigation deferred.

### 3. δ comparison destroys pipeline
kbd.w compared the ASCII value with 0 to check for unmapped keys, then
tried to print. But δ (test bond) writes the comparison result (0 or 1)
to pipeline, destroying the ASCII value.

**Fix**: save ASCII to scratch before comparing, reload for print.

### 4. Scratch address collision
kbd.w originally saved scancode to 0x9B00, which is the loop header
address used by the consumer walk.

**Fix**: kbd.w uses 0x9B20/0x9B28 instead.

## Files Changed

```
MODIFIED:
  genesis.w         loop allocation at 0x80000, registry writes
  legacy/x86.asm    BSS registry, find_work, scancode table, kbd incbin
  build/run         assemble test_loop.w + kbd.w

NEW:
  walks/test_loop.w  consumer walk (byte records → debugcon)
  walks/kbd.w        keyboard walk (PS/2 poll → table lookup → debugcon)
```

## Memory Map

### Scratch (0x9A00-0x9A40)
```
0x9A00  &loop_list           (set by handoff)
0x9A08  &loop_count          (set by handoff)
0x9A10  &consumer_walk_list  (set by handoff)
0x9A18  &consumer_wlen_list  (set by handoff)
0x9A20  test_consumer ptr    (set by handoff)
0x9A28  test_consumer len    (set by handoff)
0x9A30  scancode_table ptr   (set by handoff)
0x9A38  kbd_walk ptr         (set by handoff)
0x9A40  kbd_walk len         (set by handoff)
```

### Loop scratch (0x9B00-0x9B28)
```
0x9B00  loop 0 header address (set by genesis, read by consumer)
0x9B08  loop 0 buffer address (set by genesis, read by consumer)
0x9B10  consumer temp: write_cursor
0x9B18  consumer temp: read_cursor
0x9B20  kbd temp: scancode
0x9B28  kbd temp: ASCII value
```

### Loop 0 (at 0x80000, fixed physical address)
```
+0x00  write_cursor = 3
+0x10  buffer_address = 0x80040
+0x18  depth = 16
+0x20  mask = 15
+0x28  record_size = 1
+0x30  scratch_a = reader cursor
+0x40  buffer: 'H' 'I' '\n' + 13 empty slots
```

## For the Next Wit

### State
- Loop pipeline works: genesis → registry → find_work → consumer drain
- Keyboard echo works: PS/2 → scancode table → debugcon
- Single core only (APs spin)
- BSS loop_region broken in MBR — using fixed 0x80000 workaround

### What's Next
1. **Per-core cursors** — APIC ID indexed, eliminate Pauli violation
2. **Display consumer** — render characters to framebuffer (keyboard
   currently echoes to debugcon only, not the teal screen)
3. **Keyboard → loop → display pipeline** — keyboard writes to input
   loop, display consumer reads and renders to framebuffer
