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
extern drain_walk, drain_walk_len
extern code_base, code_cursor
extern ap_drain_ready

lattice_start:
    lea rax, [code_base]
    mov [code_cursor], rax

    ; walk genesis (inits hardware, loops, consumer registration)
    lea rdi, [main_walk]
    mov esi, [main_len]
    call walker_wave

    ; signal APs: genesis done, loops initialized
    mov dword [ap_drain_ready], 1

    ; BSP enters kbd_walk (polls PS/2, handles keyboard + mouse)
    lea rdi, [kbd_walk]
    mov esi, [kbd_walk_len]
    call walker_wave

    ; safety net (kbd_walk has loop_back, should never reach here)
.spin:
    pause
    jmp .spin
