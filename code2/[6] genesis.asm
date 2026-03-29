; γ — genesis. the founding walk.
;
;   physics:    the big bang — runs once, creates the initial state
;   math:       initialization polynomial
;   programmer: boot sequence — init display, test loops + bind
;   english:    sets everything up, then hands over

bits 64

%include "[0] walk.inc"
%include "loop.inc"

; ── loop headers + buffers ───────────────────────────────────

section .bss
align 64
loop_test:      resb 64
align 8
loop_test_buf:  resb 8 * 8

; ── bind test sub-walk ───────────────────────────────────────

section .rodata

bind_sub:
    π                                       ; read [scratch_b] → ρ
    db θ(²,ρ,ρ)
    dd (loop_test + LP_SCRATCHB)

    skip_z .bs_done - .bs_work

.bs_work:
    π̄δ̄₃                                     ; subtract(ρ, 1)
    db θ(ρ,¹,ρ)
    db 1

    π̄                                       ; write [scratch_b] = ρ
    db θ(²,ρ,ρ)
    dd (loop_test + LP_SCRATCHB)

%ifdef DEBUG
    π̄δ                                      ; port_write(0xE9, '!')
    db θ(¹,¹,ρ), 0xE9, '!'
%endif

    π̄δ̄                                      ; add(1, 0) → ρ = 1 (nonzero)
    db θ(¹,¹,ρ), 1, 0

.bs_done:
bind_sub_end:

; ── the walk ─────────────────────────────────────────────────

global γ, γ_len

γ:

    ; ── display ───────────────────────────────────────────────
    π₇                                      ; read dword [0x9100] → FB address
    db θ(²,ρ,ρ)
    dd 0x9100

    skip_z .fill_end - .fill_start

.fill_start:
    π̄∮̄₃                                     ; dword fill(ρ, teal, pixels)
    db θ(ρ,²,²)
    dd 0x00008080
    dd 1024 * 768
.fill_end:

    ; ── init loop ─────────────────────────────────────────────
    π̄                                       ; bufaddr
    db θ(²,³,ρ)
    dd (loop_test + LP_BUFADDR)
    dq loop_test_buf

    π̄                                       ; depth = 8
    db θ(²,³,ρ)
    dd (loop_test + LP_DEPTH)
    dq 8

    π̄                                       ; mask = 7
    db θ(²,³,ρ)
    dd (loop_test + LP_MASK)
    dq 7

    π̄                                       ; recsz = 8
    db θ(²,³,ρ)
    dd (loop_test + LP_RECSZ)
    dq 8

    ; ── test: loop write/read ─────────────────────────────────
    π̄δ̄                                      ; add(0x42, 0) → ρ = 'B'
    db θ(¹,¹,ρ)
    db 0x42, 0

    loop_write loop_test

    loop_empty loop_test
    skip_nz .read_end - .read_start

.read_start:
    loop_read loop_test
    π                                       ; deref slot pointer → data
    db θ(ρ,ρ,ρ)

%ifdef DEBUG
    π̄δ                                      ; print data byte
    db θ(¹,ρ,ρ)
    db 0xE9
    π̄δ                                      ; newline
    db θ(¹,¹,ρ), 0xE9, 10
%endif
.read_end:

    ; ── test: bind ────────────────────────────────────────────
    π̄δ̄                                      ; ρ = 3
    db θ(¹,¹,ρ), 3, 0
    π̄                                       ; write [scratch_b] = 3
    db θ(²,ρ,ρ)
    dd (loop_test + LP_SCRATCHB)

    ∘∮δ                                      ; bind(sub_walk, len) — loops 3×
    db θ(²,²,ρ)
    dd bind_sub
    dd (bind_sub_end - bind_sub)

%ifdef DEBUG
    π̄δ                                      ; newline
    db θ(¹,¹,ρ), 0xE9, 10
%endif

    ; ── boot confirmation ─────────────────────────────────────
%ifdef DEBUG
    π̄δ
    db θ(¹,¹,ρ), 0xE9, 'V'
    π̄δ
    db θ(¹,¹,ρ), 0xE9, 'o'
    π̄δ
    db θ(¹,¹,ρ), 0xE9, 'E'
    π̄δ
    db θ(¹,¹,ρ), 0xE9, 10
%endif

γ_len: dd (γ_len - γ)
