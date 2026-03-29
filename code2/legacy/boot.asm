; boot.asm — MBR bootloader (512 bytes, 16-bit real mode)
;
; BIOS loads this from disk sector 0 at 0x7C00. We:
;   1. Enable A20 (fast gate)
;   2. Load kernel from disk via INT 13h (LBA, DAP)
;   3. Enumerate VESA modes → find 1024×768×32bpp linear FB
;   4. Store FB info at 0x9100 (genesis reads it later)
;   5. Switch to 32-bit protected mode
;   6. Copy kernel from 0x10000 to 1MB (where linker placed it)
;   7. Jump to _start (cpu.asm)
;
; If VESA fails, FB address = 0. Genesis skips display.
; SEC1/SEC2 set by build script: how many disk sectors to load.

bits 16
org 0x7C00

LOAD_SEG1   equ 0x1000              ; 0x10000 physical
LOAD_SEG2   equ 0x2000              ; 0x20000 physical
LOAD_PHYS   equ 0x10000
KERNEL_DEST equ 0x100000            ; 1MB — linker expects it here
VESA_INFO   equ 0x9000              ; mode info block scratch
FB_INFO     equ 0x9100              ; 8 bytes: addr(4) + bpp(2) + pitch(2)

; sector counts — build script injects via -DSEC1=N -DSEC2=N
%ifndef SEC1
SEC1 equ 127
%endif
%ifndef SEC2
SEC2 equ 127
%endif
SECTORS equ (SEC1 + SEC2)

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drive], dl            ; BIOS passes boot drive in DL

    ; ── A20 gate (fast method) ───────────────────────────────
    in al, 0x92
    or al, 2
    out 0x92, al

    ; ── load kernel from disk ────────────────────────────────
    ; INT 13h AH=42h: extended read using DAP (LBA addressing)
    ; First chunk: up to SEC1 sectors at 0x1000:0000
    mov word  [dap_count], SEC1
    mov word  [dap_off],   0
    mov word  [dap_seg],   LOAD_SEG1
    mov dword [dap_lba],   1        ; kernel starts at sector 1
    mov ah, 0x42
    mov dl, [boot_drive]
    lea si, [dap]
    int 0x13
    jc .error

%if SEC2 > 0
    ; Second chunk: remaining sectors at 0x2000:0000
    mov word  [dap_count], SEC2
    mov word  [dap_seg],   LOAD_SEG2
    mov dword [dap_lba],   1 + SEC1
    mov ah, 0x42
    mov dl, [boot_drive]
    lea si, [dap]
    int 0x13
    jc .error
%endif

%ifdef DEBUG
    mov al, 'D'                     ; disk loaded
    out 0xE9, al
%endif

    ; ── VESA: enumerate modes, find 1024×768×32 ──────────────
    ; INT 10h AX=4F00h: get VBE controller info
    mov ax, 0x4F00
    mov di, 0x9200                  ; VBE info block (512 bytes)
    mov dword [di], 'VBE2'         ; request VBE 2.0+ info
    int 0x10
    cmp ax, 0x004F
    jne .no_vesa

    ; mode list is a far pointer at VBE info block offset +14
    mov si, [0x9200 + 14]           ; offset
    mov fs, [0x9200 + 16]           ; segment

.scan_mode:
    mov cx, [fs:si]
    cmp cx, 0xFFFF                  ; end of mode list
    je .no_vesa

    ; INT 10h AX=4F01h: get mode info for mode CX
    push si
    push cx
    mov ax, 0x4F01
    or cx, 0x4000                   ; request linear FB info
    mov di, VESA_INFO
    int 0x10
    pop cx
    pop si

    cmp ax, 0x004F
    jne .next_mode

    ; check: 1024 wide, 768 high, 32 bpp, linear FB capable
    cmp word [VESA_INFO + 18], 1024
    jne .next_mode
    cmp word [VESA_INFO + 20], 768
    jne .next_mode
    cmp byte [VESA_INFO + 25], 32
    jne .next_mode
    test byte [VESA_INFO], 0x80     ; mode attributes: linear FB?
    jz .next_mode

    ; found it — set the mode
    mov bx, cx
    or bx, 0x4000                   ; linear framebuffer bit
    mov ax, 0x4F02
    int 0x10
    cmp ax, 0x004F
    jne .no_vesa

    ; store FB info for genesis
    mov eax, [VESA_INFO + 40]       ; physical FB address
    mov [FB_INFO], eax
    mov byte [FB_INFO + 4], 32      ; bpp
    mov ax, [VESA_INFO + 16]        ; pitch (bytes per line)
    mov [FB_INFO + 6], ax

%ifdef DEBUG
    mov al, 'V'                     ; VESA set
    out 0xE9, al
%endif
    jmp .go_pm

.next_mode:
    add si, 2
    jmp .scan_mode

.no_vesa:
    mov dword [FB_INFO], 0          ; no framebuffer — genesis skips display
%ifdef DEBUG
    mov al, 'v'                     ; VESA failed
    out 0xE9, al
%endif

    ; ── protected mode ───────────────────────────────────────
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

    ; copy kernel from 0x10000 to 1MB
    mov esi, LOAD_PHYS
    mov edi, KERNEL_DEST
    mov ecx, SECTORS * 512 / 4
    cld
    rep movsd

    ; jump to cpu.asm _start at 1MB
    jmp KERNEL_DEST

; ── error handler ────────────────────────────────────────────
bits 16
.error:
    mov al, '!'
    out 0xE9, al
    hlt
    jmp .error

; ── data ─────────────────────────────────────────────────────
boot_drive: db 0

align 4
dap:
    db 0x10, 0                      ; DAP size, reserved
dap_count: dw 0                     ; sector count
dap_off:   dw 0                     ; offset
dap_seg:   dw 0                     ; segment
dap_lba:   dd 0, 0                  ; LBA (64-bit)

align 8
gdt:
    dq 0                            ; null descriptor
    dq 0x00CF9A000000FFFF          ; 0x08: code32 (flat, execute/read)
    dq 0x00CF92000000FFFF          ; 0x10: data32 (flat, read/write)
gdt_desc:
    dw gdt_desc - gdt - 1
    dd gdt

times 510 - ($ - $$) db 0
dw 0xAA55                          ; MBR signature
