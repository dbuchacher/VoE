; ═══════════════════════════════════════════════════════════════════
; mbr.asm — Master Boot Record (512 bytes)
;
; BIOS → load kernel → VESA 1024x768x32 → protected mode → go.
; FB info stored at 0x9100 for the lattice (addr, bpp, pitch).
; ═══════════════════════════════════════════════════════════════════

bits 16
org 0x7C00

KERNEL_LOAD equ 0x100000
LOAD1_SEG   equ 0x1000
LOAD2_SEG   equ 0x2000
LOAD_PHYS   equ 0x10000
; sector counts — set by build script via -DSEC1=N -DSEC2=N
%ifndef SEC1
SEC1        equ 127
%endif
%ifndef SEC2
SEC2        equ 127
%endif
SECTORS     equ (SEC1 + SEC2)
VESA_INFO   equ 0x9000
FB_INFO     equ 0x9100         ; 8 bytes: addr(4) + bpp(2) + pitch(2)

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drive], dl

    ; A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; ── load kernel ──
    mov word  [dap_count], SEC1
    mov word  [dap_off],   0
    mov word  [dap_seg],   LOAD1_SEG
    mov dword [dap_lba],   1
    mov ah, 0x42
    mov dl, [boot_drive]
    lea si, [dap]
    int 0x13
    jc .error

%if SEC2 > 0
    mov word  [dap_count], SEC2
    mov word  [dap_seg],   LOAD2_SEG
    mov dword [dap_lba],   1 + SEC1
    mov ah, 0x42
    mov dl, [boot_drive]
    lea si, [dap]
    int 0x13
    jc .error
%endif

    mov al, 'D'
    out 0xE9, al

    ; ── VESA: find 1024x768x32 by enumerating modes ──

    ; get VBE info → mode list pointer at vbe_buf+14
    mov ax, 0x4F00
    mov di, 0x9200             ; VBE info block (512 bytes at 0x9200)
    mov dword [di], 'VBE2'    ; request VBE 2.0+ info
    int 0x10
    cmp ax, 0x004F
    jne .no_vesa

    ; load mode list pointer (far ptr: offset at +14, segment at +16)
    mov si, [0x9200 + 14]
    mov fs, [0x9200 + 16]

.scan_mode:
    mov cx, [fs:si]
    cmp cx, 0xFFFF             ; end of list
    je .no_vesa

    ; get mode info
    push si
    push cx
    mov ax, 0x4F01
    or cx, 0x4000              ; request linear FB info
    mov di, VESA_INFO
    int 0x10
    pop cx
    pop si

    cmp ax, 0x004F
    jne .next_mode

    ; check 1024x768x32
    cmp word [VESA_INFO + 18], 1024
    jne .next_mode
    cmp word [VESA_INFO + 20], 768
    jne .next_mode
    cmp byte [VESA_INFO + 25], 32
    jne .next_mode
    test byte [VESA_INFO], 0x80       ; linear FB supported?
    jz .next_mode

    ; found it — set the mode
    mov bx, cx
    or bx, 0x4000             ; linear FB bit
    mov ax, 0x4F02
    int 0x10
    cmp ax, 0x004F
    jne .no_vesa

    ; store FB info
    mov eax, [VESA_INFO + 40]
    mov [FB_INFO], eax
    mov byte [FB_INFO + 4], 32
    mov ax, [VESA_INFO + 16]
    mov [FB_INFO + 6], ax

    mov al, 'V'
    out 0xE9, al
    jmp .go_pm

.next_mode:
    add si, 2
    jmp .scan_mode

.no_vesa:
    mov dword [FB_INFO], 0
    mov al, 'v'
    out 0xE9, al

.go_pm:
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:.pm32

bits 32
.pm32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x7C00
    mov esi, LOAD_PHYS
    mov edi, KERNEL_LOAD
    mov ecx, SECTORS * 512 / 4
    cld
    rep movsd
    jmp KERNEL_LOAD

bits 16
.print_hex:
    ; print eax as 8 hex digits to debugcon
    mov ecx, 8
.ph_loop:
    rol eax, 4
    push eax
    and al, 0x0F
    add al, '0'
    cmp al, '9'
    jle .ph_ok
    add al, 7
.ph_ok:
    out 0xE9, al
    pop eax
    dec ecx
    jnz .ph_loop
    ret

.error:
    mov al, '!'
    out 0xE9, al
    hlt
    jmp .error

boot_drive: db 0

align 4
dap:
    db 0x10, 0
dap_count: dw 0
dap_off:   dw 0
dap_seg:   dw 0
dap_lba:   dd 0, 0

align 8
gdt:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_desc:
    dw gdt_desc - gdt - 1
    dd gdt

times 510 - ($ - $$) db 0
dw 0xAA55
