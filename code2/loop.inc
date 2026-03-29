; loop.inc — loop (ring buffer) macros for walks
;
; %include this from any walk that needs loop I/O.
; Requires walk.inc to be included first.
;
; Three macros:
;   loop_empty %1   check if empty from this reader's view
;                   pipeline = 1 (empty) or 0 (has data)
;
;   loop_read %1    zero-copy read: pipeline = slot POINTER
;                   consumer reads fields at offsets from pointer.
;                   cursor already advanced. CALLER CHECKS EMPTY FIRST.
;
;   loop_write %1   write pipeline value to loop (drop mode)
;                   wraps and overwrites oldest if full. that's
;                   what rings DO. check fullness yourself for block mode.
;
; %1 = loop header base address (compile-time symbol, e.g., loop_kbd)
;
; READER MODEL:
;   Each reader owns its cursor in scratch_a [48]. Single reader per
;   loop for now. Multiple readers = per-core storage (future).
;   Writer owns [0:7] write cursor + buffer slots.
;   No shared mutable state. No atomics. No locks.
;
; TEMP STORAGE:
;   Uses per-loop scratch_b [56] for temp. NOT global stash.
;   Each loop has its own scratch_b — cores on different loops
;   never collide (Pauli exclusion at the loop level).
;
; ZERO-COPY:
;   loop_read returns a pointer to the slot in the buffer.
;   Consumer reads fields at offsets. The data stays in place.
;   Observation doesn't move data (boson semantics).
;
; Loop header layout (64 bytes, cache-line aligned):
;   [0:7]   write cursor    (single writer — fermion)
;   [8:15]  slowest reader  (for reclamation — writer updates)
;   [16:23] buffer address
;   [24:31] depth (power of 2)
;   [32:39] mask (depth - 1)
;   [40:47] record size in bytes
;   [48:55] scratch_a       (reader's cursor — per-loop)
;   [56:63] scratch_b       (per-loop temp)

; ── header offsets ────────────────────────────────────────────
LP_WCURSOR  equ 0
LP_SLOWREAD equ 8
LP_BUFADDR  equ 16
LP_DEPTH    equ 24
LP_MASK     equ 32
LP_RECSZ    equ 40
LP_SCRATCHA equ 48
LP_SCRATCHB equ 56


; ══════════════════════════════════════════════════════════════
; loop_empty — is there unread data?
;
; Compares reader's cursor (scratch_a) to write cursor.
; pipeline = 1 if empty (reader caught up), 0 if has data.
; 4 bonds. No shared state touched.
; ══════════════════════════════════════════════════════════════

%macro loop_empty 1

    ;; read write cursor, save to scratch_b (temp)
    w +1,0,0,0                              ; read qword [wcursor]
    db F(U32,ρ,ρ)
    dd (%1 + LP_WCURSOR)

    w -1,0,0,0                              ; scratch_b = write_cursor
    db F(U32,ρ,ρ)
    dd (%1 + LP_SCRATCHB)

    ;; read this reader's cursor
    w +1,0,0,0                              ; read qword [scratch_a]
    db F(U32,ρ,ρ)
    dd (%1 + LP_SCRATCHA)

    ;; compare: reader_cursor == write_cursor?
    w 0,0,0,+1                              ; test(pipeline, deref scratch_b)
    db F(ρ,U32,ρ) | 0x40                    ;   arg1 = deref → write_cursor value
    dd (%1 + LP_SCRATCHB)

%endmacro


; ══════════════════════════════════════════════════════════════
; loop_read — zero-copy read, returns slot POINTER
;
; CALLER MUST CHECK EMPTY FIRST (loop_empty + skip_nz).
;
; Strategy: advance cursor first, then compute pointer from
; (cursor - 1). Pipeline ends with the slot pointer. No temp
; storage needed — advance-before-compute eliminates the save.
;
; The consumer reads fields at offsets from the pointer.
; Record size doesn't matter to this macro — the topology
; determines what the consumer reads.
;
; 7 bonds. Pipeline = slot pointer.
; ══════════════════════════════════════════════════════════════

%macro loop_read 1

    ;; 1. read reader's cursor
    w +1,0,0,0                              ; read qword [scratch_a] → cursor C
    db F(U32,ρ,ρ)
    dd (%1 + LP_SCRATCHA)

    ;; 2. advance cursor: C + 1
    w 0,0,-1,0                              ; increment(pipeline) → C + 1
    db F(ρ,ρ,ρ)

    ;; 3. write advanced cursor back
    w -1,0,0,0                              ; scratch_a = C + 1
    db F(U32,ρ,ρ)
    dd (%1 + LP_SCRATCHA)

    ;; 4. recover original cursor: C + 1 - 1 = C
    w -1,0,0,-3                             ; subtract(pipeline, 1) → C
    db F(ρ,U8,ρ)
    db 1

    ;; 5. slot_index = C AND mask
    w 0,0,0,+3                              ; AND(pipeline, deref mask)
    db F(ρ,U32,ρ) | 0x40
    dd (%1 + LP_MASK)

    ;; 6. byte_offset = slot_index × record_size
    w +2,+2,0,0                             ; atom T*D(pipeline, deref recsz)
    db F(ρ,U32,ρ) | 0x40
    dd (%1 + LP_RECSZ)

    ;; 7. slot_pointer = buffer_address + byte_offset
    w -1,0,0,-1                             ; add(deref bufaddr, pipeline)
    db F(U32,ρ,ρ) | 0x80
    dd (%1 + LP_BUFADDR)

%endmacro


; ══════════════════════════════════════════════════════════════
; loop_write — write pipeline value to loop (drop mode)
;
; Input: pipeline = value to write (qword).
; Drop mode: wraps and overwrites oldest. No fullness check.
; For blocking writes, check fullness before calling.
;
; Uses scratch_b to save the data value while computing the
; slot address from the write cursor.
;
; 9 bonds. Pipeline = written value at end.
; ══════════════════════════════════════════════════════════════

%macro loop_write 1

    ;; 1. save data to scratch_b
    w -1,0,0,0                              ; scratch_b = pipeline (data)
    db F(U32,ρ,ρ)
    dd (%1 + LP_SCRATCHB)

    ;; 2. read write cursor
    w +1,0,0,0                              ; read qword [wcursor] → W
    db F(U32,ρ,ρ)
    dd (%1 + LP_WCURSOR)

    ;; 3. slot_index = W AND mask
    w 0,0,0,+3                              ; AND(pipeline, deref mask)
    db F(ρ,U32,ρ) | 0x40
    dd (%1 + LP_MASK)

    ;; 4. byte_offset = slot_index × record_size
    w +2,+2,0,0                             ; atom T*D(pipeline, deref recsz)
    db F(ρ,U32,ρ) | 0x40
    dd (%1 + LP_RECSZ)

    ;; 5. slot_address = buffer_address + byte_offset
    w -1,0,0,-1                             ; add(deref bufaddr, pipeline)
    db F(U32,ρ,ρ) | 0x80
    dd (%1 + LP_BUFADDR)

    ;; 6. write data to slot: [slot_addr] = deref scratch_b
    w -1,0,0,0                              ; write: [pipeline] = data
    db F(ρ,U32,ρ) | 0x40                    ;   arg1 = deref scratch_b → data
    dd (%1 + LP_SCRATCHB)

    ;; 7. advance write cursor: read, increment, write back
    w +1,0,0,0                              ; read [wcursor] → W
    db F(U32,ρ,ρ)
    dd (%1 + LP_WCURSOR)

    w 0,0,-1,0                              ; increment → W + 1
    db F(ρ,ρ,ρ)

    w -1,0,0,0                              ; wcursor = W + 1
    db F(U32,ρ,ρ)
    dd (%1 + LP_WCURSOR)

%endmacro
