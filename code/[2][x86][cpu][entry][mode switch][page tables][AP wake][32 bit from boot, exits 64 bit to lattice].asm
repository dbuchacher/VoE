; ═══════════════════════════════════════════════════════════════════
; [2] CPU entry — 32-bit to 64-bit mode switch, then hand off to lattice
;
; The MBR [1] jumps here at 1MB. We're in 32-bit protected mode.
; The CPU can't run 64-bit code yet. This file fixes that.
;
; What this does:
;   1. Copy the AP trampoline to 0x8000 (low memory, for waking other cores later)
;   2. Build page tables (identity map 0-1GB + 3-4GB for MMIO)
;   3. Enable PAE, long mode (EFER.LME), and paging — now we're 64-bit
;   4. Load a 64-bit GDT and far jump into long mode
;   5. Zero BSS (the bootloader didn't do it for us in multiboot path)
;   6. Wake secondary cores via INIT/SIPI IPI sequence
;   7. Jump to lattice_start [3] — never returns
;
; The trampoline (raw bytes at the bottom) is a separate 16→32→64 bit
; mode switch that gets copied to physical 0x8000. APs execute it after
; receiving the SIPI. It's raw hex because it runs at a fixed physical
; address — NASM can't relocate it. Each byte is hand-assembled.
;
; APs (secondary cores) wake up, get a per-core stack from their APIC ID,
; and spin waiting for work. The bind drain (not built yet) will give them
; loops to process. For now they just pause-loop.
;
; After this file runs, we're in 64-bit mode with all cores awake.
; x86 setup is done. The lattice takes over.
; ═══════════════════════════════════════════════════════════════════

; ─── entry trampoline (MBR jumps to 1MB, this is the first byte) ──

section .multiboot
bits 32
    jmp _start                             ; 5 bytes — MBR lands here
    align 4
    dd 0x1BADB002                          ; multiboot magic
    dd 0x00000003                          ; flags: align + meminfo
    dd -(0x1BADB002 + 0x00000003)          ; checksum

; ─── bss ───────────────────────────────────────────────────────

section .bss
align 16
stack_bottom:   resb 16384
global stack_top
stack_top:

align 4096
global ap_stacks
ap_stacks:      resb 4096 * 16            ; 16 AP stacks, 4K each

; page tables — in dedicated section so BSS zeroing doesn't clobber them
section .pagetables nobits alloc write
align 4096
pml4:           resb 4096
pdpt:           resb 4096
pd0:            resb 4096                  ; low memory 0-1GB
pd3:            resb 4096                  ; high memory (MMIO)

section .bss

; ─── data ──────────────────────────────────────────────────────

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

global ap_count, trampoline_ready
ap_count:         dd 0
trampoline_ready: db 0

; ─── 32-bit entry ──────────────────────────────────────────────

section .text
bits 32

global _start
extern lattice_start

_start:
    cli

    ; mask all PIC IRQs — BIOS leaves them unmasked. Without this,
    ; KVM's interrupt-window logic during MMIO FB writes (EPT violations)
    ; can cause subtle corruption with our stale IDTR. We poll, not interrupt.
    mov al, 0xFF
    out 0x21, al                               ; mask all master PIC IRQs
    out 0xA1, al                               ; mask all slave PIC IRQs

    ; copy trampoline to 0x8000 (for AP wake later)
    mov esi, trampoline_code
    mov edi, 0x8000
    mov ecx, trampoline_code_end
    sub ecx, trampoline_code
    cld
    rep movsb

    ; ── build page tables ──

    ; PML4[0] → PDPT
    mov eax, pdpt
    or eax, 3
    mov [pml4], eax

    ; PDPT[0] → PD0 (low 1GB)
    mov eax, pd0
    or eax, 3
    mov [pdpt], eax

    ; PDPT[3] → PD3 (high 1GB, for MMIO)
    mov eax, pd3
    or eax, 3
    mov [pdpt + 3*8], eax

    ; PD0: identity map 0-1GB as 2MB huge pages
    xor ecx, ecx
    mov eax, 0x83                          ; present | writable | 2MB
.map_low:
    mov [pd0 + ecx*8], eax
    mov dword [pd0 + ecx*8 + 4], 0
    add eax, 0x200000
    inc ecx
    cmp ecx, 512
    jl .map_low

    ; PD3: map 3-4GB (MMIO region) as 2MB uncacheable pages
    xor ecx, ecx
    mov eax, 0xC0000083                    ; base 3GB | present | writable | 2MB
.map_high:
    mov [pd3 + ecx*8], eax
    mov dword [pd3 + ecx*8 + 4], 0
    add eax, 0x200000
    inc ecx
    cmp ecx, 512
    jl .map_high

    ; APIC MMIO page: mark uncacheable (PCD bit)
    or dword [pd3 + 503 * 8], 0x10

    ; ── enable long mode ──

    ; load page table
    mov eax, pml4
    mov cr3, eax

    ; enable PAE + SSE support
    mov eax, cr4
    or eax, (1<<5)|(1<<9)|(1<<10)         ; PAE | OSFXSR | OSXMMEXCPT
    mov cr4, eax

    ; enable long mode (EFER.LME)
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1<<8)
    wrmsr

    ; enable paging, clear FPU emulation
    mov eax, cr0
    or eax, (1<<31)                        ; PG
    and eax, ~((1<<2)|(1<<3))              ; clear EM, TS
    mov cr0, eax

    ; load GDT and far jump to 64-bit
    lgdt [gdtr]
    jmp 0x08:long_mode

; ─── 64-bit entry ──────────────────────────────────────────────

bits 64

long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, stack_top

    ; ── zero BSS ──
    extern __bss_start, __bss_end
    lea rdi, [__bss_start]
    lea rcx, [__bss_end]
    sub rcx, rdi
    shr rcx, 3                             ; count in qwords
    xor eax, eax
    rep stosq

    ; ── wake APs ──
    call wake_cores

    ; ── hand off to the lattice ──
    jmp lattice_start


; ═══════════════════════════════════════════════════════════════════
; AP WAKE — INIT/SIPI sequence
; ═══════════════════════════════════════════════════════════════════

wake_cores:
    ; write trampoline parameters (data area at 0x8090+)
    ; GDT pointer
    mov ax, [gdtr]
    mov [0x8090], ax
    mov eax, [gdtr + 2]
    mov [0x8092], eax
    ; page table base
    mov rax, cr3
    mov [0x8098], eax
    ; 64-bit entry point
    lea rax, [ap_entry_64]
    mov [0x80A0], eax
    mov word [0x80A4], 0x08                ; code64 selector

    mov r10, 0xFEE00000                    ; APIC base

    ; INIT IPI (broadcast to all APs)
    mov dword [r10 + 0x310], 0
    mov dword [r10 + 0x300], 0x000C4500
    ; wait ~10ms
    mov ecx, 1000000
.d1: dec ecx
    jnz .d1

    ; SIPI → vector 0x08 = APs start at physical 0x8000
    mov dword [r10 + 0x310], 0
    mov dword [r10 + 0x300], 0x000C4608
    mov ecx, 100000
.d2: dec ecx
    jnz .d2

    ; second SIPI (spec says send twice)
    mov dword [r10 + 0x310], 0
    mov dword [r10 + 0x300], 0x000C4608
    ret


; ═══════════════════════════════════════════════════════════════════
; AP ENTRY — APs land here after trampoline mode switch
; ═══════════════════════════════════════════════════════════════════

ap_entry_64:
    ; per-core stack from APIC ID
    mov r10, 0xFEE00000
    mov eax, [r10 + 0x20]
    shr eax, 24
    and eax, 0xFF
    shl rax, 12                            ; 4KB per core
    lea rsp, [ap_stacks + 4096*16]
    sub rsp, rax

    lock inc dword [ap_count]

    ; spin until DNA is assigned (TODO: bind drain)
.ap_spin:
    pause
    jmp .ap_spin


; ═══════════════════════════════════════════════════════════════════
; TRAMPOLINE — 16-bit → 32-bit → 64-bit mode switch for APs
; Assembled separately, inlined as raw bytes, copied to 0x8000
; ═══════════════════════════════════════════════════════════════════

section .rodata
align 16
global trampoline_code, trampoline_code_end

trampoline_code:
    ; 109 bytes: real mode → protected → long mode → far jump to ap_entry_64
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
trampoline_code_end:


