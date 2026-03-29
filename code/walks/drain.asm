; drain.asm — bind drain: the anarcho-capitalist scheduler
;
; Single loop (loop_kbd). Inlines all operations using loop_kbd's
; scratch bytes [48:63] for temp storage. Does NOT use global stash.
; This is critical: the BSP uses stash for mouse/kbd walks.
; If drain touched stash, BSP+AP would race on the same addresses.
;
; Per-loop scratch = isolation. No shared mutable state. No locks.

bits 64

%include "[0][tool][walk.inc][NASM macros to write walks as coordinates instead of hex].inc"

extern loop_kbd
extern consumer_list, consumer_len_list
extern walk_sub

; Loop header offsets
LP_WCURSOR  equ 0
LP_RCURSOR  equ 8
LP_BUFADDR  equ 16
LP_MASK     equ 32
LP_RECSZ    equ 40
LP_SCRATCHA equ 48
LP_SCRATCHB equ 56

section .rodata

global drain_walk, drain_walk_len

drain_walk:

; ══════════════════════════════════════════════════════════════
; STEP 1: Check if loop_kbd has data (inlined, no stash)
;
; Read wcursor → save to loop scratch_a → read rcursor → compare.
; ══════════════════════════════════════════════════════════════

    ; read wcursor
    w +1,0,0,0
    db F(U32,P,P)
    dd (loop_kbd + LP_WCURSOR)

    ; save to loop scratch_a
    w -1,0,0,0
    db F(U32,P,P)
    dd (loop_kbd + LP_SCRATCHA)

    ; read rcursor
    w +1,0,0,0
    db F(U32,P,P)
    dd (loop_kbd + LP_RCURSOR)

    ; test(rcursor, *scratch_a) → 1 if equal (empty)
    w 0,0,0,+1
    db F(P,U32,P) | 0x40
    dd (loop_kbd + LP_SCRATCHA)

    ; if empty, skip to loopback
    skip_nz (.loopback - .has_work)

.has_work:

; ══════════════════════════════════════════════════════════════
; STEP 2: Read one record (inlined loop_read, uses loop scratch)
;
;   1. slot_index = rcursor AND mask
;   2. byte_offset = slot_index × recsz
;   3. slot_addr = bufaddr + byte_offset
;   4. data = [slot_addr]
;   5. save data to scratch_a
;   6. advance rcursor
; ══════════════════════════════════════════════════════════════

    ; 1. read rcursor
    w +1,0,0,0
    db F(U32,P,P)
    dd (loop_kbd + LP_RCURSOR)

    ; 2. slot_index = rcursor AND mask
    w 0,0,0,+3
    db F(P,U32,P) | 0x40
    dd (loop_kbd + LP_MASK)

    ; 3. byte_offset = slot_index × recsz
    w +2,+2,0,0
    db F(P,U32,P) | 0x40
    dd (loop_kbd + LP_RECSZ)

    ; 4. slot_addr = bufaddr + byte_offset
    w -1,0,0,-1
    db F(U32,P,P) | 0x80
    dd (loop_kbd + LP_BUFADDR)

    ; 5. data = [slot_addr]
    w +1,0,0,0
    db F(P,P,P)

    ; 6. save data to scratch_a
    w -1,0,0,0
    db F(U32,P,P)
    dd (loop_kbd + LP_SCRATCHA)

    ; 7. advance rcursor: read, increment, write back
    w +1,0,0,0
    db F(U32,P,P)
    dd (loop_kbd + LP_RCURSOR)

    w 0,0,-1,0
    db F(P,P,P)

    w -1,0,0,0
    db F(U32,P,P)
    dd (loop_kbd + LP_RCURSOR)

; ══════════════════════════════════════════════════════════════
; STEP 3: Call consumer walk
;
; Data is in loop scratch_a. Consumer reads it from there.
; ══════════════════════════════════════════════════════════════

    ; load consumer walk ptr from consumer_list[0]
    w +1,0,0,0
    db F(U32,P,P)
    dd consumer_list

    ; if consumer_ptr == 0, skip
    w 0,0,0,+1
    db F(P,U8,P), 0
    skip_nz (.loopback - .call_consumer)

.call_consumer:
    ; save consumer_ptr to scratch_b
    w -1,0,0,0
    db F(U32,P,P)
    dd (loop_kbd + LP_SCRATCHB)

    ; load consumer length (double deref: consumer_len_list[0] → ptr → value)
    w +1,0,0,0
    db F(U32,P,P)
    dd consumer_len_list
    w +1,0,0,0
    db F(P,P,P)

    ; apply(walk_sub, *scratch_b = consumer_ptr, pipeline = length)
    w 0,+1,0,0
    db F(U32,U32,P) | 0x40
    dd walk_sub
    dd (loop_kbd + LP_SCRATCHB)

; ══════════════════════════════════════════════════════════════
; STEP 4: Loop back
; ══════════════════════════════════════════════════════════════

.loopback:
    w -1,0,0,-1
    db F(U8,U8,P), 1, 0
.lb:
    loop_back (.lb - drain_walk)

drain_walk_end:
drain_walk_len: dd (drain_walk_end - drain_walk)
