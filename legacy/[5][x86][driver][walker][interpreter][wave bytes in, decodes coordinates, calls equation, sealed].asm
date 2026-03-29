; ═══════════════════════════════════════════════════════════════════
; [5] Walker — the wave byte interpreter. Sealed.
;
; This is half of the x86 driver (the other half is the equation [4]).
; It reads a walk (a byte array of wave-encoded lattice records) and
; executes each record by calling get_atom to get a JIT'd function.
;
; A walk is a flat byte stream. The walker reads it left to right:
;
;   1. Read one byte (the wave byte). This encodes 4 coordinates:
;      bits [7:6]=T  [5:4]=D  [3:2]=M  [1:0]=Q
;      00=0  01=+1  11=-1  10=extended (next byte is signed i8 value)
;
;   2. Read extension bytes for any dimension that used 10 (extended).
;      Most bonds are shell-1 (no extensions). Higher magnitudes cost 1 byte each.
;
;   3. Read the flags byte. This says how big each arg is:
;      bits [1:0]=arg0  [3:2]=arg1  [5:4]=arg2
;      00=pipeline (use previous result)  01=u8  10=u32  11=u64
;      bit 7: dereference arg0 (treat value as a pointer, read what it points to)
;      bit 6: dereference arg1 (same)
;
;   4. Read inline arg bytes (0, 1, 4, or 8 bytes per arg depending on flags).
;
;   5. Call get_atom(T, D, M, Q) → returns a function pointer to JIT'd x86.
;      Call that function with (arg0, arg1, arg2, pipeline).
;      Result goes into the pipeline register (r15).
;
;   6. Repeat until end of walk.
;
; Walker control bytes (>= 0xFC) are not bonds — they control the walker:
;   0xFE + u32: skip_z   — skip N bytes forward if pipeline == 0
;   0xFD + u32: skip_nz  — skip N bytes forward if pipeline != 0
;   0xFC + u32: loop_back — rewind N bytes backward if pipeline != 0
;
; The pipeline (r15) carries results between records. Every bond reads it
; as arg3 and the previous result feeds the next bond automatically.
;
; On a balanced ternary processor, this entire file dissolves. The hardware
; would read wave bytes natively — no decoding, no flags, no JIT calls.
;
; Input:  rdi = pointer to walk data, esi = walk length in bytes
; Output: rax = last pipeline value
; ═══════════════════════════════════════════════════════════════════

section .text
bits 64

extern get_atom
extern stash_a, stash_b

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
    test r13d, 0x80           ; bit 7: indirect arg0?
    jz .a0_direct
    mov rdi, [rdi]            ; dereference pointer
.a0_direct:

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
    test r13d, 0x40           ; bit 6: indirect arg1?
    jz .a1_direct
    mov rsi, [rsi]            ; dereference pointer
.a1_direct:

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


; ─── wave byte decode table (2-bit → signed coordinate) ─────

section .rodata

decode_2bit:
    db  0                                  ; 00 → 0
    db  1                                  ; 01 → +1
    db  0                                  ; 10 → 0 (reserved)
    db -1                                  ; 11 → -1


; ─── externs ──────────────────────────────────────────────────

extern w_acc
