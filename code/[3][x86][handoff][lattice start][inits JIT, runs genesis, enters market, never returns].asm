; ═══════════════════════════════════════════════════════════════════
; [3] Handoff — the last x86 code before walks take over
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
    lea rax, [code_base]
    mov [code_cursor], rax

    ; walk genesis
    lea rdi, [main_walk]
    mov esi, [main_len]
    call walker_wave

    ; walk kbd
    lea rdi, [kbd_walk]
    mov esi, [kbd_walk_len]
    call walker_wave

    ; safety net
.spin:
    pause
    jmp .spin
