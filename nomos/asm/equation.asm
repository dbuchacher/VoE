; ═══════════════════════════════════════════════════════════════════
; equation.asm — f(a,b,c,d) = T^a * D^b * M^c * Q^d
;
; The equation. Pure JIT + walker. Sealed.
;
; Contains:
;   get_atom(t,d,m,q)  → cached JIT code pointer
;   emit_function       → generates x86-64 from coordinates
;   walker_wave(rdi,rsi) → reads wave byte walks, executes via get_atom
;   lattice_start       → seeds 16 bonds, walks genesis
;
; The equation exists because CPUs are manufactured around an ISA
; (x86-64). It translates lattice coordinates into machine code.
; On a chip that natively evaluated T^a*D^b*M^c*Q^d and read wave
; bytes directly, this file dissolves. The walks stay identical.
;
; Everything else is walks (lat/). No system.asm. No third file.
; ═══════════════════════════════════════════════════════════════════


; ─── data ──────────────────────────────────────────────────────

section .data

; hash table (4096 entries)
HTSIZE  equ 4096
HTMASK  equ HTSIZE - 1
ht_ptr: times HTSIZE dq 0
ht_key: times HTSIZE dq 0

global code_cursor
code_cursor: dq 0

; walker accumulator — last result, readable by walks
global w_acc
w_acc: dq 0

; ─── bss ───────────────────────────────────────────────────────

section .bss
align 4096
global code_base
code_base: resb 262144                    ; 256KB code buffer


; ═══════════════════════════════════════════════════════════════════
; LATTICE START — seeds the ground state, runs first test
; ═══════════════════════════════════════════════════════════════════

section .text
bits 64

global lattice_start

lattice_start:
    ; init code generation buffer
    lea rax, [code_base]
    mov [code_cursor], rax

    ; ── seed the 16 bonds (the forces / ground state) ──
    ; 15 force patterns (bitmask 1-15) with positive signs
    lea rsi, [bond_seeds]
    mov ecx, 15
.seed:
    push rcx
    push rsi
    movsx edi, byte [rsi]
    movsx esi, byte [rsi+1]
    movsx edx, byte [rsi+2]
    movsx ecx, byte [rsi+3]
    call get_atom
    pop rsi
    pop rcx
    add rsi, 4
    dec ecx
    jnz .seed

    ; ── walk genesis (runs once, returns) ──
    lea rdi, [main_walk]
    mov esi, [main_len]
    call walker_wave

    ; ── genesis returned. enter the market. ──
    ; walk the keyboard poll (first available work)
    ; TODO: bind heartbeat — find fullest trace instead of hardcoding
    lea rdi, [kbd_walk]
    mov esi, [kbd_walk_len]
    call walker_wave

    ; if walk returns (it shouldn't — loop_back), spin
.spin:
    pause
    jmp .spin


; ═══════════════════════════════════════════════════════════════════
; WALKER — reads wave byte walks, executes via get_atom
;
; Format per record:
;   byte 0: wave byte (2 bits per dim, [7:6]=T [5:4]=D [3:2]=M [1:0]=Q)
;   byte 1: flags ([reserved:2][arg2:2][arg1:2][arg0:2])
;            00=pipeline  01=u8  10=u32  11=u64
;   bytes 2+: inline args (variable length per flags)
;
; Walker control (wave byte >= 0xFC):
;   0xFE + u32: skip_z (skip N bytes forward if pipeline == 0)
;   0xFD + u32: skip_nz (skip N bytes forward if pipeline != 0)
;   0xFC + u32: loop_back (rewind N bytes if pipeline != 0)
;
; Input: rdi = walk data pointer, esi = walk length (bytes)
; ═══════════════════════════════════════════════════════════════════

global walker_wave
walker_wave:
    push rbx
    push r12
    push r13
    push r14

    mov r12, rdi              ; walk data pointer
    mov r13d, esi             ; length (temp)
    lea r14, [r12 + r13]      ; end pointer
    xor r15d, r15d            ; pipeline = 0

.ww_next:
    cmp r12, r14
    jge .ww_done

    movzx eax, byte [r12]    ; read wave byte

    ; ── walker control: >= 0xFC ──
    cmp al, 0xFC
    jae .ww_control

    ; ── regular bond/atom ──
    movzx ebx, al             ; wave byte → ebx
    inc r12                    ; past wave byte

    ; decode wave byte → 4 coords
    ; 00=0  01=+1  11=-1  10=extended (read signed i8 from walk)
    lea rax, [decode_2bit]

    ; T = bits [7:6]
    mov ecx, ebx
    shr ecx, 6
    cmp ecx, 2
    jne .dec_t_lut
    movsx edi, byte [r12]     ; extended: signed i8
    inc r12
    jmp .dec_d
.dec_t_lut:
    movsx edi, byte [rax + rcx]
.dec_d:
    ; D = bits [5:4]
    mov ecx, ebx
    shr ecx, 4
    and ecx, 3
    cmp ecx, 2
    jne .dec_d_lut
    movsx esi, byte [r12]
    inc r12
    jmp .dec_m
.dec_d_lut:
    movsx esi, byte [rax + rcx]
.dec_m:
    ; M = bits [3:2]
    mov ecx, ebx
    shr ecx, 2
    and ecx, 3
    cmp ecx, 2
    jne .dec_m_lut
    movsx edx, byte [r12]
    inc r12
    jmp .dec_q
.dec_m_lut:
    movsx edx, byte [rax + rcx]
.dec_q:
    ; Q = bits [1:0]
    mov ecx, ebx
    and ecx, 3
    cmp ecx, 2
    jne .dec_q_lut
    movsx ecx, byte [r12]
    inc r12
    jmp .dec_flags
.dec_q_lut:
    movsx ecx, byte [rax + rcx]

.dec_flags:
    ; flags byte follows wave byte + any extension bytes
    movzx r13d, byte [r12]
    inc r12

    ; get_atom(edi=T, esi=D, edx=M, ecx=Q) → rax = function pointer
    call get_atom
    mov rbx, rax               ; fn ptr → rbx
    ; r12, r13, r14, r15 preserved (callee-saved, get_atom saves them)

    ; ── read arg0 → rdi ──
    mov eax, r13d
    and eax, 3
    jz .a0_pipe
    cmp eax, 1
    je .a0_u8
    cmp eax, 2
    je .a0_u32
    mov rdi, [r12]             ; u64
    add r12, 8
    jmp .a0_done
.a0_u8:
    movzx edi, byte [r12]
    inc r12
    jmp .a0_done
.a0_u32:
    mov edi, [r12]
    add r12, 4
    jmp .a0_done
.a0_pipe:
    mov rdi, r15
.a0_done:

    ; ── read arg1 → rsi ──
    mov eax, r13d
    shr eax, 2
    and eax, 3
    jz .a1_pipe
    cmp eax, 1
    je .a1_u8
    cmp eax, 2
    je .a1_u32
    mov rsi, [r12]             ; u64
    add r12, 8
    jmp .a1_done
.a1_u8:
    movzx esi, byte [r12]
    inc r12
    jmp .a1_done
.a1_u32:
    mov esi, [r12]
    add r12, 4
    jmp .a1_done
.a1_pipe:
    mov rsi, r15
.a1_done:

    ; ── read arg2 → rdx ──
    mov eax, r13d
    shr eax, 4
    and eax, 3
    jz .a2_pipe
    cmp eax, 1
    je .a2_u8
    cmp eax, 2
    je .a2_u32
    mov rdx, [r12]             ; u64
    add r12, 8
    jmp .a2_done
.a2_u8:
    movzx edx, byte [r12]
    inc r12
    jmp .a2_done
.a2_u32:
    mov edx, [r12]
    add r12, 4
    jmp .a2_done
.a2_pipe:
    mov rdx, r15
.a2_done:

    mov rcx, r15               ; arg3 = pipeline always

    call rbx                   ; call JIT'd function
    mov r15, rax               ; result → pipeline
    mov [w_acc], rax
    jmp .ww_next

    ; ── walker control ──
.ww_control:
    mov ebx, [r12+1]          ; u32 byte count

    cmp al, 0xFE
    je .ww_skip_z
    cmp al, 0xFD
    je .ww_skip_nz

    ; 0xFC: loop_back — rewind if pipeline != 0
    test r15, r15
    jz .ww_ctrl_advance
    sub r12, rbx               ; rewind by N bytes
    jmp .ww_next

.ww_skip_z:
    test r15, r15
    jnz .ww_ctrl_advance
    add r12, rbx               ; skip forward N bytes
    jmp .ww_ctrl_advance

.ww_skip_nz:
    test r15, r15
    jz .ww_ctrl_advance
    add r12, rbx               ; skip forward N bytes
    ; fall through

.ww_ctrl_advance:
    add r12, 5                 ; past control byte + u32
    jmp .ww_next

.ww_done:
    pop r14
    pop r13
    pop r12
    pop rbx
    ret


; ═══════════════════════════════════════════════════════════════════
; get_atom(edi=t, esi=d, edx=m, ecx=q) → rax = code pointer
;
; Hash lookup. Cache hit = return immediately.
; Cache miss = JIT via emit_function, cache, return.
; ═══════════════════════════════════════════════════════════════════

global get_atom
get_atom:
    push rbx
    push r12
    push r13
    push r14
    push r15

    movsx r12d, di
    movsx r13d, si
    movsx r14d, dx
    movsx r15d, cx

    ; pack key: 4 x i16 → i64
    movzx rax, r12w
    shl rax, 48
    movzx rbx, r13w
    shl rbx, 32
    or rax, rbx
    movzx rbx, r14w
    shl rbx, 16
    or rax, rbx
    movzx rbx, r15w
    or rax, rbx
    mov r8, rax

    ; hash: 7t + 13d + 19m + 23q
    imul eax, r12d, 7
    imul ebx, r13d, 13
    add eax, ebx
    imul ebx, r14d, 19
    add eax, ebx
    imul ebx, r15d, 23
    add eax, ebx
    and eax, HTMASK
    mov ebx, eax

    ; cache lookup
    cmp [ht_key + rbx*8], r8
    jne .miss
    mov rax, [ht_ptr + rbx*8]
    test rax, rax
    jnz .out

.miss:
    call emit_function
    mov [ht_key + rbx*8], r8
    mov [ht_ptr + rbx*8], rax

.out:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret


; ═══════════════════════════════════════════════════════════════════
; emit_function — coords in r12d-r15d → JIT x86-64 machine code
; returns code pointer in rax
; ═══════════════════════════════════════════════════════════════════

emit_function:
    push rbp
    mov rdi, [code_cursor]
    mov rbp, rdi                           ; save start address

    ; classify: any odd coord → bond
    mov eax, r12d
    or eax, r13d
    or eax, r14d
    or eax, r15d
    test eax, 1
    jnz .bond

    ; ─── ATOM: halve coords → exponents, emit mul/div chain ────

    mov eax, r12d
    sar eax, 1
    mov r8d, eax                           ; T exponent
    mov eax, r13d
    sar eax, 1
    mov r9d, eax                           ; D exponent
    mov eax, r14d
    sar eax, 1
    mov r10d, eax                          ; M exponent
    mov eax, r15d
    sar eax, 1
    mov r11d, eax                          ; Q exponent

    ; wave? (all exponents zero)
    mov eax, r8d
    or eax, r9d
    or eax, r10d
    or eax, r11d
    jnz .not_wave

    ; wave: mov rax, rdi; ret
    mov byte [rdi],   0x48
    mov byte [rdi+1], 0x89
    mov byte [rdi+2], 0xF8
    mov byte [rdi+3], 0xC3
    add rdi, 4
    jmp .fin

.not_wave:
    ; ── save inputs to r8-r11 (mul clobbers rdx, div reads rdx:rax) ──

    test r8d, r8d                          ; T: rdi → r8
    jz .ns_t
    mov word [rdi], 0x8949
    mov byte [rdi+2], 0xF8                 ; 49 89 F8 = mov r8, rdi
    add rdi, 3
.ns_t:
    test r9d, r9d                          ; D: rsi → r9
    jz .ns_d
    mov word [rdi], 0x8949
    mov byte [rdi+2], 0xF1                 ; 49 89 F1 = mov r9, rsi
    add rdi, 3
.ns_d:
    test r10d, r10d                        ; M: rdx → r10
    jz .ns_m
    mov word [rdi], 0x8949
    mov byte [rdi+2], 0xD2                 ; 49 89 D2 = mov r10, rdx
    add rdi, 3
.ns_m:
    test r11d, r11d                        ; Q: rcx → r11
    jz .ns_q
    mov word [rdi], 0x8949
    mov byte [rdi+2], 0xCB                 ; 49 89 CB = mov r11, rcx
    add rdi, 3
.ns_q:

    ; ── seed: mov eax, 1 ──
    mov byte [rdi], 0xB8
    mov dword [rdi+1], 1
    add rdi, 5

    ; ── zero guards for negative exponents ──
    ; test rN,rN; jnz +3; xor eax,eax; ret
    test r8d, r8d
    jns .ng_t
    mov dword [rdi],   0x75C0854D         ; test r8, r8; jnz
    mov dword [rdi+4], 0xC3C03103         ; xor eax,eax; ret
    add rdi, 8
.ng_t:
    test r9d, r9d
    jns .ng_d
    mov dword [rdi],   0x75C9854D
    mov dword [rdi+4], 0xC3C03103
    add rdi, 8
.ng_d:
    test r10d, r10d
    jns .ng_m
    mov dword [rdi],   0x75D2854D
    mov dword [rdi+4], 0xC3C03103
    add rdi, 8
.ng_m:
    test r11d, r11d
    jns .ng_q
    mov dword [rdi],   0x75DB854D
    mov dword [rdi+4], 0xC3C03103
    add rdi, 8
.ng_q:

    ; ── multiplications (positive exponents first for precision) ──

    mov eax, r8d                           ; T × r8
    test eax, eax
    jle .nm_t
.lm_t:
    mov word [rdi], 0xF749
    mov byte [rdi+2], 0xE0                 ; 49 F7 E0 = mul r8
    add rdi, 3
    dec eax
    jnz .lm_t
.nm_t:
    mov eax, r9d                           ; D × r9
    test eax, eax
    jle .nm_d
.lm_d:
    mov word [rdi], 0xF749
    mov byte [rdi+2], 0xE1                 ; 49 F7 E1 = mul r9
    add rdi, 3
    dec eax
    jnz .lm_d
.nm_d:
    mov eax, r10d                          ; M × r10
    test eax, eax
    jle .nm_m
.lm_m:
    mov word [rdi], 0xF749
    mov byte [rdi+2], 0xE2                 ; 49 F7 E2 = mul r10
    add rdi, 3
    dec eax
    jnz .lm_m
.nm_m:
    mov eax, r11d                          ; Q × r11
    test eax, eax
    jle .nm_q
.lm_q:
    mov word [rdi], 0xF749
    mov byte [rdi+2], 0xE3                 ; 49 F7 E3 = mul r11
    add rdi, 3
    dec eax
    jnz .lm_q
.nm_q:

    ; ── divisions (negative exponents) ──

    mov eax, r8d                           ; T ÷ r8
    test eax, eax
    jns .nd_t
    neg eax
.ld_t:
    mov word [rdi], 0xD231                 ; xor edx, edx
    mov word [rdi+2], 0xF749
    mov byte [rdi+4], 0xF0                 ; div r8
    add rdi, 5
    dec eax
    jnz .ld_t
.nd_t:
    mov eax, r9d                           ; D ÷ r9
    test eax, eax
    jns .nd_d
    neg eax
.ld_d:
    mov word [rdi], 0xD231
    mov word [rdi+2], 0xF749
    mov byte [rdi+4], 0xF1                 ; div r9
    add rdi, 5
    dec eax
    jnz .ld_d
.nd_d:
    mov eax, r10d                          ; M ÷ r10
    test eax, eax
    jns .nd_m
    neg eax
.ld_m:
    mov word [rdi], 0xD231
    mov word [rdi+2], 0xF749
    mov byte [rdi+4], 0xF2                 ; div r10
    add rdi, 5
    dec eax
    jnz .ld_m
.nd_m:
    mov eax, r11d                          ; Q ÷ r11
    test eax, eax
    jns .nd_q
    neg eax
.ld_q:
    mov word [rdi], 0xD231
    mov word [rdi+2], 0xF749
    mov byte [rdi+4], 0xF3                 ; div r11
    add rdi, 5
    dec eax
    jnz .ld_q
.nd_q:

    ; ── ret ──
    mov byte [rdi], 0xC3
    inc rdi
    jmp .fin


; ─── BOND: classify force pattern, JIT control flow ────────────

.bond:
    ; build bitmask (eax) and sign bits (ecx)
    xor eax, eax
    xor ecx, ecx

    test r12d, 1                           ; P (transport)
    jz .no_p
    or eax, 1
    test r12d, r12d
    js .no_ps
    or ecx, 1
.no_ps:
.no_p:
    test r13d, 1                           ; C (synthesis)
    jz .no_c
    or eax, 2
    test r13d, r13d
    js .no_cs
    or ecx, 2
.no_cs:
.no_c:
    test r14d, 1                           ; R (cycling)
    jz .no_r
    or eax, 4
    test r14d, r14d
    js .no_rs
    or ecx, 4
.no_rs:
.no_r:
    test r15d, 1                           ; W (regulation)
    jz .no_w
    or eax, 8
    test r15d, r15d
    js .no_ws
    or ecx, 8
.no_ws:
.no_w:

    ; dispatch via jump table
    and eax, 0xF
    lea r8, [bond_table]
    mov eax, [r8 + rax*4]
    jmp rax


; ═══════════════════════════════════════════════════════════════════
; BOND HANDLERS — indexed by force bitmask (0-15)
;
; ecx = sign bits, rdi = code write cursor
; Convention: T=rdi, D=rsi, M=rdx, Q=rcx at runtime
; ═══════════════════════════════════════════════════════════════════

.b_0:   ; 0: impossible (no odd coord = atom, shouldn't get here)
    mov byte [rdi], 0xC3
    inc rdi
    jmp .fin

.b_p:   ; 1: P — read (+P) / write (-P), width from |T|
    ;   |T|=1: qword   |T|=3: byte   |T|=5: word   |T|=7: dword
    mov eax, r12d              ; T coordinate
    test eax, eax
    jns .p_tpos
    neg eax
.p_tpos:
    test ecx, 1
    jz .b_p_write
    ; ── read ──
    cmp eax, 7
    jge .p_dword_read
    cmp eax, 5
    jge .p_word_read
    cmp eax, 3
    jge .p_byte_read
    ; +P qword read: mov rax, [rdi]; ret
    mov dword [rdi], 0xC3078B48
    add rdi, 4
    jmp .fin
.p_dword_read:
    ; +P dword read: mov eax, [rdi]; ret
    mov byte [rdi],   0x8B
    mov byte [rdi+1], 0x07
    mov byte [rdi+2], 0xC3
    add rdi, 3
    jmp .fin
.p_word_read:
    ; +P word read: movzx eax, word [rdi]; ret
    mov byte [rdi],   0x0F
    mov byte [rdi+1], 0xB7
    mov byte [rdi+2], 0x07
    mov byte [rdi+3], 0xC3
    add rdi, 4
    jmp .fin
.p_byte_read:
    ; +P byte read: movzx eax, byte [rdi]; ret
    mov byte [rdi],   0x0F
    mov byte [rdi+1], 0xB6
    mov byte [rdi+2], 0x07     ; movzx eax, byte [rdi]
    mov byte [rdi+3], 0xC3
    add rdi, 4
    jmp .fin
.b_p_write:
    cmp eax, 7
    jge .p_dword_write
    cmp eax, 5
    jge .p_word_write
    cmp eax, 3
    jge .p_byte_write
    ; -P qword write: mov [rdi], rsi; mov rax, rsi; ret
    mov dword [rdi],  0x48378948
    mov word [rdi+4], 0xF089
    mov byte [rdi+6], 0xC3
    add rdi, 7
    jmp .fin
.p_dword_write:
    ; -P dword write: mov [rdi], esi; mov eax, esi; ret
    mov byte [rdi],   0x89
    mov byte [rdi+1], 0x37     ; mov [rdi], esi
    mov byte [rdi+2], 0x89
    mov byte [rdi+3], 0xF0     ; mov eax, esi
    mov byte [rdi+4], 0xC3
    add rdi, 5
    jmp .fin
.p_word_write:
    ; -P word write: mov [rdi], si; movzx eax, si; ret
    mov byte [rdi],   0x66     ; operand size prefix
    mov byte [rdi+1], 0x89
    mov byte [rdi+2], 0x37     ; mov [rdi], si
    mov byte [rdi+3], 0x0F
    mov byte [rdi+4], 0xB7
    mov byte [rdi+5], 0xC6     ; movzx eax, si
    mov byte [rdi+6], 0xC3
    add rdi, 7
    jmp .fin
.p_byte_write:
    ; -P byte write: mov [rdi], sil; movzx rax, sil; ret
    mov byte [rdi],   0x40     ; REX
    mov byte [rdi+1], 0x88
    mov byte [rdi+2], 0x37     ; mov [rdi], sil
    mov byte [rdi+3], 0x40     ; REX
    mov byte [rdi+4], 0x0F
    mov byte [rdi+5], 0xB6
    mov byte [rdi+6], 0xC6     ; movzx eax, sil
    mov byte [rdi+7], 0xC3
    add rdi, 8
    jmp .fin

.b_c:   ; 2: C — apply (+C) / curry (-C)
    test ecx, 2
    jz .b_c_curry
    ; +C apply: jmp rdi
    mov word [rdi], 0xE7FF
    add rdi, 2
    jmp .fin
.b_c_curry:
    ; -C curry: mov rax, rdi; ret
    mov dword [rdi], 0xC3F88948
    add rdi, 4
    jmp .fin

.b_pc:  ; 3: P+C — fold (load [rsi], call rdx)
    mov byte [rdi],   0x48
    mov byte [rdi+1], 0x8B
    mov byte [rdi+2], 0x3E                 ; mov rdi, [rsi]
    mov word [rdi+3], 0xE2FF               ; jmp rdx
    add rdi, 5
    jmp .fin

.b_r:   ; 4: R — drain (+R) / increment (-R)
    test ecx, 4
    jz .b_r_neg
    ; +R drain: mov rax,rdi; dec rax; jnz $-3; ret
    mov dword [rdi],  0xF88948
    mov byte [rdi+3], 0x48
    mov byte [rdi+4], 0xFF
    mov byte [rdi+5], 0xC8
    mov byte [rdi+6], 0x75
    mov byte [rdi+7], 0xFB
    mov byte [rdi+8], 0xC3
    add rdi, 9
    jmp .fin
.b_r_neg:
    ; -R increment: mov rax, rdi; inc rax; ret
    mov dword [rdi],  0xF88948
    mov byte [rdi+3], 0x48
    mov byte [rdi+4], 0xFF
    mov byte [rdi+5], 0xC0
    mov byte [rdi+6], 0xC3
    add rdi, 7
    jmp .fin

.b_pr:  ; 5: P+R — slurp (+P) / fill (-P), width from |R|
    mov eax, r14d
    test eax, eax
    jns .pr_rpos
    neg eax
.pr_rpos:
    test ecx, 1
    jz .b_fill
    ; slurp: rdi=dest, rsi=src, edx=count
    mov word [rdi], 0xD189                 ; mov ecx, edx
    mov byte [rdi+2], 0xFC                 ; cld
    cmp eax, 5
    jge .slurp_q
    cmp eax, 3
    jge .slurp_d
    ; byte width
    mov word [rdi+3], 0xA4F3               ; rep movsb
    mov byte [rdi+5], 0xC3
    add rdi, 6
    jmp .fin
.slurp_d:
    mov word [rdi+3], 0xA5F3               ; rep movsd
    mov byte [rdi+5], 0xC3
    add rdi, 6
    jmp .fin
.slurp_q:
    mov byte [rdi+3], 0xF3                 ; rep movsq
    mov byte [rdi+4], 0x48
    mov byte [rdi+5], 0xA5
    mov byte [rdi+6], 0xC3
    add rdi, 7
    jmp .fin
.b_fill:
    cmp eax, 5
    jge .fill_q
    cmp eax, 3
    jge .fill_d
    ; byte fill
    mov byte [rdi],   0x40
    mov byte [rdi+1], 0x88
    mov byte [rdi+2], 0xF0                 ; mov al, sil
    mov byte [rdi+3], 0x89
    mov byte [rdi+4], 0xD1                 ; mov ecx, edx
    mov byte [rdi+5], 0xFC                 ; cld
    mov word [rdi+6], 0xAAF3               ; rep stosb
    mov byte [rdi+8], 0xC3
    add rdi, 9
    jmp .fin
.fill_d:
    mov word [rdi],   0xF089               ; mov eax, esi
    mov word [rdi+2], 0xD189               ; mov ecx, edx
    mov byte [rdi+4], 0xFC                 ; cld
    mov word [rdi+5], 0xABF3               ; rep stosd
    mov byte [rdi+7], 0xC3
    add rdi, 8
    jmp .fin
.fill_q:
    mov word [rdi],   0x8948               ; mov rax, rsi
    mov byte [rdi+2], 0xF0
    mov word [rdi+3], 0xD189               ; mov ecx, edx
    mov byte [rdi+5], 0xFC                 ; cld
    mov byte [rdi+6], 0xF3                 ; rep stosq
    mov byte [rdi+7], 0x48
    mov byte [rdi+8], 0xAB
    mov byte [rdi+9], 0xC3
    add rdi, 10
    jmp .fin

.b_cr:  ; 6: C+R — fix (call fn N times)
    mov byte [rdi],    0x53                ; push rbx
    mov byte [rdi+1],  0x48                ; mov rbx, rdi
    mov byte [rdi+2],  0x89
    mov byte [rdi+3],  0xFB
    mov byte [rdi+4],  0x48                ; mov rax, rsi
    mov byte [rdi+5],  0x89
    mov byte [rdi+6],  0xF0
    mov byte [rdi+7],  0x48                ; mov rdi, rax
    mov byte [rdi+8],  0x89
    mov byte [rdi+9],  0xC7
    mov word [rdi+10], 0xD3FF              ; call rbx
    mov word [rdi+12], 0xCAFF              ; dec rdx
    mov word [rdi+14], 0xF775              ; jnz -9
    mov byte [rdi+16], 0x5B               ; pop rbx
    mov byte [rdi+17], 0xC3               ; ret
    add rdi, 18
    jmp .fin

.b_pcr: ; 7: P+C+R — foldl
    mov byte [rdi],    0x53               ; push rbx
    mov word [rdi+1],  0x5441             ; push r12
    mov word [rdi+3],  0x5541             ; push r13
    mov byte [rdi+5],  0x49               ; mov r12, rdi
    mov byte [rdi+6],  0x89
    mov byte [rdi+7],  0xFC
    mov byte [rdi+8],  0x49               ; mov r13, rsi
    mov byte [rdi+9],  0x89
    mov byte [rdi+10], 0xF5
    mov word [rdi+11], 0xD389             ; mov ebx, edx
    mov byte [rdi+13], 0x48               ; mov rax, rcx
    mov byte [rdi+14], 0x89
    mov byte [rdi+15], 0xC8
    mov word [rdi+16], 0xDB85             ; test ebx, ebx
    mov word [rdi+18], 0x1274             ; jz +18
    mov byte [rdi+20], 0x48               ; mov rdi, rax
    mov byte [rdi+21], 0x89
    mov byte [rdi+22], 0xC7
    mov byte [rdi+23], 0x49               ; mov rsi, [r13]
    mov byte [rdi+24], 0x8B
    mov byte [rdi+25], 0x75
    mov byte [rdi+26], 0x00
    mov byte [rdi+27], 0x41               ; call r12
    mov word [rdi+28], 0xD4FF
    mov byte [rdi+30], 0x49               ; add r13, 8
    mov byte [rdi+31], 0x83
    mov byte [rdi+32], 0xC5
    mov byte [rdi+33], 0x08
    mov word [rdi+34], 0xCBFF             ; dec ebx
    mov word [rdi+36], 0xEA75             ; jnz -22
    mov word [rdi+38], 0x5D41             ; pop r13
    mov word [rdi+40], 0x5C41             ; pop r12
    mov byte [rdi+42], 0x5B               ; pop rbx
    mov byte [rdi+43], 0xC3               ; ret
    add rdi, 44
    jmp .fin

.b_w:   ; 8: W — gates by |Q| magnitude
    ;   |Q|=1: test/guard (shell 1)
    ;   |Q|=3: AND/OR    (shell 2 — boolean gates)
    ;   |Q|=5: XOR/NOT   (shell 3 — boolean gates)
    ;   |Q|=7: SHL/SHR   (shell 4 — shift gates)
    ;   |Q|=9: BSWAP32/64 (shell 5 — endian conversion)
    mov eax, r15d              ; Q coordinate
    test eax, eax
    jns .w_qpos
    neg eax
.w_qpos:
    cmp eax, 9
    jge .w_bswap
    cmp eax, 7
    jge .w_shift
    cmp eax, 5
    jge .w_xor_not
    cmp eax, 3
    jge .w_and_or
    ; ── |Q|=1: test/guard (existing) ──
    test ecx, 8
    jz .b_w_guard
    ; +W test: cmp edi,esi; sete al; movzx eax,al; ret
    mov word [rdi],   0xF739
    mov dword [rdi+2], 0xC0940F
    mov byte [rdi+5], 0x0F
    mov byte [rdi+6], 0xB6
    mov byte [rdi+7], 0xC0
    mov byte [rdi+8], 0xC3
    add rdi, 9
    jmp .fin
.b_w_guard:
    ; -W guard: xor eax,eax; test esi,esi; cmovnz eax,edi; ret
    mov word [rdi],   0xC031
    mov word [rdi+2], 0xF685
    mov byte [rdi+4], 0x0F
    mov byte [rdi+5], 0x45
    mov byte [rdi+6], 0xC7
    mov byte [rdi+7], 0xC3
    add rdi, 8
    jmp .fin
    ; ── |Q|=3: AND / OR ──
.w_and_or:
    ; all bitwise: mov rax, rdi (48 89 F8) + op + ret (C3)
    mov word [rdi], 0x8948
    mov byte [rdi+2], 0xF8
    test ecx, 8
    jz .w_or
    ; +W AND: and rax, rsi (48 21 F0)
    mov word [rdi+3], 0x2148
    mov byte [rdi+5], 0xF0
    mov byte [rdi+6], 0xC3
    add rdi, 7
    jmp .fin
.w_or:
    ; -W OR: or rax, rsi (48 09 F0)
    mov word [rdi+3], 0x0948
    mov byte [rdi+5], 0xF0
    mov byte [rdi+6], 0xC3
    add rdi, 7
    jmp .fin
    ; ── |Q|=5: XOR / NOT ──
.w_xor_not:
    mov word [rdi], 0x8948
    mov byte [rdi+2], 0xF8
    test ecx, 8
    jz .w_not
    ; +W XOR: xor rax, rsi (48 31 F0)
    mov word [rdi+3], 0x3148
    mov byte [rdi+5], 0xF0
    mov byte [rdi+6], 0xC3
    add rdi, 7
    jmp .fin
.w_not:
    ; -W NOT: not rax (48 F7 D0)
    mov word [rdi+3], 0xF748
    mov byte [rdi+5], 0xD0
    mov byte [rdi+6], 0xC3
    add rdi, 7
    jmp .fin
    ; ── |Q|=7: SHL / SHR ──
.w_shift:
    ; mov rax, rdi; mov ecx, esi; sh{l|r} rax, cl; ret
    mov word [rdi], 0x8948
    mov byte [rdi+2], 0xF8
    mov word [rdi+3], 0xF189     ; mov ecx, esi
    mov word [rdi+5], 0xD348     ; 48 D3 prefix
    test ecx, 8
    jz .w_shr
    ; +W SHL: shl rax, cl (48 D3 E0)
    mov byte [rdi+7], 0xE0
    mov byte [rdi+8], 0xC3
    add rdi, 9
    jmp .fin
.w_shr:
    ; -W SHR: shr rax, cl (48 D3 E8)
    mov byte [rdi+7], 0xE8
    mov byte [rdi+8], 0xC3
    add rdi, 9
    jmp .fin
    ; ── |Q|=9: BSWAP32 / BSWAP64 ──
.w_bswap:
    test ecx, 8
    jz .w_bswap64
    ; +W BSWAP32: mov eax, edi; bswap eax; ret
    mov word [rdi],   0xF889       ; mov eax, edi
    mov word [rdi+2], 0xC80F       ; bswap eax
    mov byte [rdi+4], 0xC3
    add rdi, 5
    jmp .fin
.w_bswap64:
    ; -W BSWAP64: mov rax, rdi; bswap rax; ret
    mov word [rdi],   0x8948       ; mov rax, rdi (48 89 F8)
    mov byte [rdi+2], 0xF8
    mov word [rdi+3], 0xC80F48     ; bswap rax (48 0F C8)
    ; wait, 48 0F C8 is 3 bytes
    mov byte [rdi+3], 0x48
    mov byte [rdi+4], 0x0F
    mov byte [rdi+5], 0xC8
    mov byte [rdi+6], 0xC3
    add rdi, 7
    jmp .fin

.b_pw:  ; 9: P+W — filter / port read / port write / add
    test ecx, 1
    jz .pw_p_neg
    test ecx, 8
    jz .pw_port_read
    ; +P +W filter: cmp edi,esi; mov eax,edi; cmovg eax,esi; ret
    mov word [rdi],   0xF739
    mov word [rdi+2], 0xF889
    mov byte [rdi+4], 0x0F
    mov byte [rdi+5], 0x4F
    mov byte [rdi+6], 0xC6
    mov byte [rdi+7], 0xC3
    add rdi, 8
    jmp .fin
.pw_port_read:
    ; +P -W port read: width from |T|: 1=byte, 3=word, 5=dword
    ; mov edx, edi; in {al|ax|eax}, dx; movzx eax, {al|ax}; ret
    mov word [rdi],   0xFA89              ; mov edx, edi (port)
    mov eax, r12d                         ; T coordinate
    test eax, eax
    jns .pr_pos
    neg eax
.pr_pos:
    cmp eax, 5
    jge .pr_dword
    cmp eax, 3
    jge .pr_word
    ; byte: in al, dx; movzx eax, al
    mov byte [rdi+2], 0xEC               ; in al, dx
    mov byte [rdi+3], 0x0F               ; movzx eax, al
    mov byte [rdi+4], 0xB6
    mov byte [rdi+5], 0xC0
    mov byte [rdi+6], 0xC3
    add rdi, 7
    jmp .fin
.pr_word:
    ; word: 66 ED = in ax, dx; 0F B7 C0 = movzx eax, ax
    mov byte [rdi+2], 0x66
    mov byte [rdi+3], 0xED               ; in ax, dx
    mov byte [rdi+4], 0x0F               ; movzx eax, ax
    mov byte [rdi+5], 0xB7
    mov byte [rdi+6], 0xC0
    mov byte [rdi+7], 0xC3
    add rdi, 8
    jmp .fin
.pr_dword:
    ; dword: ED = in eax, dx; ret
    mov byte [rdi+2], 0xED               ; in eax, dx
    mov byte [rdi+3], 0xC3
    add rdi, 4
    jmp .fin

.pw_p_neg:
    test ecx, 8
    jz .pw_add
    ; -P +W port write: width from |T|: 1=byte, 3=word, 5=dword
    ; mov edx, edi; mov {al|ax|eax}, esi; out dx, {al|ax|eax}; ret
    mov word [rdi],   0xFA89              ; mov edx, edi (port)
    mov eax, r12d                         ; T coordinate
    test eax, eax
    jns .pwn_pos
    neg eax
.pwn_pos:
    cmp eax, 5
    jge .pw_dword
    cmp eax, 3
    jge .pw_word
    ; byte: movzx eax, sil; out dx, al
    mov byte [rdi+2], 0x40               ; REX
    mov byte [rdi+3], 0x0F               ; movzx eax, sil
    mov byte [rdi+4], 0xB6
    mov byte [rdi+5], 0xC6
    mov byte [rdi+6], 0xEE               ; out dx, al
    mov byte [rdi+7], 0xC3
    add rdi, 8
    jmp .fin
.pw_word:
    ; word: mov eax, esi; 66 EF = out dx, ax
    mov word [rdi+2], 0xF089             ; mov eax, esi
    mov byte [rdi+4], 0x66
    mov byte [rdi+5], 0xEF               ; out dx, ax
    mov byte [rdi+6], 0xC3
    add rdi, 7
    jmp .fin
.pw_dword:
    ; dword: mov eax, esi; EF = out dx, eax
    mov word [rdi+2], 0xF089             ; mov eax, esi
    mov byte [rdi+4], 0xEF               ; out dx, eax
    mov byte [rdi+5], 0xC3
    add rdi, 6
    jmp .fin
.pw_add:
    ; -P -W add: lea rax, [rdi+rsi]; ret
    mov byte [rdi],   0x48
    mov byte [rdi+1], 0x8D
    mov byte [rdi+2], 0x04
    mov byte [rdi+3], 0x37
    mov byte [rdi+4], 0xC3
    add rdi, 5
    jmp .fin

.b_cw:  ; 10: C+W — maybe (conditional call)
    mov word [rdi],   0xF685               ; test esi, esi
    mov word [rdi+2], 0x0274               ; jz +2
    mov word [rdi+4], 0xE7FF               ; jmp rdi
    mov word [rdi+6], 0xC031               ; xor eax, eax
    mov byte [rdi+8], 0xC3                 ; ret
    add rdi, 9
    jmp .fin

.b_pcw: ; 11: P+C+W — map (apply fn to each element)
    mov byte [rdi],    0x53               ; push rbx
    mov word [rdi+1],  0x5441             ; push r12
    mov word [rdi+3],  0x5541             ; push r13
    mov byte [rdi+5],  0x48               ; mov rbx, rdi
    mov byte [rdi+6],  0x89
    mov byte [rdi+7],  0xFB
    mov byte [rdi+8],  0x49               ; mov r12, rsi
    mov byte [rdi+9],  0x89
    mov byte [rdi+10], 0xF4
    mov byte [rdi+11], 0x41               ; mov r13d, edx
    mov byte [rdi+12], 0x89
    mov byte [rdi+13], 0xD5
    mov byte [rdi+14], 0x45               ; test r13d, r13d
    mov byte [rdi+15], 0x85
    mov byte [rdi+16], 0xED
    mov byte [rdi+17], 0x74               ; jz +12
    mov byte [rdi+18], 0x0C
    mov byte [rdi+19], 0x49               ; mov rdi, [r12]
    mov byte [rdi+20], 0x8B
    mov byte [rdi+21], 0x3C
    mov byte [rdi+22], 0x24
    mov word [rdi+23], 0xD3FF             ; call rbx
    mov byte [rdi+25], 0x49               ; add r12, 8
    mov byte [rdi+26], 0x83
    mov byte [rdi+27], 0xC4
    mov byte [rdi+28], 0x08
    mov byte [rdi+29], 0x41               ; dec r13d
    mov word [rdi+30], 0xCDFF
    mov byte [rdi+32], 0xEB               ; jmp -14
    mov byte [rdi+33], 0xF0
    mov word [rdi+34], 0x5D41             ; pop r13
    mov word [rdi+36], 0x5C41             ; pop r12
    mov byte [rdi+38], 0x5B               ; pop rbx
    mov byte [rdi+39], 0xC3               ; ret
    add rdi, 40
    jmp .fin

.b_rw:  ; 12: R+W — take_while (scan until null byte)
    mov word [rdi],    0xC031              ; xor eax, eax
    mov dword [rdi+2], 0x00073C80          ; cmp byte [rdi+rax], 0
    mov byte [rdi+6],  0x74               ; jz +4
    mov byte [rdi+7],  0x04
    mov word [rdi+8],  0xC0FF              ; inc rax
    mov word [rdi+10], 0xF6EB              ; jmp -10
    mov byte [rdi+12], 0xC3               ; ret
    add rdi, 13
    jmp .fin

.b_prw: ; 13: P+R+W — scan (sum bytes until null)
    mov word [rdi],    0xC031              ; xor eax, eax
    mov byte [rdi+2],  0x0F               ; movzx ecx, byte [rdi]
    mov byte [rdi+3],  0xB6
    mov byte [rdi+4],  0x0F
    mov word [rdi+5],  0xC985              ; test ecx, ecx
    mov byte [rdi+7],  0x74               ; jz +5
    mov byte [rdi+8],  0x05
    mov word [rdi+9],  0xC801              ; add eax, ecx
    mov byte [rdi+11], 0x48               ; inc rdi
    mov word [rdi+12], 0xC7FF
    mov byte [rdi+14], 0xEB               ; jmp -14
    mov byte [rdi+15], 0xF0
    mov byte [rdi+16], 0xC3               ; ret
    add rdi, 17
    jmp .fin

.b_crw: ; 14: C+R+W — bind (chain fn until returns 0)
    mov byte [rdi],    0x53               ; push rbx
    mov byte [rdi+1],  0x48               ; mov rbx, rdi
    mov byte [rdi+2],  0x89
    mov byte [rdi+3],  0xFB
    mov byte [rdi+4],  0x48               ; mov rdi, rsi
    mov byte [rdi+5],  0x89
    mov byte [rdi+6],  0xF7
    mov word [rdi+7],  0xD3FF             ; call rbx
    mov byte [rdi+9],  0x48               ; test rax, rax
    mov byte [rdi+10], 0x85
    mov byte [rdi+11], 0xC0
    mov byte [rdi+12], 0x74               ; jz +5
    mov byte [rdi+13], 0x05
    mov byte [rdi+14], 0x48               ; mov rdi, rax
    mov byte [rdi+15], 0x89
    mov byte [rdi+16], 0xC7
    mov byte [rdi+17], 0xEB               ; jmp -12
    mov byte [rdi+18], 0xF4
    mov byte [rdi+19], 0x5B               ; pop rbx
    mov byte [rdi+20], 0xC3               ; ret
    add rdi, 21
    jmp .fin

.b_pcrw: ; 15: P+C+R+W — hylo (full pipeline)
    mov byte [rdi],    0x53               ; push rbx
    mov word [rdi+1],  0x5441             ; push r12
    mov word [rdi+3],  0x5541             ; push r13
    mov word [rdi+5],  0x5641             ; push r14
    mov byte [rdi+7],  0x49               ; mov r12, rdi
    mov byte [rdi+8],  0x89
    mov byte [rdi+9],  0xFC
    mov byte [rdi+10], 0x49               ; mov r13, rsi
    mov byte [rdi+11], 0x89
    mov byte [rdi+12], 0xF5
    mov byte [rdi+13], 0x41               ; mov r14d, edx
    mov byte [rdi+14], 0x89
    mov byte [rdi+15], 0xD6
    mov byte [rdi+16], 0x48               ; mov rbx, rcx
    mov byte [rdi+17], 0x89
    mov byte [rdi+18], 0xCB
    mov byte [rdi+19], 0x45               ; test r14d, r14d
    mov byte [rdi+20], 0x85
    mov byte [rdi+21], 0xF6
    mov word [rdi+22], 0x1B74             ; jz +27
    mov byte [rdi+24], 0x48               ; mov rdi, rbx
    mov byte [rdi+25], 0x89
    mov byte [rdi+26], 0xDF
    mov byte [rdi+27], 0x49               ; mov rsi, [r13]
    mov byte [rdi+28], 0x8B
    mov byte [rdi+29], 0x75
    mov byte [rdi+30], 0x00
    mov byte [rdi+31], 0x41               ; call r12
    mov word [rdi+32], 0xD4FF
    mov byte [rdi+34], 0x48               ; mov rbx, rax
    mov byte [rdi+35], 0x89
    mov byte [rdi+36], 0xC3
    mov byte [rdi+37], 0x48               ; test rax, rax
    mov byte [rdi+38], 0x85
    mov byte [rdi+39], 0xC0
    mov word [rdi+40], 0x0974             ; jz +9
    mov byte [rdi+42], 0x49               ; add r13, 8
    mov byte [rdi+43], 0x83
    mov byte [rdi+44], 0xC5
    mov byte [rdi+45], 0x08
    mov byte [rdi+46], 0x41               ; dec r14d
    mov word [rdi+47], 0xCEFF
    mov word [rdi+49], 0xE075             ; jnz -32
    mov byte [rdi+51], 0x48               ; mov rax, rbx
    mov byte [rdi+52], 0x89
    mov byte [rdi+53], 0xD8
    mov word [rdi+54], 0x5E41             ; pop r14
    mov word [rdi+56], 0x5D41             ; pop r13
    mov word [rdi+58], 0x5C41             ; pop r12
    mov byte [rdi+60], 0x5B               ; pop rbx
    mov byte [rdi+61], 0xC3               ; ret
    add rdi, 62
    jmp .fin


; ─── emit finish ───────────────────────────────────────────────

.fin:
    mov [code_cursor], rdi
    mov rax, rbp
    pop rbp
    ret


; ─── bond jump table ──────────────────────────────────────────

section .rodata
align 4
bond_table:
    dd emit_function.b_0                   ; 0  (impossible)
    dd emit_function.b_p                   ; 1  P
    dd emit_function.b_c                   ; 2  C
    dd emit_function.b_pc                  ; 3  P+C
    dd emit_function.b_r                   ; 4  R
    dd emit_function.b_pr                  ; 5  P+R
    dd emit_function.b_cr                  ; 6  C+R
    dd emit_function.b_pcr                 ; 7  P+C+R
    dd emit_function.b_w                   ; 8  W
    dd emit_function.b_pw                  ; 9  P+W
    dd emit_function.b_cw                  ; 10 C+W
    dd emit_function.b_pcw                 ; 11 P+C+W
    dd emit_function.b_rw                  ; 12 R+W
    dd emit_function.b_prw                 ; 13 P+R+W
    dd emit_function.b_crw                 ; 14 C+R+W
    dd emit_function.b_pcrw                ; 15 P+C+R+W


; ─── bond seed table (15 positive-sign patterns) ──────────────

bond_seeds:
    db  1,  0,  0,  0                     ; P:     read
    db  0,  1,  0,  0                     ; C:     apply
    db  1,  1,  0,  0                     ; P+C:   fold
    db  0,  0,  1,  0                     ; R:     drain
    db  1,  0,  1,  0                     ; P+R:   slurp
    db  0,  1,  1,  0                     ; C+R:   fix
    db  1,  1,  1,  0                     ; P+C+R: foldl
    db  0,  0,  0,  1                     ; W:     test
    db  1,  0,  0,  1                     ; P+W:   filter
    db  0,  1,  0,  1                     ; C+W:   maybe
    db  1,  1,  0,  1                     ; P+C+W: map
    db  0,  0,  1,  1                     ; R+W:   take_while
    db  1,  0,  1,  1                     ; P+R+W: scan
    db  0,  1,  1,  1                     ; C+R+W: bind
    db  1,  1,  1,  1                     ; P+C+R+W: hylo


; ─── wave byte decode table (2-bit → signed coordinate) ─────

decode_2bit:
    db  0                                  ; 00 → 0
    db  1                                  ; 01 → +1
    db  0                                  ; 10 → 0 (reserved)
    db -1                                  ; 11 → -1


; ─── externs ──────────────────────────────────────────────────

extern main_walk, main_len
extern kbd_walk, kbd_walk_len

