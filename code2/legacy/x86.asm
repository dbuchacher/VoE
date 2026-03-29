; x86.asm — the substrate. everything here dissolves on a trit processor.
;
; One file, four sections:
;   1. CPU    — 32→64 bit mode switch, page tables, wake all cores
;   2. HANDOFF — init JIT buffer, walk genesis, enter find-work
;   3. NOMOS  — ƒ(τ,χ,μ,φ) = τᵃ·χᵇ·μᶜ·φᵈ — the JIT compiler
;   4. WALKER — ψ — wave byte interpreter
;
; Interface (used by walks):
;   ƒ  — resolve coordinates → function pointer
;   ψ  — interpret a walk (byte array)
;   ρ  — last pipeline result
;
; Everything else is internal x86 plumbing.


; ═══════════════════════════════════════════════════════════════
; SECTION 1: CPU — entry, mode switch, page tables, AP wake
; ═══════════════════════════════════════════════════════════════

; ── multiboot header ──────────────────────────────────────────

section .multiboot
bits 32
    jmp _start
    align 4
    dd 0x1BADB002                          ; multiboot magic
    dd 0x00000003                          ; flags: align + meminfo
    dd -(0x1BADB002 + 0x00000003)          ; checksum

; ── BSS ───────────────────────────────────────────────────────

section .bss
align 16
stack_bottom:   resb 16384                 ; 16KB BSP stack
stack_top:

align 4096
ap_stacks:      resb 4096 * 16            ; 16 AP stacks, 4KB each

; page tables in dedicated section (not zeroed with BSS)
section .pagetables nobits alloc write
align 4096
pml4:           resb 4096
pdpt:           resb 4096
pd0:            resb 4096                  ; low 1GB
pd3:            resb 4096                  ; high 1GB (MMIO)

; ── data ──────────────────────────────────────────────────────

section .data
align 4

gdt:            dq 0                       ; null descriptor
                dq 0x00AF9A000000FFFF      ; 0x08: code64 (L=1)
                dq 0x00CF92000000FFFF      ; 0x10: data64
                dq 0x00CF9A000000FFFF      ; 0x18: code32 (trampoline)
gdt_end:
gdtr:           dw gdt_end - gdt - 1
                dd gdt

ap_count:       dd 0
genesis_done:   dd 0

; ── 32-bit entry ──────────────────────────────────────────────

section .text
bits 32

global _start

_start:
    cli

    ; mask all PIC IRQs — we poll, not interrupt
    mov al, 0xFF
    out 0x21, al
    out 0xA1, al

    ; copy AP trampoline to 0x8000
    mov esi, trampoline_code
    mov edi, 0x8000
    mov ecx, trampoline_end - trampoline_code
    cld
    rep movsb

    ; ── build page tables ──

    ; PML4[0] → PDPT
    mov eax, pdpt
    or eax, 3                              ; present | writable
    mov [pml4], eax

    ; PDPT[0] → PD0 (low 1GB)
    mov eax, pd0
    or eax, 3
    mov [pdpt], eax

    ; PDPT[3] → PD3 (high 1GB, MMIO)
    mov eax, pd3
    or eax, 3
    mov [pdpt + 3*8], eax

    ; PD0: identity map 0–1GB as 2MB huge pages
    xor ecx, ecx
    mov eax, 0x83                          ; present | writable | 2MB
.map_low:
    mov [pd0 + ecx*8], eax
    mov dword [pd0 + ecx*8 + 4], 0
    add eax, 0x200000
    inc ecx
    cmp ecx, 512
    jl .map_low

    ; PD3: map 3–4GB (MMIO) as 2MB pages
    xor ecx, ecx
    mov eax, 0xC0000083                    ; 3GB base | present | writable | 2MB
.map_high:
    mov [pd3 + ecx*8], eax
    mov dword [pd3 + ecx*8 + 4], 0
    add eax, 0x200000
    inc ecx
    cmp ecx, 512
    jl .map_high

    ; APIC MMIO page: mark uncacheable (PCD)
    or dword [pd3 + 503*8], 0x10

    ; ── enable long mode ──

    mov eax, pml4
    mov cr3, eax

    ; PAE + SSE
    mov eax, cr4
    or eax, (1<<5)|(1<<9)|(1<<10)
    mov cr4, eax

    ; EFER.LME (long mode enable)
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1<<8)
    wrmsr

    ; enable paging, clear FPU emulation
    mov eax, cr0
    or eax, (1<<31)
    and eax, ~((1<<2)|(1<<3))
    mov cr0, eax

    lgdt [gdtr]
    jmp 0x08:long_mode

; ── 64-bit entry ──────────────────────────────────────────────

bits 64

global long_mode
long_mode:
%ifdef DEBUG
    mov al, 'L'
    out 0xE9, al
%endif
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, stack_top

    ; zero BSS
    extern __bss_start, __bss_end
    lea rdi, [__bss_start]
    lea rcx, [__bss_end]
    sub rcx, rdi
    shr rcx, 3
    xor eax, eax
    rep stosq

    ; wake APs
    call wake_cores

    ; hand off to the lattice
    jmp Λ


; ══════════════════════════════════════════════════════════════
; AP wake — INIT/SIPI broadcast
; ══════════════════════════════════════════════════════════════

wake_cores:
    ; write trampoline parameters at 0x8090+
    mov ax, [gdtr]
    mov [0x8090], ax
    mov eax, [gdtr + 2]
    mov [0x8092], eax

    mov rax, cr3
    mov [0x8098], eax

    lea rax, [ap_entry]
    mov [0x80A0], eax
    mov word [0x80A4], 0x08                ; code64 selector

    mov r10, 0xFEE00000                    ; APIC base

    ; INIT IPI (broadcast to all APs)
    mov dword [r10 + 0x310], 0
    mov dword [r10 + 0x300], 0x000C4500
    mov ecx, 1000000
.d1: dec ecx
     jnz .d1

    ; SIPI → vector 0x08 = physical 0x8000
    mov dword [r10 + 0x310], 0
    mov dword [r10 + 0x300], 0x000C4608
    mov ecx, 100000
.d2: dec ecx
     jnz .d2

    ; second SIPI (spec says send twice)
    mov dword [r10 + 0x310], 0
    mov dword [r10 + 0x300], 0x000C4608
    ret


; ══════════════════════════════════════════════════════════════
; AP entry — APs land here after trampoline mode switch
; ══════════════════════════════════════════════════════════════

ap_entry:
    ; per-core stack from APIC ID
    mov r10, 0xFEE00000
    mov eax, [r10 + 0x20]
    shr eax, 24
    and eax, 0xFF
    shl rax, 12                            ; 4KB per core
    lea rsp, [ap_stacks + 4096*16]
    sub rsp, rax

    lock inc dword [ap_count]

    ; wait for genesis to complete
.ap_wait:
    pause
    cmp dword [genesis_done], 0
    je .ap_wait

    ; find-work loop (spin for now — walks come later)
.ap_spin:
    pause
    jmp .ap_spin


; ══════════════════════════════════════════════════════════════
; Trampoline — 16-bit → 32-bit → 64-bit for APs
; Raw bytes, hand-assembled. Copied to physical 0x8000.
; Reads GDT, CR3, and entry point from 0x8090+ data area.
; ══════════════════════════════════════════════════════════════

section .rodata
align 16
trampoline_code:
    ;  0: cli; xor ax,ax; mov ds,ax        — real mode setup
    ;  5: mov byte [0x8100],0x11           — debug: "I'm alive"
    ; 10: lgdt [0x8090]                    — load GDT from data area
    ; 16: mov eax,cr0; or al,1; mov cr0,eax — enable PE
    ; 22: jmp 0x18:0x8020                  — far jump to 32-bit (code32 seg)
    ; 32: mov byte [0x8100],0x22           — debug: "in 32-bit"
    ; 39: mov ax,0x10; mov ds,ax; mov es,ax; mov ss,ax — load data seg
    ; 49: mov eax,cr4; or eax,0x620; mov cr4,eax — PAE+SSE
    ; 61: mov eax,[0x8098]; mov cr3,eax    — load page table
    ; 69: mov ecx,0xC0000080; rdmsr; or eax,0x100; wrmsr — EFER.LME
    ; 81: mov eax,cr0; or eax,0x80000000; and eax,~0xC; mov cr0,eax — PG
    ; 93: mov byte [0x8100],0x33           — debug: "in 64-bit"
    ; 100: xor edi,edi; jmp far [0x80A0]   — jump to ap_entry
    db 0xfa, 0x31, 0xc0, 0x8e, 0xd8, 0xc6, 0x06, 0x00, 0x81, 0x11
    db 0x66, 0x0f, 0x01, 0x16, 0x90, 0x80, 0x0f, 0x20, 0xc0, 0x0c
    db 0x01, 0x0f, 0x22, 0xc0, 0x66, 0xea, 0x20, 0x80, 0x00, 0x00
    db 0x18, 0x00, 0xc6, 0x05, 0x00, 0x81, 0x00, 0x00, 0x22, 0x66
    db 0xb8, 0x10, 0x00, 0x8e, 0xd8, 0x8e, 0xc0, 0x8e, 0xd0, 0x0f
    db 0x20, 0xe0, 0x0d, 0x20, 0x06, 0x00, 0x00, 0x0f, 0x22, 0xe0
    db 0xa1, 0x98, 0x80, 0x00, 0x00, 0x0f, 0x22, 0xd8, 0xb9, 0x80
    db 0x00, 0x00, 0xc0, 0x0f, 0x32, 0x0d, 0x00, 0x01, 0x00, 0x00
    db 0x0f, 0x30, 0x0f, 0x20, 0xc0, 0x0d, 0x00, 0x00, 0x00, 0x80
    db 0x83, 0xe0, 0xf3, 0x0f, 0x22, 0xc0, 0xc6, 0x05, 0x00, 0x81
    db 0x00, 0x00, 0x33, 0xff, 0x2d, 0xa0, 0x80, 0x00, 0x00
trampoline_end:


; ═══════════════════════════════════════════════════════════════
; SECTION 2: HANDOFF — init JIT buffer, walk genesis, enter find-work
; ═══════════════════════════════════════════════════════════════

section .text
bits 64

; firmware location — injected by build script
%ifndef FWSECTORS
FWSECTORS equ 0
%endif
%ifndef FWSTART
FWSTART equ 0
%endif
%ifndef GPUSECTORS
GPUSECTORS equ 0
%endif
%ifndef GPUSTART
GPUSTART equ 0
%endif

Λ:
    ; init JIT code buffer
    lea rax, [code_base]
    mov [code_cursor], rax

    ; store loop memory base at 0x9200 (known convention, like FB at 0x9100)
    lea rax, [loop_region]
    mov [0x9200], rax

    ; firmware disk locations (read by ATA walks)
    ; CPU: 0x9300 = LBA start, 0x9304 = sector count
    ; GPU: 0x9318 = LBA start, 0x931C = sector count
    mov dword [0x9300], FWSTART
    mov dword [0x9304], FWSECTORS
    mov dword [0x9318], GPUSTART
    mov dword [0x931C], GPUSECTORS

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


; ═══════════════════════════════════════════════════════════════
; SECTION 3: NOMOS — ƒ(τ,χ,μ,φ) = τᵃ·χᵇ·μᶜ·φᵈ — the JIT compiler
; ═══════════════════════════════════════════════════════════════

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

align 64
loop_region: resb 8192                    ; 8KB for loop headers + buffers


; ── text ──────────────────────────────────────────────────────

section .text
bits 64


; ══════════════════════════════════════════════════════════════
; ƒ — resolve coordinates to function
;
; edi=τ, esi=χ, edx=μ, ecx=φ → rax = function pointer
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
    movsx r15d, cx                         ; φ

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
; Emits a call to ∏ with exponents as immediates.
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
    mov r8d, eax                           ; τ_exp
    mov eax, r13d
    sar eax, 1
    mov r9d, eax                           ; χ_exp
    mov eax, r14d
    sar eax, 1
    mov r10d, eax                          ; μ_exp
    mov eax, r15d
    sar eax, 1
    mov r11d, eax                          ; φ_exp

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

    ; emit: mov eax, ∏ (B8 xx xx xx xx)
    mov byte [rbx], 0xB8
    mov eax, ∏
    mov dword [rbx+1], eax
    add rbx, 5

    ; emit: call rax (FF D0)
    mov byte [rbx], 0xFF
    mov byte [rbx+1], 0xD0
    add rbx, 2

    ret


; ══════════════════════════════════════════════════════════════
; ∏ — runtime evaluator for τᵃ·χᵇ·μᶜ·φᵈ
;
; Called by JIT'd atom functions at walk time.
; rdi=τ_val, rsi=χ_val, rdx=μ_val, rcx=φ_val
; r8d=T_exp, r9d=D_exp, r10d=M_exp, r11d=Q_exp
; Returns result in rax.
; ══════════════════════════════════════════════════════════════

∏:
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; save dimension values (div clobbers rdx)
    mov r12, rdi                           ; τ_val
    mov r13, rsi                           ; χ_val
    mov r14, rdx                           ; μ_val
    mov r15, rcx                           ; φ_val

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
;  Width from |T|: 1=qword, 3=byte, 5=word, 7=dword.

ε_read:
    test r12d, r12d
    js .write

    ; +P (read)
    mov eax, r12d                          ; τ magnitude
    cmp eax, 3
    je .read_byte
    cmp eax, 5
    je .read_word
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

.read_word:
    ; movzx eax, word [rdi]  (0F B7 07)
    mov byte [rbx], 0x0F
    mov byte [rbx+1], 0xB7
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
    cmp eax, 5
    je .write_word
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

.write_word:
    ; mov [rdi], si   (66 89 37)
    mov byte [rbx], 0x66
    mov byte [rbx+1], 0x89
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
;  Width from |μ|: 1=byte, 3=dword, >=5=qword

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
    mov eax, r15d                          ; φ coordinate
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
;  πδ̄: port_read (byte)      π̄δ: port_write (byte)
;  π₇δ̄: port_read (dword)   π̄₇δ: port_write (dword)    — |T|≥7 selects width
;  πδ̄₃: MSR read             π̄δ₃: MSR write              — |Q|=3 selects register file
;  πδ̄₅: CPUID read                                        — |Q|=5 selects CPU identity
;  π̄δ̄: add                   π̄δ̄|3: subtract

ε_filter:
    mov eax, r15d                          ; φ
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
    ; +P -W: port_read, dword port_read, MSR read, or CPUID
    ; eax = |Q|, r12d = signed T (positive)

    cmp eax, 3
    je .msr_read
    cmp eax, 5
    je .cpuid_read

    cmp r12d, 7
    jge .port_read_dword

    ; byte: movzx edx, di (0F B7 D7) + in al, dx (EC) + movzx eax, al (0F B6 C0)
    mov byte [rbx], 0x0F
    mov byte [rbx+1], 0xB7
    mov byte [rbx+2], 0xD7
    mov byte [rbx+3], 0xEC
    mov byte [rbx+4], 0x0F
    mov byte [rbx+5], 0xB6
    mov byte [rbx+6], 0xC0
    add rbx, 7
    ret

.port_read_dword:
    ; dword: movzx edx, di (0F B7 D7) + in eax, dx (ED)
    mov byte [rbx], 0x0F
    mov byte [rbx+1], 0xB7
    mov byte [rbx+2], 0xD7
    mov byte [rbx+3], 0xED
    add rbx, 4
    ret

.msr_read:
    ; rdmsr: mov ecx, edi (89 F9) + rdmsr (0F 32)
    ;         + shl rdx, 32 (48 C1 E2 20) + or rax, rdx (48 09 D0)
    mov byte [rbx], 0x89
    mov byte [rbx+1], 0xF9
    mov byte [rbx+2], 0x0F
    mov byte [rbx+3], 0x32
    mov byte [rbx+4], 0x48
    mov byte [rbx+5], 0xC1
    mov byte [rbx+6], 0xE2
    mov byte [rbx+7], 0x20
    mov byte [rbx+8], 0x48
    mov byte [rbx+9], 0x09
    mov byte [rbx+10], 0xD0
    add rbx, 11
    ret

.cpuid_read:
    ; cpuid: arg0 = leaf (EAX input), returns EAX
    ; Also packs: vendor string check into result high bits
    ; For leaf 0: EAX=max leaf, EBX/ECX/EDX=vendor string
    ; For leaf 1: EAX=family/model/stepping
    ;
    ; emit: push rbx (53) + mov eax, edi (89 F8) + cpuid (0F A2)
    ;       + pop rbx (5B)
    ; cpuid clobbers EBX which we use as emit pointer — must save.
    ; But this runs at CALL time, not emit time. The JIT'd function
    ; is called later. At call time, rbx is free (not emit pointer).
    ; Actually: the walker calls the JIT'd function with rdi=arg0, etc.
    ; The JIT'd function runs, returns rax. rbx is caller-saved by our
    ; convention (walker saves it). But cpuid clobbers rbx/rcx/rdx.
    ; We only need EAX back. So: mov eax,edi + cpuid is enough.
    ; But we must preserve rbx if callee-saved. Let's push/pop to be safe.
    ;
    ; emit: push rbx (53) + mov eax, edi (89 F8) + cpuid (0F A2) + pop rbx (5B)
    mov byte [rbx], 0x53                   ; push rbx
    mov byte [rbx+1], 0x89                 ; mov eax, edi
    mov byte [rbx+2], 0xF8
    mov byte [rbx+3], 0x0F                 ; cpuid
    mov byte [rbx+4], 0xA2
    mov byte [rbx+5], 0x5B                 ; pop rbx
    add rbx, 6
    ret

.write_side:
    ; ── -P (write side) ──
    test ecx, ecx
    js .add

    ; -P +W: port_write, dword port_write, or MSR write
    ; eax = |Q|, r12d = signed T (negative)

    cmp eax, 3
    je .msr_write

    mov edx, r12d
    neg edx                                    ; |T|
    cmp edx, 7
    jge .port_write_dword

    ; byte: movzx edx, di (0F B7 D7) + mov al, sil (40 8A C6) + out dx, al (EE)
    ;       + xor eax, eax (31 C0)
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

.port_write_dword:
    ; dword: movzx edx, di (0F B7 D7) + mov eax, esi (89 F0) + out dx, eax (EF)
    ;        + xor eax, eax (31 C0)
    mov byte [rbx], 0x0F
    mov byte [rbx+1], 0xB7
    mov byte [rbx+2], 0xD7
    mov byte [rbx+3], 0x89
    mov byte [rbx+4], 0xF0
    mov byte [rbx+5], 0xEF
    mov byte [rbx+6], 0x31
    mov byte [rbx+7], 0xC0
    add rbx, 8
    ret

.msr_write:
    ; wrmsr: mov ecx, edi (89 F9) + mov rax, rsi (48 89 F0)
    ;        + mov rdx, rsi (48 89 F2) + shr rdx, 32 (48 C1 EA 20)
    ;        + wrmsr (0F 30) + xor eax, eax (31 C0)
    mov byte [rbx], 0x89
    mov byte [rbx+1], 0xF9
    mov byte [rbx+2], 0x48
    mov byte [rbx+3], 0x89
    mov byte [rbx+4], 0xF0
    mov byte [rbx+5], 0x48
    mov byte [rbx+6], 0x89
    mov byte [rbx+7], 0xF2
    mov byte [rbx+8], 0x48
    mov byte [rbx+9], 0xC1
    mov byte [rbx+10], 0xEA
    mov byte [rbx+11], 0x20
    mov byte [rbx+12], 0x0F
    mov byte [rbx+13], 0x30
    mov byte [rbx+14], 0x31
    mov byte [rbx+15], 0xC0
    add rbx, 16
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
    mov byte [rbx], 0xB8                   ; mov eax, λ
    mov eax, λ
    mov dword [rbx+1], eax
    add rbx, 5
    mov byte [rbx], 0xFF                   ; call rax
    mov byte [rbx+1], 0xD0
    add rbx, 2
    ret

λ:
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
    mov byte [rbx], 0xB8                   ; mov eax, ω
    mov eax, ω
    mov dword [rbx+1], eax
    add rbx, 5
    mov byte [rbx], 0xFF                   ; call rax
    mov byte [rbx+1], 0xD0
    add rbx, 2
    ret

ω:
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
    ; π∮δ = scan (accumulate).  π∮δ̄ = port bulk read (rep insd).
    ; dispatch on φ sign: positive = scan, negative = port bulk
    test r15d, r15d
    js .port_bulk

    ; ── scan (prefix sum) ──
    mov byte [rbx], 0xB8                   ; mov eax, Σ
    mov eax, Σ
    mov dword [rbx+1], eax
    add rbx, 5
    mov byte [rbx], 0xFF                   ; call rax
    mov byte [rbx+1], 0xD0
    add rbx, 2
    ret

.port_bulk:
    ; ── port bulk read: rep insd ──
    ; rdi = dest address, esi = port number, edx = dword count
    ; emit: xchg edx, ecx (87 D1) — count to ecx
    ;        movzx edx, si (0F B7 D6) — port to dx
    ;        cld (FC)
    ;        rep insd (F3 6D) — read count dwords from port to [rdi]
    ;        xor eax, eax (31 C0)
    mov byte [rbx], 0x87                   ; xchg edx, ecx
    mov byte [rbx+1], 0xD1
    mov byte [rbx+2], 0x0F                 ; movzx edx, si
    mov byte [rbx+3], 0xB7
    mov byte [rbx+4], 0xD6
    mov byte [rbx+5], 0xFC                 ; cld
    mov byte [rbx+6], 0xF3                 ; rep insd
    mov byte [rbx+7], 0x6D
    mov byte [rbx+8], 0x31                 ; xor eax, eax
    mov byte [rbx+9], 0xC0
    add rbx, 10
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


; ═══════════════════════════════════════════════════════════════
; SECTION 4: WALKER — ψ — wave byte interpreter
; ═══════════════════════════════════════════════════════════════

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
    xor r15d, r15d                 ; pipeline = 0

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
    jmp .skip_done                     ; +5 to account for instruction size

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


; ═══════════════════════════════════════════════════════════════
; GENESIS WALK DATA — assembled by hodos, included as raw bytes
; ═══════════════════════════════════════════════════════════════

section .rodata
global γ, γ_len
γ:      incbin ".build/genesis.bin"
γ_len:  dd (γ_len - γ)

section .data
