; ƒ — νόμος (nomos: the law). Sealed.
;
;   ƒ(T, D, M, Q) = T^a · D^b · M^c · Q^d
;
;   physics:    the fundamental law — quantum numbers in, interaction out
;   math:       coordinate → function resolver with lazy materialization
;   programmer: JIT compiler — 4 integers in, x86 function pointer out
;   english:    give it 4 numbers, it figures out what operation that means
;
; Even coordinates = atoms (pure math, monomial evaluation)
; Odd coordinates  = bonds (forces, the 16 patterns of control flow)
;
; On a trit processor this dissolves. The walks stay identical.


; ── data ──────────────────────────────────────────────────────

section .data

HTSIZE equ 4096
HTMASK equ (HTSIZE - 1)

ht_ptr: times HTSIZE dq 0                 ; cached function pointers
ht_key: times HTSIZE dq 0                 ; cached coordinate keys

global code_cursor
code_cursor: dq 0

jit_lock: dd 0

global ρ
ρ: dq 0


; ── BSS ───────────────────────────────────────────────────────

section .bss
global code_base
align 4096
code_base: resb 262144                    ; 256KB JIT code buffer


; ── text ──────────────────────────────────────────────────────

section .text
bits 64

extern ψ


; ══════════════════════════════════════════════════════════════
; ƒ — resolve coordinates to function
;
; edi=T, esi=D, edx=M, ecx=Q → rax = function pointer
; Thread-safe: spinlock serializes JIT emission.
; ══════════════════════════════════════════════════════════════

global ƒ
ƒ:
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; save coordinates (sign-extend for safety)
    movsx r12d, di                         ; T
    movsx r13d, si                         ; D
    movsx r14d, dx                         ; M
    movsx r15d, cx                         ; Q

    ; pack key: 4 × i16 → u64
    movzx rax, r12w
    movzx rbx, r13w
    shl rbx, 16
    or rax, rbx
    movzx rbx, r14w
    shl rbx, 32
    or rax, rbx
    movzx rbx, r15w
    shl rbx, 48
    or rax, rbx
    mov r8, rax                            ; r8 = key

    ; hash
    imul rax, r8, 2654435761
    shr rax, 16
    and eax, HTMASK
    mov r9d, eax                           ; r9 = table index

    ; cache check
    lea r10, [ht_key]
    cmp [r10 + r9*8], r8
    jne .skip
    lea r10, [ht_ptr]
    mov rax, [r10 + r9*8]
    test rax, rax
    jnz .done

.skip:                                        ; cache miss
    mov eax, 1
.loop:                                        ; spinlock
    xchg [jit_lock], eax
    test eax, eax
    jz .⊢
    pause
    jmp .loop

.⊢:                                        ; locked — double-check
    lea r10, [ht_key]
    cmp [r10 + r9*8], r8
    jne .gen
    lea r10, [ht_ptr]
    mov rax, [r10 + r9*8]
    test rax, rax
    jnz .⊣

.gen:                                        ; emit JIT code
    mov rbx, [code_cursor]

    push r8                                ; key
    push r9                                ; hash index
    push rbx                               ; function start address

    ; classify: all even = atom, any odd = bond
    mov eax, r12d
    or eax, r13d
    or eax, r14d
    or eax, r15d
    test eax, 1
    jnz .β

    call ε_atom
    jmp .gen_ω

.β:                                        ; bond path
    call ε_bond

.gen_ω:                                      ; emission done
    mov byte [rbx], 0xC3                   ; ret
    inc rbx

    pop rax                                ; function start
    pop r9                                 ; hash index
    pop r8                                 ; key

    ; cache
    lea r10, [ht_ptr]
    mov [r10 + r9*8], rax
    lea r10, [ht_key]
    mov [r10 + r9*8], r8

    mov [code_cursor], rbx

.⊣:                                        ; unlock
    mov dword [jit_lock], 0

.done:                                        ; done
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret


; ══════════════════════════════════════════════════════════════
; ε_atom — all coordinates even
;
; Emits a call to μ with exponents as immediates.
; r12-r15 = coordinates, rbx = emit pointer.
; ══════════════════════════════════════════════════════════════

ε_atom:
    ; identity check (all zero → return arg0)
    mov eax, r12d
    or eax, r13d
    or eax, r14d
    or eax, r15d
    jnz .gen

    ; emit: mov rax, rdi (48 89 F8)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF8
    add rbx, 3
    ret

.gen:
    ; compute exponents at emit time (coord / 2)
    mov eax, r12d
    sar eax, 1
    mov r8d, eax                           ; T_exp
    mov eax, r13d
    sar eax, 1
    mov r9d, eax                           ; D_exp
    mov eax, r14d
    sar eax, 1
    mov r10d, eax                          ; M_exp
    mov eax, r15d
    sar eax, 1
    mov r11d, eax                          ; Q_exp

    ; emit: mov r8d, T_exp  (41 B8 xx xx xx xx)
    mov byte [rbx], 0x41
    mov byte [rbx+1], 0xB8
    mov dword [rbx+2], r8d
    add rbx, 6

    ; emit: mov r9d, D_exp  (41 B9 xx xx xx xx)
    mov byte [rbx], 0x41
    mov byte [rbx+1], 0xB9
    mov dword [rbx+2], r9d
    add rbx, 6

    ; emit: mov r10d, M_exp (41 BA xx xx xx xx)
    mov byte [rbx], 0x41
    mov byte [rbx+1], 0xBA
    mov dword [rbx+2], r10d
    add rbx, 6

    ; emit: mov r11d, Q_exp (41 BB xx xx xx xx)
    mov byte [rbx], 0x41
    mov byte [rbx+1], 0xBB
    mov dword [rbx+2], r11d
    add rbx, 6

    ; emit: mov eax, μ (B8 xx xx xx xx)
    mov byte [rbx], 0xB8
    mov eax, μ
    mov dword [rbx+1], eax
    add rbx, 5

    ; emit: call rax (FF D0)
    mov byte [rbx], 0xFF
    mov byte [rbx+1], 0xD0
    add rbx, 2

    ret


; ══════════════════════════════════════════════════════════════
; μ — runtime evaluator for T^a * D^b * M^c * Q^d
;
; Called by JIT'd atom functions at walk time.
; rdi=T_val, rsi=D_val, rdx=M_val, rcx=Q_val
; r8d=T_exp, r9d=D_exp, r10d=M_exp, r11d=Q_exp
; Returns result in rax.
; ══════════════════════════════════════════════════════════════

μ:
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; save dimension values (div clobbers rdx)
    mov r12, rdi                           ; T_val
    mov r13, rsi                           ; D_val
    mov r14, rdx                           ; M_val
    mov r15, rcx                           ; Q_val

    mov rax, 1                             ; accumulator

    ; ── T dimension ──
    test r8d, r8d
    jz .D_
    jns .T_mul_init
    neg r8d
    mov ebx, r8d
.T_div:
    xor edx, edx
    div r12
    dec ebx
    jnz .T_div
    jmp .D_
.T_mul_init:
    mov ebx, r8d
.T_mul:
    imul rax, r12
    dec ebx
    jnz .T_mul

.D_:
    ; ── D dimension ──
    test r9d, r9d
    jz .M_
    jns .D_mul_init
    neg r9d
    mov ebx, r9d
.D_div:
    xor edx, edx
    div r13
    dec ebx
    jnz .D_div
    jmp .M_
.D_mul_init:
    mov ebx, r9d
.D_mul:
    imul rax, r13
    dec ebx
    jnz .D_mul

.M_:
    ; ── M dimension ──
    test r10d, r10d
    jz .Q_
    jns .M_mul_init
    neg r10d
    mov ebx, r10d
.M_div:
    xor edx, edx
    div r14
    dec ebx
    jnz .M_div
    jmp .Q_
.M_mul_init:
    mov ebx, r10d
.M_mul:
    imul rax, r14
    dec ebx
    jnz .M_mul

.Q_:
    ; ── Q dimension ──
    test r11d, r11d
    jz .D_one
    jns .Q_mul_init
    neg r11d
    mov ebx, r11d
.Q_div:
    xor edx, edx
    div r15
    dec ebx
    jnz .Q_div
    jmp .D_one
.Q_mul_init:
    mov ebx, r11d
.Q_mul:
    imul rax, r15
    dec ebx
    jnz .Q_mul

.D_one:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret


; ══════════════════════════════════════════════════════════════
; ε_bond — any odd coordinate
;
; Computes force bitmask, dispatches to handler.
; P=bit0 (T odd), C=bit1 (D odd), R=bit2 (M odd), W=bit3 (Q odd)
; ══════════════════════════════════════════════════════════════

ε_bond:
    xor eax, eax
    test r12d, 1
    jz .no_read
    or eax, 1                              ; P
.no_read:
    test r13d, 1
    jz .no_call
    or eax, 2                              ; C
.no_call:
    test r14d, 1
    jz .no_loop
    or eax, 4                              ; R
.no_loop:
    test r15d, 1
    jz .no_test
    or eax, 8                              ; W
.no_test:
    lea rcx, [bond_table]
    jmp [rcx + rax*8]


section .rodata
align 8
bond_table:
    dq ε_wave                             ;  0: ∅  identity (wave)
    dq ε_read                             ;  1: π  read/write
    dq ε_call                             ;  2: ∘  call/return
    dq ε_fold                             ;  3: π∘  fold (read then call)
    dq ε_loop                             ;  4: ∮  loop/increment
    dq ε_copy                             ;  5: π∮  copy/fill
    dq ε_fix                             ;  6: ∘∮  fix (call N times)
    dq ε_foldl                             ;  7: π∘∮  foldl (fold over array)
    dq ε_test                             ;  8: δ  test/guard + magnitudes
    dq ε_filter                             ;  9: πδ  filter/port/add
    dq ε_maybe                            ; 10: ∘δ  maybe (conditional call)
    dq ε_map                            ; 11: π∘δ  map (apply to each)
    dq ε_until                            ; 12: ∮δ  until (scan to zero)
    dq ε_scan                            ; 13: π∮δ  scan (accumulate)
    dq ε_bind                            ; 14: ∘∮δ  bind (the event loop)
    dq ε_hylo                            ; 15: π∘∮δ  hylo (full pipeline)


section .text


; ── ∅ identity (wave) ────────────────────────────────

ε_wave:
    ; emit: mov rax, rdi (return arg0)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF8
    add rbx, 3
    ret


; ── π read/write ────────────────────────────────
;
;  π = read from [arg0].  π̄ = write arg1 to [arg0].
;  Width from |T|: 1=qword, 3=byte, 7=dword.

ε_read:
    test r12d, r12d
    js .write

    ; +P (read)
    mov eax, r12d                          ; T magnitude
    cmp eax, 3
    je .read_byte
    cmp eax, 7
    je .read_dword

    ; qword: mov rax, [rdi]  (48 8B 07)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x8B
    mov byte [rbx+2], 0x07
    add rbx, 3
    ret

.read_byte:
    ; movzx eax, byte [rdi]  (0F B6 07)
    mov byte [rbx], 0x0F
    mov byte [rbx+1], 0xB6
    mov byte [rbx+2], 0x07
    add rbx, 3
    ret

.read_dword:
    ; mov eax, [rdi]  (8B 07)
    mov byte [rbx], 0x8B
    mov byte [rbx+1], 0x07
    add rbx, 2
    ret

.write:
    ; -P (write)
    mov eax, r12d
    neg eax                                ; |T|
    cmp eax, 3
    je .write_byte
    cmp eax, 7
    je .write_dword

    ; qword: mov [rdi], rsi  (48 89 37)  + return value
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0x37
    add rbx, 3
    jmp .write_ret

.write_byte:
    ; mov [rdi], sil  (40 88 37)
    mov byte [rbx], 0x40
    mov byte [rbx+1], 0x88
    mov byte [rbx+2], 0x37
    add rbx, 3
    jmp .write_ret

.write_dword:
    ; mov [rdi], esi  (89 37)
    mov byte [rbx], 0x89
    mov byte [rbx+1], 0x37
    add rbx, 2

.write_ret:
    ; return written value: mov rax, rsi (48 89 F0)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF0
    add rbx, 3
    ret


; ── ∘ call/return ────────────────────────────────
;
;  ∘ = invoke sub-walk (recursive ψ)
;  ∘̄ = return arg0 (curry)

ε_call:
    test r13d, r13d
    js .curry

    ; +C: mov eax, ψ; call rax
    mov byte [rbx], 0xB8                   ; mov eax, imm32
    mov eax, ψ
    mov dword [rbx+1], eax
    add rbx, 5
    mov byte [rbx], 0xFF                   ; call rax (FF D0)
    mov byte [rbx+1], 0xD0
    add rbx, 2
    ret

.curry:
    ; -C: mov rax, rdi (48 89 F8)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF8
    add rbx, 3
    ret


; ── ∮ loop/increment ───────────────────────────
;
;  ∮ = drain (countdown) — stub for now
;  ∮̄ = increment (arg0 + 1)

ε_loop:
    test r14d, r14d
    js .inc

    ; +R: stub (return 0)
    mov byte [rbx], 0x31                   ; xor eax, eax
    mov byte [rbx+1], 0xC0
    add rbx, 2
    ret

.inc:
    ; -R: lea rax, [rdi+1]  (48 8D 47 01)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x8D
    mov byte [rbx+2], 0x47
    mov byte [rbx+3], 0x01
    add rbx, 4
    ret


; ── π∮ copy/fill ──────────────────────────────
;
;  π∮ = copy (memcpy): rdi=dest, rsi=src, edx=count
;  π̄∮̄ = fill (memset): rdi=dest, sil=byte/esi=dword, edx=count
;  Width from |M|: 1=byte, 3=dword, >=5=qword

ε_copy:
    test r12d, r12d
    js .fill

    ; ── slurp (memcpy) ──
    ; emit: mov ecx, edx; cld; rep movs{b,d,q}
    mov byte [rbx], 0x89                   ; mov ecx, edx (89 D1)
    mov byte [rbx+1], 0xD1
    add rbx, 2
    mov byte [rbx], 0xFC                   ; cld
    inc rbx

    mov eax, r14d
    test eax, eax
    jns .fwd
    neg eax
.fwd:
    cmp eax, 5
    jge .fwd_q
    cmp eax, 3
    jge .fwd_d
    ; byte: F3 A4
    mov byte [rbx], 0xF3
    mov byte [rbx+1], 0xA4
    add rbx, 2
    jmp .fwd_ret
.fwd_d:
    ; dword: F3 A5
    mov byte [rbx], 0xF3
    mov byte [rbx+1], 0xA5
    add rbx, 2
    jmp .fwd_ret
.fwd_q:
    ; qword: F3 48 A5
    mov byte [rbx], 0xF3
    mov byte [rbx+1], 0x48
    mov byte [rbx+2], 0xA5
    add rbx, 3
.fwd_ret:
    ; return 0
    mov byte [rbx], 0x31
    mov byte [rbx+1], 0xC0
    add rbx, 2
    ret

.fill:
    ; ── fill (memset) ──
    mov eax, r14d
    neg eax                                ; |M|
    cmp eax, 5
    jge .fill_q
    cmp eax, 3
    jge .fill_d

    ; byte fill: mov al, sil; mov ecx, edx; cld; rep stosb
    mov byte [rbx], 0x40                   ; mov al, sil (40 8A C6)
    mov byte [rbx+1], 0x8A
    mov byte [rbx+2], 0xC6
    add rbx, 3
    mov byte [rbx], 0x89                   ; mov ecx, edx (89 D1)
    mov byte [rbx+1], 0xD1
    add rbx, 2
    mov byte [rbx], 0xFC                   ; cld
    inc rbx
    mov byte [rbx], 0xF3                   ; rep stosb (F3 AA)
    mov byte [rbx+1], 0xAA
    add rbx, 2
    jmp .fill_ret

.fill_d:
    ; dword fill: mov eax, esi; mov ecx, edx; cld; rep stosd
    mov byte [rbx], 0x89                   ; mov eax, esi (89 F0)
    mov byte [rbx+1], 0xF0
    add rbx, 2
    mov byte [rbx], 0x89                   ; mov ecx, edx (89 D1)
    mov byte [rbx+1], 0xD1
    add rbx, 2
    mov byte [rbx], 0xFC                   ; cld
    inc rbx
    mov byte [rbx], 0xF3                   ; rep stosd (F3 AB)
    mov byte [rbx+1], 0xAB
    add rbx, 2
    jmp .fill_ret

.fill_q:
    ; qword fill: mov rax, rsi; mov ecx, edx; cld; rep stosq
    mov byte [rbx], 0x48                   ; mov rax, rsi (48 89 F0)
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF0
    add rbx, 3
    mov byte [rbx], 0x89                   ; mov ecx, edx
    mov byte [rbx+1], 0xD1
    add rbx, 2
    mov byte [rbx], 0xFC                   ; cld
    inc rbx
    mov byte [rbx], 0xF3                   ; rep stosq (F3 48 AB)
    mov byte [rbx+1], 0x48
    mov byte [rbx+2], 0xAB
    add rbx, 3

.fill_ret:
    mov byte [rbx], 0x31                   ; xor eax, eax
    mov byte [rbx+1], 0xC0
    add rbx, 2
    ret


; ── δ test/guard + magnitudes ───────────
;
;  |Q|=1:  +W test (==)        -W guard (cmov)
;  |Q|=3:  +W AND              -W OR
;  |Q|=5:  +W XOR              -W NOT
;  |Q|=7:  +W SHL              -W SHR
;  |Q|=11: +W less-than        -W greater-than
;  |Q|=13: +W negate           -W abs

ε_test:
    mov eax, r15d                          ; Q coordinate
    mov ecx, eax                           ; preserve sign
    test eax, eax
    jns .dispatch
    neg eax
.dispatch:
    ; eax = |Q|, ecx = original Q (sign)

    cmp eax, 1
    je .eq
    cmp eax, 3
    je .and
    cmp eax, 5
    je .xor
    cmp eax, 7
    je .shl
    cmp eax, 11
    je .lt
    cmp eax, 13
    je .neg
    ; unknown magnitude — return 0
    mov byte [rbx], 0x31                   ; xor eax, eax
    mov byte [rbx+1], 0xC0
    add rbx, 2
    ret

.eq:
    test ecx, ecx
    js .guard
    ; +W: test (cmp eq, sete)
    ;   xor eax, eax (31 C0)
    ;   cmp rdi, rsi (48 39 F7)
    ;   sete al      (0F 94 C0)
    mov byte [rbx], 0x31
    mov byte [rbx+1], 0xC0
    mov byte [rbx+2], 0x48
    mov byte [rbx+3], 0x39
    mov byte [rbx+4], 0xF7
    mov byte [rbx+5], 0x0F
    mov byte [rbx+6], 0x94
    mov byte [rbx+7], 0xC0
    add rbx, 8
    ret

.guard:
    ; -W: if arg1 != 0 return arg0, else return pipeline
    ;   mov rax, rcx      (48 89 C8)
    ;   test rsi, rsi     (48 85 F6)
    ;   cmovnz rax, rdi   (48 0F 45 C7)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xC8
    mov byte [rbx+3], 0x48
    mov byte [rbx+4], 0x85
    mov byte [rbx+5], 0xF6
    mov byte [rbx+6], 0x48
    mov byte [rbx+7], 0x0F
    mov byte [rbx+8], 0x45
    mov byte [rbx+9], 0xC7
    add rbx, 10
    ret

.and:
    test ecx, ecx
    js .or
    ; +W|3: AND  →  mov rax, rdi (48 89 F8) + and rax, rsi (48 21 F0)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF8
    mov byte [rbx+3], 0x48
    mov byte [rbx+4], 0x21
    mov byte [rbx+5], 0xF0
    add rbx, 6
    ret
.or:
    ; -W|3: OR  →  mov rax, rdi + or rax, rsi (48 09 F0)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF8
    mov byte [rbx+3], 0x48
    mov byte [rbx+4], 0x09
    mov byte [rbx+5], 0xF0
    add rbx, 6
    ret

.xor:
    test ecx, ecx
    js .not
    ; +W|5: XOR  →  mov rax, rdi + xor rax, rsi (48 31 F0)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF8
    mov byte [rbx+3], 0x48
    mov byte [rbx+4], 0x31
    mov byte [rbx+5], 0xF0
    add rbx, 6
    ret
.not:
    ; -W|5: NOT  →  mov rax, rdi + not rax (48 F7 D0)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF8
    mov byte [rbx+3], 0x48
    mov byte [rbx+4], 0xF7
    mov byte [rbx+5], 0xD0
    add rbx, 6
    ret

.shl:
    test ecx, ecx
    js .shr
    ; +W|7: SHL  →  mov rax, rdi + mov ecx, esi (89 F1) + shl rax, cl (48 D3 E0)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF8
    mov byte [rbx+3], 0x89
    mov byte [rbx+4], 0xF1
    mov byte [rbx+5], 0x48
    mov byte [rbx+6], 0xD3
    mov byte [rbx+7], 0xE0
    add rbx, 8
    ret
.shr:
    ; -W|7: SHR  →  same but shr (48 D3 E8)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF8
    mov byte [rbx+3], 0x89
    mov byte [rbx+4], 0xF1
    mov byte [rbx+5], 0x48
    mov byte [rbx+6], 0xD3
    mov byte [rbx+7], 0xE8
    add rbx, 8
    ret

.lt:
    test ecx, ecx
    js .gt
    ; +W|11: less-than  →  xor eax, eax + cmp rdi, rsi + setl al (0F 9C C0)
    mov byte [rbx], 0x31
    mov byte [rbx+1], 0xC0
    mov byte [rbx+2], 0x48
    mov byte [rbx+3], 0x39
    mov byte [rbx+4], 0xF7
    mov byte [rbx+5], 0x0F
    mov byte [rbx+6], 0x9C
    mov byte [rbx+7], 0xC0
    add rbx, 8
    ret
.gt:
    ; -W|11: greater-than  →  same but setg (0F 9F C0)
    mov byte [rbx], 0x31
    mov byte [rbx+1], 0xC0
    mov byte [rbx+2], 0x48
    mov byte [rbx+3], 0x39
    mov byte [rbx+4], 0xF7
    mov byte [rbx+5], 0x0F
    mov byte [rbx+6], 0x9F
    mov byte [rbx+7], 0xC0
    add rbx, 8
    ret

.neg:
    test ecx, ecx
    js .abs
    ; +W|13: negate  →  mov rax, rdi + neg rax (48 F7 D8)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF8
    mov byte [rbx+3], 0x48
    mov byte [rbx+4], 0xF7
    mov byte [rbx+5], 0xD8
    add rbx, 6
    ret
.abs:
    ; -W|13: abs  →  mov rax, rdi + neg rax + cmovs rax, rdi (48 0F 48 C7)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF8
    mov byte [rbx+3], 0x48
    mov byte [rbx+4], 0xF7
    mov byte [rbx+5], 0xD8
    mov byte [rbx+6], 0x48
    mov byte [rbx+7], 0x0F
    mov byte [rbx+8], 0x48
    mov byte [rbx+9], 0xC7
    add rbx, 10
    ret


; ── πδ filter / port / add ─────────────────
;
;  πδ: filter (min)          +P +W|3: max
;  πδ̄: port_read             π̄δ: port_write
;  π̄δ̄: add                   π̄δ̄|3: subtract

ε_filter:
    mov eax, r15d                          ; Q
    mov ecx, eax                           ; preserve sign
    test eax, eax
    jns .dispatch
    neg eax
.dispatch:
    ; eax = |Q|, ecx = signed Q

    test r12d, r12d
    js .write_side

    ; ── +P (read side) ──
    test ecx, ecx
    js .port_read

    ; +P +W: filter = min(a, b)
    cmp eax, 3
    je .max

    ; min: mov rax, rdi + cmp rdi, rsi + cmovg rax, rsi
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF8
    mov byte [rbx+3], 0x48
    mov byte [rbx+4], 0x39
    mov byte [rbx+5], 0xF7
    mov byte [rbx+6], 0x48
    mov byte [rbx+7], 0x0F
    mov byte [rbx+8], 0x4F                 ; cmovg
    mov byte [rbx+9], 0xC6
    add rbx, 10
    ret

.max:
    ; πδ|3: max  →  same but cmovl (0F 4C)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF8
    mov byte [rbx+3], 0x48
    mov byte [rbx+4], 0x39
    mov byte [rbx+5], 0xF7
    mov byte [rbx+6], 0x48
    mov byte [rbx+7], 0x0F
    mov byte [rbx+8], 0x4C                 ; cmovl
    mov byte [rbx+9], 0xC6
    add rbx, 10
    ret

.port_read:
    ; +P -W: port_read
    ;   movzx edx, di (0F B7 D7) + in al, dx (EC) + movzx eax, al (0F B6 C0)
    mov byte [rbx], 0x0F
    mov byte [rbx+1], 0xB7
    mov byte [rbx+2], 0xD7
    mov byte [rbx+3], 0xEC
    mov byte [rbx+4], 0x0F
    mov byte [rbx+5], 0xB6
    mov byte [rbx+6], 0xC0
    add rbx, 7
    ret

.write_side:
    ; ── -P (write side) ──
    test ecx, ecx
    js .add

    ; π̄δ: port_write
    ;   movzx edx, di (0F B7 D7) + mov al, sil (40 8A C6) + out dx, al (EE)
    ;   + xor eax, eax (31 C0)
    mov byte [rbx], 0x0F
    mov byte [rbx+1], 0xB7
    mov byte [rbx+2], 0xD7
    mov byte [rbx+3], 0x40
    mov byte [rbx+4], 0x8A
    mov byte [rbx+5], 0xC6
    mov byte [rbx+6], 0xEE
    mov byte [rbx+7], 0x31
    mov byte [rbx+8], 0xC0
    add rbx, 9
    ret

.add:
    ; -P -W: add/subtract
    cmp eax, 3
    je .sub

    ; add: lea rax, [rdi+rsi] (48 8D 04 37)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x8D
    mov byte [rbx+2], 0x04
    mov byte [rbx+3], 0x37
    add rbx, 4
    ret

.sub:
    ; -P-W|3: subtract  →  mov rax, rdi + sub rax, rsi (48 29 F0)
    mov byte [rbx], 0x48
    mov byte [rbx+1], 0x89
    mov byte [rbx+2], 0xF8
    mov byte [rbx+3], 0x48
    mov byte [rbx+4], 0x29
    mov byte [rbx+5], 0xF0
    add rbx, 6
    ret


; ── π∘ fold (read then call) ────────────────────────────────────
;
;  Read [arg0], then call ψ(arg1, arg2).
;  The read value is stored in ρ for the sub-walk.
;  Returns the sub-walk's result.

ε_fold:
    mov byte [rbx], 0xB8                   ; mov eax, φ
    mov eax, φ
    mov dword [rbx+1], eax
    add rbx, 5
    mov byte [rbx], 0xFF                   ; call rax
    mov byte [rbx+1], 0xD0
    add rbx, 2
    ret

φ:
    push rbx
    push r12
    mov rbx, rsi                           ; save walk ptr
    mov r12d, edx                          ; save walk len
    mov rax, [rdi]                         ; π: read from arg0
    mov [ρ], rax                       ; accessible via ρ
    mov rdi, rbx                           ; ∘: call ψ
    mov esi, r12d
    call ψ
    pop r12
    pop rbx
    ret


; ── ∘∮ fix (call N times) ────────────────────────────────────
;
;  Call ψ(arg0, arg1) arg2 times.
;  Returns last sub-walk result.

ε_fix:
    mov byte [rbx], 0xB8                   ; mov eax, ν
    mov eax, ν
    mov dword [rbx+1], eax
    add rbx, 5
    mov byte [rbx], 0xFF                   ; call rax
    mov byte [rbx+1], 0xD0
    add rbx, 2
    ret

ν:
    push rbx
    push r12
    push r13
    mov rbx, rdi                           ; walk ptr
    mov r12d, esi                          ; walk len
    mov r13d, edx                          ; count
    xor eax, eax
    test r13d, r13d
    jz .done
.loop:
    mov rdi, rbx
    mov esi, r12d
    call ψ
    dec r13d
    jnz .loop
.done:
    pop r13
    pop r12
    pop rbx
    ret


; ── π∘∮ foldl (fold over array) ────────────────────────────────
;
;  Left fold: read successive qwords from [arg0], call
;  ψ(arg1, arg2) for each. Count from pipeline.
;  Each element stored in ρ before the call.
;  Returns last sub-walk result.

ε_foldl:
    mov byte [rbx], 0xB8                   ; mov eax, Φ
    mov eax, Φ
    mov dword [rbx+1], eax
    add rbx, 5
    mov byte [rbx], 0xFF                   ; call rax
    mov byte [rbx+1], 0xD0
    add rbx, 2
    ret

Φ:
    push rbx
    push r12
    push r13
    push r14
    mov r13, rdi                           ; array ptr
    mov rbx, rsi                           ; walk ptr
    mov r12d, edx                          ; walk len
    mov r14d, ecx                          ; count (from pipeline)
    xor eax, eax
    test r14d, r14d
    jz .done
.loop:
    mov rax, [r13]                         ; π: read element
    mov [ρ], rax
    add r13, 8                             ; ∮: advance
    mov rdi, rbx                           ; ∘: call
    mov esi, r12d
    call ψ
    dec r14d
    jnz .loop
.done:
    pop r14
    pop r13
    pop r12
    pop rbx
    ret


; ── ∘δ maybe (conditional call) ─────────────────────────────────
;
;  Conditional call: if arg1 != 0, call ψ(arg0, arg2).
;  Else return 0.

ε_maybe:
    mov byte [rbx], 0xB8                   ; mov eax, δ
    mov eax, δ
    mov dword [rbx+1], eax
    add rbx, 5
    mov byte [rbx], 0xFF                   ; call rax
    mov byte [rbx+1], 0xD0
    add rbx, 2
    ret

δ:
    test rsi, rsi                          ; δ: test condition
    jz .skip
    push rbx
    mov ebx, edx                           ; save walk len
    mov esi, ebx                           ; walk len
    call ψ                       ; ∘: call (rdi already = walk ptr)
    pop rbx
    ret
.skip:
    xor eax, eax
    ret


; ── π∘δ map (apply to each) ─────────────────────────────────
;
;  Apply function to each element that passes test.
;  Read qwords from [arg0], for each nonzero element call
;  ψ(arg1, arg2). Count from pipeline.
;  Returns last sub-walk result.

ε_map:
    mov byte [rbx], 0xB8                   ; mov eax, σ
    mov eax, σ
    mov dword [rbx+1], eax
    add rbx, 5
    mov byte [rbx], 0xFF                   ; call rax
    mov byte [rbx+1], 0xD0
    add rbx, 2
    ret

σ:
    push rbx
    push r12
    push r13
    push r14
    mov r13, rdi                           ; array ptr
    mov rbx, rsi                           ; walk ptr
    mov r12d, edx                          ; walk len
    mov r14d, ecx                          ; count (from pipeline)
    xor eax, eax
    test r14d, r14d
    jz .done
.loop:
    mov rax, [r13]                         ; π: read
    test rax, rax                          ; δ: test
    jz .next
    mov [ρ], rax
    mov rdi, rbx                           ; ∘: call
    mov esi, r12d
    call ψ
.next:
    add r13, 8
    dec r14d
    jnz .loop
.done:
    pop r14
    pop r13
    pop r12
    pop rbx
    ret


; ── ∮δ until (scan to zero) ────────────────────────────
;
;  Scan forward from [arg0] while bytes are nonzero.
;  Returns count of nonzero bytes (like strlen).

ε_until:
    mov byte [rbx], 0xB8                   ; mov eax, τ
    mov eax, τ
    mov dword [rbx+1], eax
    add rbx, 5
    mov byte [rbx], 0xFF                   ; call rax
    mov byte [rbx+1], 0xD0
    add rbx, 2
    ret

τ:
    xor eax, eax                           ; count = 0
.loop:
    cmp byte [rdi + rax], 0                ; δ: test
    jz .done
    inc rax                                ; ∮: advance
    jmp .loop
.done:
    ret


; ── π∮δ scan (accumulate) ────────────────────────────────
;
;  Running accumulation: sum qwords from [arg0], count in arg1.
;  Returns the accumulated sum.

ε_scan:
    mov byte [rbx], 0xB8                   ; mov eax, Σ
    mov eax, Σ
    mov dword [rbx+1], eax
    add rbx, 5
    mov byte [rbx], 0xFF                   ; call rax
    mov byte [rbx+1], 0xD0
    add rbx, 2
    ret

Σ:
    xor eax, eax                           ; accumulator = 0
    test esi, esi
    jz .done
    mov ecx, esi                           ; count
.loop:
    add rax, [rdi]                         ; π: read + accumulate
    add rdi, 8                             ; ∮: advance
    dec ecx                                ; δ: test count
    jnz .loop
.done:
    ret


; ── ∘∮δ bind (the event loop) ────────────────────────────────
;
;  THE EVENT LOOP. Call ψ(arg0, arg1), test result,
;  loop while nonzero. Returns 0 when the sub-walk signals done.
;
;  This IS the drain. bind = call + loop + test. No read (no P).
;  The scheduler calls consumers, loops, checks empty.
;  The consumer does the reading.

ε_bind:
    mov byte [rbx], 0xB8                   ; mov eax, β
    mov eax, β
    mov dword [rbx+1], eax
    add rbx, 5
    mov byte [rbx], 0xFF                   ; call rax
    mov byte [rbx+1], 0xD0
    add rbx, 2
    ret

β:
    push rbx
    push r12
    mov rbx, rdi                           ; walk ptr
    mov r12d, esi                          ; walk len
.loop:
    mov rdi, rbx                           ; ∘: call
    mov esi, r12d
    call ψ
    test rax, rax                          ; δ: test
    jnz .loop                         ; ∮: loop if nonzero
    ; result is 0 — sub-walk signaled done
    pop r12
    pop rbx
    ret


; ── π∘∮δ hylo (full pipeline) ──────────────────────────────
;
;  Full pipeline: read, call, loop, test. All four forces.
;  Read successive qwords from [arg0]. For each nonzero value,
;  store in ρ and call ψ(arg1, arg2). Stop on
;  zero element. Returns last sub-walk result.
;
;  hylo = unfold + transform + fold. The universal pipeline.

ε_hylo:
    mov byte [rbx], 0xB8                   ; mov eax, η
    mov eax, η
    mov dword [rbx+1], eax
    add rbx, 5
    mov byte [rbx], 0xFF                   ; call rax
    mov byte [rbx+1], 0xD0
    add rbx, 2
    ret

η:
    push rbx
    push r12
    push r13
    mov r13, rdi                           ; data ptr
    mov rbx, rsi                           ; walk ptr
    mov r12d, edx                          ; walk len
.loop:
    mov rax, [r13]                         ; π: read
    test rax, rax                          ; δ: test
    jz .done
    mov [ρ], rax
    add r13, 8                             ; ∮: advance
    mov rdi, rbx                           ; ∘: call
    mov esi, r12d
    call ψ
    jmp .loop
.done:
    xor eax, eax
    pop r13
    pop r12
    pop rbx
    ret
