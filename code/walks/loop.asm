; loop.asm — loop read/write primitives
;
; %macros for interacting with loops (ring buffers).
; %include this file from any walk that needs loop I/O.
; Requires walk.inc to be included first.
;
; Three macros:
;   loop_empty %1   — check if loop is empty
;                     pipeline = 1 (empty) or 0 (has data)
;   loop_read %1    — read one record from loop
;                     pipeline = data value. advances read cursor.
;                     CALLER MUST CHECK EMPTY FIRST.
;   loop_write %1   — write pipeline value to loop (drop mode)
;                     advances write cursor. no fullness check.
;                     wraps and overwrites oldest if full. this is
;                     what rings DO — the default behavior.
;
; Uses stash_a (loop_empty) and stash_b (loop_read, loop_write).
; Don't nest or interleave — one operation at a time per walk.
;
; %1 = loop header base address (compile-time symbol, e.g., loop_kbd)
;
; Each loop header is 64 bytes (cache-line aligned):
;   [0:7]   write cursor    (single writer — fermion)
;   [8:15]  read cursor     (many readers — boson)
;   [16:23] buffer address  (pointer to ring buffer memory)
;   [24:31] depth           (power of 2, number of slots)
;   [32:39] mask            (depth - 1, for cursor wrapping)
;   [40:47] record size     (bytes per record)
;   [48:63] reserved
;
; Session 2 proved this pattern correct (round-trip: write 42, read '*').
; Reverted then because QEMU TCG spewed spurious debugcon bytes.
; TCG bug, not ours. Use KVM or real hardware.

; ── loop header offsets ───────────────────────────────────────
LP_WCURSOR  equ 0
LP_RCURSOR  equ 8
LP_BUFADDR  equ 16
LP_DEPTH    equ 24
LP_MASK     equ 32
LP_RECSZ    equ 40
LP_SCRATCHA equ 48                          ; per-loop scratch (replaces global stash_a)
LP_SCRATCHB equ 56                          ; per-loop scratch (replaces global stash_b)


; ══════════════════════════════════════════════════════════════
; loop_empty — check if loop has data
;
; pipeline = 1 if empty (read == write), 0 if has data.
; Uses stash_a for temp (consumer side — separated from producer).
; ══════════════════════════════════════════════════════════════

%macro loop_empty 1

    ;; read write cursor, save to stash_a
    w +1,0,0,0                              ; read qword [loop + WCURSOR] → pipeline
    db F(U32,P,P)
    dd (%1 + LP_WCURSOR)

    w -1,0,0,0                              ; write: stash_a = write_cursor
    db F(U32,P,P)
    dd stash_a

    ;; read read cursor
    w +1,0,0,0                              ; read qword [loop + RCURSOR] → pipeline
    db F(U32,P,P)
    dd (%1 + LP_RCURSOR)

    ;; compare: read_cursor == write_cursor?
    w 0,0,0,+1                              ; +W test
    db F(P,U32,P) | 0x40                    ;   arg1 = deref stash_a → write cursor value
    dd stash_a

%endmacro


; ══════════════════════════════════════════════════════════════
; loop_read — read one record from the loop
;
; CALLER MUST VERIFY NON-EMPTY FIRST (loop_empty + skip_nz).
; pipeline = data value from the next unread slot.
; Advances read cursor by 1.
; Uses stash_b for temp (consumer side — separated from producer).
;
; How it works:
;   1. read_cursor → AND mask → slot index
;   2. slot index × record size → byte offset  (atom T*D)
;   3. buffer address + byte offset → slot address
;   4. read [slot_address] → data
;   5. save data, advance cursor, restore data
; ══════════════════════════════════════════════════════════════

%macro loop_read 1

    ;; 1. read_cursor → pipeline
    w +1,0,0,0                              ; read qword [loop + RCURSOR]
    db F(U32,P,P)
    dd (%1 + LP_RCURSOR)

    ;; 2. slot_index = read_cursor AND mask
    w 0,0,0,+3                              ; AND(pipeline, *mask)
    db F(P,U32,P) | 0x40                    ;   arg1 = deref [loop + MASK]
    dd (%1 + LP_MASK)

    ;; 3. byte_offset = slot_index × record_size
    w +2,+2,0,0                             ; atom TD(pipeline, *recsz)
    db F(P,U32,P) | 0x40                    ;   arg1 = deref [loop + RECSZ]
    dd (%1 + LP_RECSZ)

    ;; 4. slot_address = buffer_address + byte_offset
    w -1,0,0,-1                             ; add(*bufaddr, pipeline)
    db F(U32,P,P) | 0x80                    ;   arg0 = deref [loop + BUFADDR]
    dd (%1 + LP_BUFADDR)

    ;; 5. read data from slot
    w +1,0,0,0                              ; read qword [pipeline] → data
    db F(P,P,P)

    ;; 6. save data to stash_b (cursor advance clobbers pipeline)
    w -1,0,0,0                              ; write: stash_b = data
    db F(U32,P,P)
    dd stash_b

    ;; 7. advance read cursor: read, increment, write back
    w +1,0,0,0                              ; read RCURSOR → pipeline
    db F(U32,P,P)
    dd (%1 + LP_RCURSOR)

    w 0,0,-1,0                              ; increment: pipeline + 1
    db F(P,P,P)

    w -1,0,0,0                              ; write: RCURSOR = pipeline (advanced)
    db F(U32,P,P)
    dd (%1 + LP_RCURSOR)

    ;; 8. restore data to pipeline from stash_b
    w +1,0,0,0                              ; read stash_b → pipeline = data
    db F(U32,P,P)
    dd stash_b

%endmacro


; ══════════════════════════════════════════════════════════════
; loop_write — write pipeline value to the loop (drop mode)
;
; Drop mode: no fullness check. Wraps and overwrites oldest.
; For blocking writes (persistence), check fullness yourself:
;   read WCURSOR, read RCURSOR, subtract, compare against depth.
;
; Input: pipeline = value to write.
; Advances write cursor by 1.
; Pipeline after macro = undefined (last cursor value).
; Uses stash_b for temp (same as consumer — safe when single-writer).
; ══════════════════════════════════════════════════════════════

%macro loop_write 1

    ;; 1. save pipeline (data to write) to stash_b
    w -1,0,0,0                              ; write: stash_b = pipeline (the data)
    db F(U32,P,P)
    dd stash_b

    ;; 2. read write_cursor → pipeline
    w +1,0,0,0                              ; read qword [loop + WCURSOR]
    db F(U32,P,P)
    dd (%1 + LP_WCURSOR)

    ;; 3. slot_index = write_cursor AND mask
    w 0,0,0,+3                              ; AND(pipeline, *mask)
    db F(P,U32,P) | 0x40                    ;   deref [loop + MASK]
    dd (%1 + LP_MASK)

    ;; 4. byte_offset = slot_index × record_size
    w +2,+2,0,0                             ; atom TD(pipeline, *recsz)
    db F(P,U32,P) | 0x40                    ;   deref [loop + RECSZ]
    dd (%1 + LP_RECSZ)

    ;; 5. slot_address = buffer_address + byte_offset
    w -1,0,0,-1                             ; add(*bufaddr, pipeline)
    db F(U32,P,P) | 0x80                    ;   deref [loop + BUFADDR]
    dd (%1 + LP_BUFADDR)

    ;; 6. write data to slot: [slot_addr] = *stash_b
    w -1,0,0,0                              ; write: [pipeline] = *stash_b
    db F(P,U32,P) | 0x40                    ;   arg1 = deref stash_b → data
    dd stash_b

    ;; 7. advance write cursor: read, increment, write back
    w +1,0,0,0                              ; read WCURSOR → pipeline
    db F(U32,P,P)
    dd (%1 + LP_WCURSOR)

    w 0,0,-1,0                              ; increment: pipeline + 1
    db F(P,P,P)

    w -1,0,0,0                              ; write: WCURSOR = pipeline (advanced)
    db F(U32,P,P)
    dd (%1 + LP_WCURSOR)

%endmacro
