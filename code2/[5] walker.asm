; ψ — the wave function
;
;   physics:    collapses wave bytes into observables
;   math:       evaluates a polynomial (sequence of terms)
;   programmer: bytecode interpreter for wave-encoded instructions
;   english:    reads a list of 4-number coordinates and does each one
;
; Input:  rdi = walk pointer, esi = walk length
; Output: rax = last result
;
; On a trit processor this dissolves — the hardware IS ψ.

section .text
bits 64

extern ƒ                          ; the equation (resolves coordinates → operations)
extern ρ                          ; last result (shared observable state)

global ψ
ψ:
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r12, rdi                   ; walk pointer
    mov r13d, esi                  ; walk length
    lea r14, [r12 + r13]           ; end
    xor r15d, r15d                 ; pipeline = 0 (vacuum state)

; ── next term in the polynomial ──────────────────────────────
;   physics:    observe the next quantum state
;   math:       evaluate next monomial/bond in the sum
;   programmer: fetch-decode-execute cycle
;   english:    read the next instruction and do it
.next:
    cmp r12, r14
    jge .done                         ; end of walk

    movzx eax, byte [r12]

    cmp al, 0xFC                   ; control bytes (skip, loop)
    jae .control

    ; ── decode wave byte → 4 coordinates ─────────────────────
    ;   physics:    extract quantum numbers from the wave packet
    ;   math:       unpack (T,D,M,Q) from base-3 encoding
    ;   programmer: decode 2-bit fields from one byte
    ;   english:    split one byte into four small numbers
    movzx ebx, al
    inc r12

    lea rax, [decode_table]

    ; τ = bits [7:6]  (time / temporal / sequence)
    mov ecx, ebx
    shr ecx, 6
    cmp ecx, 2
    jne .τ
    movsx edi, byte [r12]
    inc r12
    jmp .dec_χ
.τ:
    movsx edi, byte [rax + rcx]

.dec_χ:
    ; χ = bits [5:4]  (distance / spatial / position)
    mov ecx, ebx
    shr ecx, 4
    and ecx, 3
    cmp ecx, 2
    jne .χ
    movsx esi, byte [r12]
    inc r12
    jmp .dec_μ
.χ:
    movsx esi, byte [rax + rcx]

.dec_μ:
    ; μ = bits [3:2]  (mass / substance / content)
    mov ecx, ebx
    shr ecx, 2
    and ecx, 3
    cmp ecx, 2
    jne .μ
    movsx edx, byte [r12]
    inc r12
    jmp .dec_φ
.μ:
    movsx edx, byte [rax + rcx]

.dec_φ:
    ; φ = bits [1:0]  (charge / quality / signal)
    mov ecx, ebx
    and ecx, 3
    cmp ecx, 2
    jne .φ
    movsx ecx, byte [r12]
    inc r12
    jmp .flags
.φ:
    movsx ecx, byte [rax + rcx]

    ; ── flags: how arguments are encoded ─────────────────────
    ;   2 bits per arg: 00=pipeline, 01=u8, 10=u32, 11=u64
    ;   bit 7: dereference arg0.  bit 6: dereference arg1.
.flags:
    movzx r13d, byte [r12]
    inc r12

    ; ── resolve coordinates → operation ──────────────────────
    ;   physics:    quantum numbers → interaction vertex
    ;   math:       coordinates → function
    ;   programmer: opcode → function pointer (JIT on miss)
    ;   english:    look up what these 4 numbers mean
    call ƒ
    mov rbx, rax

    ; ── load arguments ───────────────────────────────────────
    ;   three args from the walk data (inline or pipeline)
    ;   fourth arg is always the pipeline (accumulated result)

    ; arg0 → rdi
    mov eax, r13d
    and eax, 3
    jz .arg0_pipe
    cmp eax, 1
    je .arg0_u8
    cmp eax, 2
    je .arg0_u32
    mov rdi, [r12]
    add r12, 8
    jmp .arg0_ok
.arg0_u8:
    movzx edi, byte [r12]
    inc r12
    jmp .arg0_ok
.arg0_u32:
    mov edi, [r12]
    add r12, 4
    jmp .arg0_ok
.arg0_pipe:
    mov rdi, r15
.arg0_ok:
    test r13d, 0x80
    jz .arg0_deref
    mov rdi, [rdi]
.arg0_deref:

    ; arg1 → rsi
    mov eax, r13d
    shr eax, 2
    and eax, 3
    jz .arg1_pipe
    cmp eax, 1
    je .arg1_u8
    cmp eax, 2
    je .arg1_u32
    mov rsi, [r12]
    add r12, 8
    jmp .arg1_ok
.arg1_u8:
    movzx esi, byte [r12]
    inc r12
    jmp .arg1_ok
.arg1_u32:
    mov esi, [r12]
    add r12, 4
    jmp .arg1_ok
.arg1_pipe:
    mov rsi, r15
.arg1_ok:
    test r13d, 0x40
    jz .arg1_deref
    mov rsi, [rsi]
.arg1_deref:

    ; arg2 → rdx
    mov eax, r13d
    shr eax, 4
    and eax, 3
    jz .arg2_pipe
    cmp eax, 1
    je .arg2_u8
    cmp eax, 2
    je .arg2_u32
    mov rdx, [r12]
    add r12, 8
    jmp .arg2_ok
.arg2_u8:
    movzx edx, byte [r12]
    inc r12
    jmp .arg2_ok
.arg2_u32:
    mov edx, [r12]
    add r12, 4
    jmp .arg2_ok
.arg2_pipe:
    mov rdx, r15
.arg2_ok:

    mov rcx, r15                   ; arg3 = pipeline (always)

    ; ── execute ──────────────────────────────────────────────
    ;   physics:    the interaction happens
    ;   math:       evaluate the term
    ;   programmer: call the JIT'd function
    ;   english:    do the thing
    call rbx
    mov r15, rax                   ; result → pipeline
    mov [ρ], rax                   ; store last result
    jmp .next

; ── walker control ───────────────────────────────────────────
;   physics:    superposition branching (conditional path selection)
;   math:       piecewise evaluation (skip terms based on value)
;   programmer: conditional jump / loop
;   english:    skip ahead or go back depending on the result
.control:
    mov ebx, [r12+1]

    cmp al, 0xFE
    je .skip_z
    cmp al, 0xFD
    je .skip_nz

    ; loop_back: rewind if pipeline ≠ 0
    test r15, r15
    jz .skip_done
    sub r12, rbx
    jmp .next

.skip_z:                              ; skip if pipeline = 0
    test r15, r15
    jnz .skip_done
    add r12, rbx
    jmp .skip_done

.skip_nz:                              ; skip if pipeline ≠ 0
    test r15, r15
    jz .skip_done
    add r12, rbx

.skip_done:
    add r12, 5
    jmp .next

; ── end ──────────────────────────────────────────────────────
;   physics:    wave function fully collapsed
;   math:       polynomial fully evaluated
;   programmer: interpreter returns
;   english:    finished, here's the answer
.done:
    mov rax, r15
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret


; ── decode table ─────────────────────────────────────────────
;   2-bit field → signed coordinate
;   00 → 0    01 → +1    10 → extended    11 → -1

section .rodata

decode_table:
    db  0
    db  1
    db  0
    db -1
