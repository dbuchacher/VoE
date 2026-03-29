; handoff.asm — init JIT, run genesis, signal APs
;
; Last substrate file before walks take over.
; After genesis, all cores enter find-work (spin for now).

section .text
bits 64

global Λ

extern ψ
extern code_base, code_cursor
extern γ, γ_len
extern genesis_done

Λ:
    ; init JIT code buffer
    lea rax, [code_base]
    mov [code_cursor], rax

    ; walk genesis
    lea rdi, [γ]
    mov esi, [γ_len]
    call ψ

    ; signal APs: genesis complete
    mov dword [genesis_done], 1

    ; BSP enters find-work (spin for now)
.spin:
    pause
    jmp .spin
