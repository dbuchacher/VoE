; loop.asm — loop read/write primitives
;
; %macros for interacting with loops (ring buffers).
; %include this file from any walk that needs loop I/O.
; Requires walk.inc to be included first.
;
; Three macros:
;   loop_empty %1   — check if loop is empty from this reader's view
;                     pipeline = 1 (empty) or 0 (has data)
;   loop_read %1    — read one record from loop
;                     pipeline = data value. advances reader's cursor.
;                     CALLER MUST CHECK EMPTY FIRST.
;   loop_write %1   — write pipeline value to loop (drop mode)
;                     advances write cursor. no fullness check.
;                     wraps and overwrites oldest if full. this is
;                     what rings DO — the default behavior.
;
; READER MODEL (session 9):
;   Each reader has its OWN cursor in scratch_a [48]. The header's
;   [8:15] is "slowest reader" for reclamation, not a live cursor.
;   Multiple readers see all records independently — boson semantics.
;   No shared mutable state between readers. No atomics. No locks.
;
;   Writer:  owns [0:7] write cursor + buffer slots
;   Reader:  owns [48:55] scratch_a as its read cursor
;   Buffer data flows one direction: writer → cache → readers
;
; Uses stash_b for temp in loop_read (data save during cursor advance).
; Uses stash_b for temp in loop_write (data save during slot compute).
; Producer and consumer don't run the same macro simultaneously on
; the same core, so sharing stash_b between read/write is safe.
;
; %1 = loop header base address (compile-time symbol, e.g., loop_kbd)
;
; Each loop header is 64 bytes (cache-line aligned):
;   [0:7]   write cursor    (single writer — fermion)
;   [8:15]  slowest reader  (for reclamation — writer updates)
;   [16:23] buffer address
;   [24:31] depth (power of 2)
;   [32:39] mask (depth - 1)
;   [40:47] record size
;   [48:55] scratch_a       (reader's own cursor — per-loop, core-safe)
;   [56:63] scratch_b       (per-loop temp for drain walk)

; ── loop header offsets ───────────────────────────────────────
LP_WCURSOR  equ 0
LP_SLOWREAD equ 8                               ; slowest reader (reclamation)
LP_BUFADDR  equ 16
LP_DEPTH    equ 24
LP_MASK     equ 32
LP_RECSZ    equ 40
LP_SCRATCHA equ 48                              ; reader's own cursor
LP_SCRATCHB equ 56                              ; per-loop temp


; ══════════════════════════════════════════════════════════════
; loop_empty — check if this reader has unread data
;
; Compares reader's own cursor (scratch_a) against write cursor.
; pipeline = 1 if empty (reader caught up to writer), 0 if has data.
; No shared state touched — reader reads write cursor (read-only)
; and its own cursor (owned exclusively).
; ══════════════════════════════════════════════════════════════

%macro loop_empty 1

    ;; read write cursor, save to stash_a (temp)
    w +1,0,0,0                              ; read [loop + WCURSOR] → pipeline
    db F(U32,P,P)
    dd (%1 + LP_WCURSOR)

    w -1,0,0,0                              ; stash_a = write_cursor (temp)
    db F(U32,P,P)
    dd stash_a

    ;; read THIS READER's cursor (scratch_a)
    w +1,0,0,0                              ; read [loop + SCRATCHA] → pipeline
    db F(U32,P,P)
    dd (%1 + LP_SCRATCHA)

    ;; compare: my_cursor == write_cursor?
    w 0,0,0,+1                              ; +W test
    db F(P,U32,P) | 0x40                    ;   arg1 = deref stash_a → write cursor
    dd stash_a

%endmacro


; ══════════════════════════════════════════════════════════════
; loop_read — read one record from the loop
;
; Uses THIS READER's cursor (scratch_a), not the header's.
; Each reader independently tracks where they are in the buffer.
; Multiple readers see all records — boson semantics.
;
; CALLER MUST VERIFY NON-EMPTY FIRST (loop_empty + skip_nz).
; pipeline = data value from the next unread slot.
; Advances reader's cursor (scratch_a) by 1.
; Uses stash_b for temp (data save during cursor advance).
;
; How it works:
;   1. reader_cursor (scratch_a) → AND mask → slot index
;   2. slot index × record size → byte offset  (atom T*D)
;   3. buffer address + byte offset → slot address
;   4. read [slot_address] → data
;   5. save data, advance reader's cursor, restore data
; ══════════════════════════════════════════════════════════════

%macro loop_read 1

    ;; 1. read THIS READER's cursor
    w +1,0,0,0                              ; read [loop + SCRATCHA] → pipeline
    db F(U32,P,P)
    dd (%1 + LP_SCRATCHA)

    ;; 2. slot_index = cursor AND mask
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

    ;; 7. advance reader's cursor: read, increment, write back
    w +1,0,0,0                              ; read SCRATCHA → pipeline
    db F(U32,P,P)
    dd (%1 + LP_SCRATCHA)

    w 0,0,-1,0                              ; increment: pipeline + 1
    db F(P,P,P)

    w -1,0,0,0                              ; write: SCRATCHA = pipeline (advanced)
    db F(U32,P,P)
    dd (%1 + LP_SCRATCHA)

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
;   read WCURSOR, read SLOWREAD, subtract, compare against depth.
;
; Input: pipeline = value to write.
; Advances write cursor by 1.
; Pipeline after macro = undefined (last cursor value).
; Uses stash_b for temp (data save during slot computation).
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
