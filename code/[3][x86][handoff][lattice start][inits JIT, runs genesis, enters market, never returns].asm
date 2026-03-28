; ═══════════════════════════════════════════════════════════════════
; [3] Handoff — the last x86 code before walks take over
;
; CPU entry [2] jumps here. We're in 64-bit mode, all cores awake.
; This is where x86 setup ends and the lattice begins.
;
; What this does:
;   1. Point the JIT code buffer to its start (equation [4] writes x86 here)
;   2. Run genesis [6] — the founding walk that inits hardware and loops
;   3. Run kbd walk — keyboard poll loop (never returns, loops via loop_back)
;   4. If the walk somehow returns, spin forever (safety net)
;
; No bond seeds. The lattice is lazy — every coordinate JITs on first
; use via get_atom. Genesis hits port_write on its first bond, the
; equation JITs it, caches it forever. Pure lazy materialization.
;
; TODO: step 3 should be the bind drain (find fullest loop, drain it)
; instead of hardcoding the keyboard walk.
; ═══════════════════════════════════════════════════════════════════

section .text
bits 64

global lattice_start

extern get_atom
extern walker_wave
extern main_walk, main_len
extern kbd_walk, kbd_walk_len
extern code_base, code_cursor

lattice_start:
    ; init code generation buffer
    lea rax, [code_base]
    mov [code_cursor], rax

    ; ── walk genesis (runs once, returns) ──
    lea rdi, [main_walk]
    mov esi, [main_len]
    call walker_wave

    ; ── genesis returned. enter the market. ──
    ; walk the keyboard poll (first available work)
    ; TODO: bind drain — find fullest loop instead of hardcoding
    lea rdi, [kbd_walk]
    mov esi, [kbd_walk_len]
    call walker_wave

    ; if walk returns (it shouldn't — loop_back), spin
.spin:
    pause
    jmp .spin
