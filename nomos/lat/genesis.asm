; genesis.asm — the founding
;
; Runs once. Expresses hardware genes. Fills traces. Returns.
; After this, all cores are equal. Development ends. Life begins.
;
; All walks use wave byte format:
;   byte 0: wave byte (2 bits per dim, 10=extended → read i8)
;   [ext]:  signed i8 per extended dimension (T,D,M,Q order)
;   byte N: flags ([reserved:2][arg2:2][arg1:2][arg0:2])
;           00=pipeline  01=u8  10=u32  11=u64
;   bytes:  inline args per flags
;
; Most hardware is already working (BIOS configured it).
; Genesis only expresses genes for devices that need init.
; Everything else is non-local — just read/write the address.

bits 64

; ─── trace layout constants ──────────────────────────────────
TR_WCURSOR equ 0
TR_RCURSOR equ 8
TR_BUFADDR equ 16
TR_DEPTH   equ 24
TR_MASK    equ 32
TR_RECSZ   equ 40

TRACE_DEPTH equ 64
TRACE_MASK  equ (TRACE_DEPTH - 1)

; ─── BSS: traces + buffers ───────────────────────────────────

section .bss

alignb 64
trace_kbd:
    resb 64                    ; trace header (cache-line aligned)
trace_kbd_buf:
    resb TRACE_DEPTH * 8      ; 64 entries × 8 bytes

; stash for walk temp storage
global stash_a
stash_a: resq 1

section .rodata

global main_walk, main_len

main_walk:

; ─── 1. proof of life (debugcon — non-local, no init needed) ──
;
; port_write = (-1,0,0,+1) = 0xC1.  flags 0x05 = arg0:u8, arg1:u8

    db 0xC1, 0x05, 0xE9, 'g'
    db 0xC1, 0x05, 0xE9, 'e'
    db 0xC1, 0x05, 0xE9, 'n'
    db 0xC1, 0x05, 0xE9, 'e'
    db 0xC1, 0x05, 0xE9, 's'
    db 0xC1, 0x05, 0xE9, 'i'
    db 0xC1, 0x05, 0xE9, 's'
    db 0xC1, 0x05, 0xE9, 10

; ─── 2. clear pixel buffer (MBR did VESA, FB address at 0x9100) ──
;
; rdd (+7,0,0,0): T=extended(10), wave byte 0x80, ext T=+7
; filld (-1,0,-3,0): T=-1(11), M=extended(10), wave byte 0xC8, ext M=-3

    db 0x80, 7                              ; rdd: T=+7 (dword read)
    db 0x02                                 ; flags: arg0=u32
    dd 0x9100                               ; FB address location

    db 0xC8                                 ; filld: T=-1, M=extended
    db (-3) & 0xFF                          ; ext M = -3
    db 0x28                                 ; flags: arg0=pipe, arg1=u32, arg2=u32
    dd 0x00336699                           ; blue
    dd (1024*768)                           ; pixel count

; ─── 3. speaker beep (gene — PIT needs config) ──────────────
;
; apply (+C) = 0x10.  walk_sub bridges to sub-walks.

    db 0x10                                 ; apply
    db 0x2A                                 ; flags: arg0=u32, arg1=u32, arg2=u32
    dd walk_sub
    dd walk_speaker_beep
    dd (walk_speaker_beep_end - walk_speaker_beep)

; ─── 4. init keyboard trace ─────────────────────────────────
;
; write (-P) = 0xC0.  set trace header fields.

    db 0xC0, 0x0A                           ; write: arg0=u32, arg1=u32
    dd (trace_kbd + TR_BUFADDR)
    dd trace_kbd_buf

    db 0xC0, 0x06                           ; write: arg0=u32, arg1=u8
    dd (trace_kbd + TR_DEPTH)
    db TRACE_DEPTH

    db 0xC0, 0x06                           ; write: arg0=u32, arg1=u8
    dd (trace_kbd + TR_MASK)
    db TRACE_MASK

    db 0xC0, 0x06                           ; write: arg0=u32, arg1=u8
    dd (trace_kbd + TR_RECSZ)
    db 8

    db 0xC1, 0x05, 0xE9, 'T'               ; debugcon: trace ready

; ─── 5. return — development ends, life begins ───────────────
;
; After this, lattice_start enters the keyboard poll loop.
; The BSP becomes the keyboard handler — not by assignment,
; by being the first core to find work.

main_len: dd (main_len - main_walk)


; ═══════════════════════════════════════════════════════════════════
; KEYBOARD POLL — walk DNA for the keyboard trace
;
; Polls port 0x64 for a key, reads port 0x60 if ready, writes
; the scancode to debugcon. Loops forever via loop_back.
;
; Wave byte encoding:
;   port_read  (+1,0,0,-1) = 0x43     AND (0,0,0,+3) = 0x02 ext Q=3
;   port_write (-1,0,0,+1) = 0xC1     add (-1,0,0,-1) = 0xC3
;
; Offsets (verify skip/loop counts against these):
;   0: port_read(0x64)    3 bytes
;   3: AND(pipe, 1)       4 bytes
;   7: skip_z(6)          5 bytes
;  12: port_read(0x60)    3 bytes
;  15: port_write(0xE9)   3 bytes
;  18: add(1, 0)          4 bytes
;  22: loop_back(22)      5 bytes
;  27: end
; ═══════════════════════════════════════════════════════════════════

section .rodata

global kbd_walk, kbd_walk_len

kbd_walk:
    ; read PS/2 status (port 0x64)
    db 0x43, 0x01, 0x64                    ; port_read(0x64) → pipeline

    ; test bit 0 (data ready?) — AND is W at |Q|=3
    db 0x02, 3                             ; wave=0x02(Q=ext), ext Q=+3
    db 0x04, 1                             ; flags: arg0=pipe, arg1=u8(1)

    ; no key? skip to add(1,0) — keep looping
    db 0xFE                                ; skip_z
    dd 6                                   ; skip 6 bytes forward

    ; key ready — read scancode (port 0x60)
    db 0x43, 0x01, 0x60                    ; port_read(0x60) → pipeline

    ; write scancode to debugcon
    db 0xC1, 0x01, 0xE9                    ; port_write(0xE9, pipeline)

    ; force pipeline = 1 for loop_back
    db 0xC3, 0x05, 1, 0                    ; add(1, 0) → pipeline = 1

    ; loop back to start
    db 0xFC                                ; loop_back
    dd 22                                  ; rewind 22 bytes

kbd_walk_end:
kbd_walk_len: dd (kbd_walk_end - kbd_walk)


; ═══════════════════════════════════════════════════════════════════
; GENES — only for hardware that needs configuration
; ═══════════════════════════════════════════════════════════════════

section .rodata

%include "lat/hw/speaker.asm"

; ─── FONT DATA ─────────────────────────────────────────────────
align 16
global font_8x16
font_8x16:
    incbin "lat/font.ha"


; ═══════════════════════════════════════════════════════════════════
; SCAFFOLDING — walk_sub bridges walker_wave to sub-walks
; ═══════════════════════════════════════════════════════════════════

section .text

global walk_sub
walk_sub:
    mov rdi, rsi
    mov esi, edx
    extern walker_wave
    call walker_wave
    ret
