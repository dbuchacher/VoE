; genesis.asm — the founding walk
;
; Runs once through the walker. Inits display, tests loops + bind.
; This is PAYLOAD — pure coordinates, no x86. The walker and
; equation handle the encoding.

bits 64

%include "[0] walk.inc"
%include "loop.inc"

; ── loop headers + buffers (BSS) ─────────────────────────────

section .bss
align 64

; test loop: 8 slots × 8 bytes = 64 bytes buffer
loop_test:  resb 64                         ; 64-byte header

align 8
loop_test_buf: resb 8 * 8                  ; 8 slots × 8 bytes


; ── sub-walks (rodata, referenced by bind) ───────────────────

section .rodata

; bind test sub-walk: decrement counter in scratch_b, print '!'
; Returns nonzero while counter > 0, returns 0 when done.
bind_sub:
    ; read counter
    w +1,0,0,0                              ; read [scratch_b] → pipeline
    db F(U32,P,P)
    dd (loop_test + LP_SCRATCHB)

    ; if zero, we're done — return 0
    skip_z .bs_done - .bs_work

.bs_work:
    ; decrement counter
    w -1,0,0,-3                             ; subtract(pipeline, 1)
    db F(P,U8,P)
    db 1

    ; write back
    w -1,0,0,0                              ; write [scratch_b] = pipeline
    db F(U32,P,P)
    dd (loop_test + LP_SCRATCHB)

%ifdef DEBUG
    ; print '!' to debugcon
    w -1,0,0,+1                             ; port_write(0xE9, '!')
    db F(U8,U8,P), 0xE9, '!'
%endif

    ; return nonzero (pipeline = decremented counter, could be 0 on last iteration)
    ; actually we need to return nonzero explicitly since the last decrement gives 0
    w -1,0,0,-1                             ; add(1, 0) → pipeline = 1
    db F(U8,U8,P), 1, 0

.bs_done:
bind_sub_end:


; ── the main walk ────────────────────────────────────────────

global γ, γ_len

γ:

    ; ── read framebuffer address from 0x9100 (dword) ──────────
    w +7,0,0,0                              ; read dword from [0x9100]
    db F(U32,P,P)
    dd 0x9100

    ; skip fill if no framebuffer (pipeline == 0)
    skip_z .fill_end - .fill_start

.fill_start:
    ; fill screen teal: 1024×768 pixels, BGRX 0x00008080
    w -1,0,-3,0                             ; dword fill(dest, value, count)
    db F(P,U32,U32)
    dd 0x00008080                           ; teal
    dd 1024 * 768
.fill_end:

    ; ── init test loop header ─────────────────────────────────
    w -1,0,0,0                              ; bufaddr
    db F(U32,U64,P)
    dd (loop_test + LP_BUFADDR)
    dq loop_test_buf

    w -1,0,0,0                              ; depth = 8
    db F(U32,U64,P)
    dd (loop_test + LP_DEPTH)
    dq 8

    w -1,0,0,0                              ; mask = 7
    db F(U32,U64,P)
    dd (loop_test + LP_MASK)
    dq 7

    w -1,0,0,0                              ; recsz = 8
    db F(U32,U64,P)
    dd (loop_test + LP_RECSZ)
    dq 8

    ; ── test: loop write/read ─────────────────────────────────
    w -1,0,0,-1                             ; pipeline = 0x42 ('B')
    db F(U8,U8,P)
    db 0x42, 0

    loop_write loop_test

    loop_empty loop_test
    skip_nz .read_end - .read_start

.read_start:
    loop_read loop_test
    w +1,0,0,0                              ; deref slot pointer → data
    db F(P,P,P)

%ifdef DEBUG
    w -1,0,0,+1                             ; print data byte
    db F(U8,P,P)
    db 0xE9
    w -1,0,0,+1                             ; newline
    db F(U8,U8,P), 0xE9, 10
%endif
.read_end:

    ; ── test: bind (pattern 14) ───────────────────────────────
    ; Set counter = 3 in scratch_b
    w -1,0,0,-1                             ; pipeline = 3
    db F(U8,U8,P), 3, 0
    w -1,0,0,0                              ; write [scratch_b] = 3
    db F(U32,P,P)
    dd (loop_test + LP_SCRATCHB)

    ; bind: call bind_sub repeatedly until it returns 0
    ; Should print "!!!" (3 times)
    w 0,+1,+1,+1                            ; bind(walk_ptr, walk_len)
    db F(U32,U32,P)
    dd bind_sub
    dd (bind_sub_end - bind_sub)

%ifdef DEBUG
    w -1,0,0,+1                             ; newline after "!!!"
    db F(U8,U8,P), 0xE9, 10
%endif

    ; ── confirm boot ──────────────────────────────────────────
%ifdef DEBUG
    w -1,0,0,+1
    db F(U8,U8,P), 0xE9, 'V'
    w -1,0,0,+1
    db F(U8,U8,P), 0xE9, 'o'
    w -1,0,0,+1
    db F(U8,U8,P), 0xE9, 'E'
    w -1,0,0,+1
    db F(U8,U8,P), 0xE9, 10
%endif

γ_len: dd (γ_len - γ)
