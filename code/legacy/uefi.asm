; uefi.asm — minimal UEFI PE32+ stub that boots VoE
;
; nasm -f bin uefi.asm -o BOOTX64.EFI
;
; Single-section PE: code + data + kernel in one .text section.
; This avoids cross-section [rel] displacement errors when UEFI
; maps sections to different virtual addresses than file offsets.
;
; Flow: GOP query → ExitBootServices → copy ELF to 1MB →
;       page tables → PIC mask → trampoline copy → jump to long_mode

bits 64
org 0

; ─── DOS header ───────────────────────────────────────────────

dos_hdr:
    dw 0x5A4D                       ; "MZ"
    times 29 dw 0
    dd pe_hdr
    times 64 - ($ - dos_hdr) db 0

; ─── PE header ────────────────────────────────────────────────

pe_hdr:
    dd 0x00004550                   ; "PE\0\0"
    dw 0x8664                       ; AMD64
    dw 2                            ; 2 sections: .text (code+data), .reloc
    dd 0, 0, 0
    dw opt_end - opt_hdr
    dw 0x2022

opt_hdr:
    dw 0x020B                       ; PE32+
    db 0, 0
    dd text_size                    ; SizeOfCode (includes data)
    dd 0                            ; SizeOfInitializedData
    dd 0                            ; SizeOfUninitializedData
    dd text_rva                     ; AddressOfEntryPoint
    dd text_rva                     ; BaseOfCode
    dq 0x00400000                   ; ImageBase
    dd 0x1000                       ; SectionAlignment
    dd 0x200                        ; FileAlignment
    dw 0, 0, 0, 0
    dw 0, 0
    dd 0
    dd img_size
    dd hdr_size
    dd 0
    dw 10                           ; EFI_APPLICATION
    dw 0
    dq 0x10000, 0x10000, 0x10000, 0
    dd 0
    dd 6
    dq 0, 0, 0, 0, 0               ; data dirs 0-4
    dd reloc_rva, reloc_size        ; 5: BaseRelocation
opt_end:

    ; .text section (code + data + embedded kernel)
    db ".text", 0, 0, 0
    dd text_size, text_rva
    dd text_raw, text_off
    dd 0, 0
    dw 0, 0
    dd 0xE0000060                   ; CODE | INITIALIZED_DATA | READ | WRITE | EXECUTE

    ; .reloc section (required dummy)
    db ".reloc", 0, 0
    dd reloc_size, reloc_rva
    dd reloc_raw, reloc_off
    dd 0, 0
    dw 0, 0
    dd 0x42000040

    times 0x200 - ($ - $$) db 0
    hdr_size equ 0x200

; ═══════════════════════════════════════════════════════════════
; .TEXT — everything in one section (code + data + kernel)
; ═══════════════════════════════════════════════════════════════

text_off equ ($ - $$)
text_rva equ 0x1000

%macro dbg 1
    mov al, %1
    out 0xE9, al
    mov dx, 0x3F8
    out dx, al
%endmacro

efi_main:
    sub rsp, 64
    mov [rel v_handle], rcx
    mov rax, [rdx + 96]            ; BootServices
    mov [rel v_bs], rax

    dbg 'U'

    ; ── GOP framebuffer ──
    lea rcx, [rel gop_guid]
    xor rdx, rdx
    lea r8, [rel v_gop]
    mov rax, [rel v_bs]
    call [rax + 320]               ; LocateProtocol
    test rax, rax
    jnz .no_gop

    mov rax, [rel v_gop]
    mov rax, [rax + 24]            ; ->Mode
    mov rbx, [rax + 24]            ; ->FrameBufferBase
    mov rcx, [rax + 8]             ; ->Info
    mov edx, [rcx + 4]             ; HorizontalResolution
    mov esi, [rcx + 8]             ; VerticalResolution
    mov edi, [rcx + 32]            ; PixelsPerScanLine
    mov [0x9100], ebx
    mov byte [0x9100 + 4], 32
    shl edi, 2
    mov [0x9100 + 6], di
    dbg 'G'
    jmp .got_fb

.no_gop:
    mov dword [0x9100], 0
    dbg 'g'

.got_fb:
    ; ── ExitBootServices ──
.getmap:
    mov qword [rel v_mmsz], 16384  ; 16KB for real hardware
    lea rcx, [rel v_mmsz]
    lea rdx, [rel v_mmbuf]
    lea r8,  [rel v_mmkey]
    lea r9,  [rel v_mmdsz]
    lea rax, [rel v_mmdvr]
    mov [rsp+32], rax
    mov rax, [rel v_bs]
    call [rax + 56]                ; GetMemoryMap
    mov rcx, [rel v_handle]
    mov rdx, [rel v_mmkey]
    mov rax, [rel v_bs]
    call [rax + 232]               ; ExitBootServices
    test rax, rax
    jz .exited
    jmp .getmap

.exited:
    cli
    dbg 'E'

    ; ── copy ELF LOAD segments to physical addresses ──
    lea rsi, [rel kernel_elf]

    mov r12d, [rsi + 24]           ; e_entry
    mov eax, [rsi + 28]            ; e_phoff
    lea rbx, [rsi + rax]
    movzx r14d, word [rsi + 42]    ; e_phentsize
    movzx r15d, word [rsi + 44]    ; e_phnum

.copy_seg:
    test r15d, r15d
    jz .copy_done
    dec r15d
    cmp dword [rbx], 1
    jne .next_seg

    mov edi, [rbx + 8]            ; p_vaddr
    mov r8d, [rbx + 4]            ; p_offset
    lea r9, [rsi + r8]
    mov ecx, [rbx + 16]           ; p_filesz
    push rsi
    push rdi
    mov rsi, r9
    cld
    rep movsb
    pop rax
    push rax
    mov ecx, [rbx + 20]           ; p_memsz
    sub ecx, [rbx + 16]
    jle .no_bss
    xor al, al
    rep stosb
.no_bss:
    pop rdi
    pop rsi

.next_seg:
    add rbx, r14
    jmp .copy_seg

.copy_done:
    dbg 'K'

    ; ── page tables: identity map 0-4GB via 1GB pages ──
    PML4 equ 0x70000
    PDPT equ 0x71000

    xor eax, eax
    mov edi, PML4
    mov ecx, 4096*2/4
    cld
    rep stosd

    mov dword [PML4], PDPT | 3
    mov dword [PML4+4], 0
    mov dword [PDPT + 0*8],     0x00000083
    mov dword [PDPT + 0*8 + 4], 0
    mov dword [PDPT + 1*8],     0x40000083
    mov dword [PDPT + 1*8 + 4], 0
    mov dword [PDPT + 2*8],     0x80000083
    mov dword [PDPT + 2*8 + 4], 0
    mov dword [PDPT + 3*8],     0xC0000083
    mov dword [PDPT + 3*8 + 4], 0

    mov rax, PML4
    mov cr3, rax

    mov rax, cr4
    or eax, (1<<9)|(1<<10)
    mov cr4, rax
    mov rax, cr0
    and eax, ~((1<<2)|(1<<3))
    mov cr0, rax

    ; ── pre-kernel setup ──
    mov al, 0xFF
    out 0x21, al
    out 0xA1, al

%if TRAMP_CODE > 0
    mov esi, TRAMP_CODE
    mov edi, 0x8000
    mov ecx, TRAMP_END - TRAMP_CODE
    cld
    rep movsb
%endif

    ; GDT at 0x6F00 (64-bit GDTR format)
    mov qword [0x6F00], 0
    mov dword [0x6F08], 0x0000FFFF
    mov dword [0x6F0C], 0x00AF9A00
    mov dword [0x6F10], 0x0000FFFF
    mov dword [0x6F14], 0x00CF9200
    mov dword [0x6F18], 0x0000FFFF
    mov dword [0x6F1C], 0x00CF9A00
    mov word  [0x6F80], 31
    mov qword [0x6F82], 0x6F00
    lgdt [0x6F80]

    ; patch kernel's gdtr
    mov word  [GDTR_ADDR], 31
    mov dword [GDTR_ADDR+2], 0x6F00

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov rsp, 0x123000

    dbg 'J'

    mov rax, LONG_MODE_ADDR
    jmp rax

; ─── data (same section as code) ─────────────────────────────

align 8
v_handle:  dq 0
v_bs:      dq 0
v_gop:     dq 0
v_mmsz:    dq 16384
v_mmkey:   dq 0
v_mmdsz:   dq 0
v_mmdvr:   dq 0
v_mmbuf:   times 16384 db 0

gop_guid:
    dd 0x9042a9de
    dw 0x23dc, 0x4a38
    db 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a

align 16
kernel_elf:
    incbin ".build/nomos.elf"
kernel_elf_end:

text_end equ ($ - $$)
text_size equ text_end - text_off
    times (((text_size + 0x1FF) / 0x200) * 0x200) - text_size db 0
text_raw equ ($ - $$) - text_off

; ═══════════════════════════════════════════════════════════════
; .RELOC — dummy
; ═══════════════════════════════════════════════════════════════

reloc_off equ ($ - $$)
reloc_rva equ text_rva + (((text_size + 0xFFF) / 0x1000) * 0x1000)

    dd 0
    dd 8

reloc_end equ ($ - $$)
reloc_size equ reloc_end - reloc_off
    times (((reloc_size + 0x1FF) / 0x200) * 0x200) - reloc_size db 0
reloc_raw equ ($ - $$) - reloc_off

img_size equ reloc_rva + (((reloc_size + 0xFFF) / 0x1000) * 0x1000)
