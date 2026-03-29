; cpu.asm — x86 substrate: entry, mode switch, page tables, AP wake
;
; Multiboot loader drops us here in 32-bit protected mode.
; We build page tables, switch to 64-bit, wake all cores,
; then hand off to the lattice. After this, x86 setup is done.

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
global stack_top
stack_top:

align 4096
global ap_stacks
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

global gdt, gdtr
gdt:            dq 0                       ; null descriptor
                dq 0x00AF9A000000FFFF      ; 0x08: code64 (L=1)
                dq 0x00CF92000000FFFF      ; 0x10: data64
                dq 0x00CF9A000000FFFF      ; 0x18: code32 (trampoline)
gdt_end:
gdtr:           dw gdt_end - gdt - 1
                dd gdt

global ap_count, genesis_done
ap_count:       dd 0
genesis_done:   dd 0

; ── 32-bit entry ──────────────────────────────────────────────

section .text
bits 32

global _start
extern Λ

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

long_mode:
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
