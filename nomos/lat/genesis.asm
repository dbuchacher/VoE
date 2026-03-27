; genesis.asm — the founding
;
; Runs once. Expresses hardware genes. Fills traces. Returns.
; After this, all cores are equal. Development ends. Life begins.
;
; Most hardware is already working (BIOS configured it).
; Genesis only expresses genes for devices that need init.
; Everything else is non-local — just read/write the address.

bits 64

PIPE equ -1

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

    dw -1, 0, 0,+1
    dq 0xE9, 'g', 0
    dw -1, 0, 0,+1
    dq 0xE9, 'e', 0
    dw -1, 0, 0,+1
    dq 0xE9, 'n', 0
    dw -1, 0, 0,+1
    dq 0xE9, 'e', 0
    dw -1, 0, 0,+1
    dq 0xE9, 's', 0
    dw -1, 0, 0,+1
    dq 0xE9, 'i', 0
    dw -1, 0, 0,+1
    dq 0xE9, 's', 0
    dw -1, 0, 0,+1
    dq 0xE9, 10, 0

; ─── 2. clear pixel buffer (MBR did VESA, FB address at 0x9100) ──

    dw +7, 0, 0, 0                             ; rdd: FB address → pipeline
    dq 0x9100, 0, 0
    dw -1, 0,-3, 0                             ; filld: [pipeline], blue, 1024*768
    dq 0, 0x00336699, (1024*768)

; ─── 3. speaker beep (gene — PIT needs config) ──────────────

    dw  0,+1, 0, 0                             ; apply: walk_sub(speaker)
    dq walk_sub, walk_speaker_beep, (walk_speaker_beep_end - walk_speaker_beep)

; ─── 4. init keyboard trace ─────────────────────────────────

    ; trace header: set buffer address, depth, mask, record size
    dw -1, 0, 0, 0                             ; write: buf_addr
    dq (trace_kbd + TR_BUFADDR), trace_kbd_buf, 0
    dw -1, 0, 0, 0                             ; write: depth
    dq (trace_kbd + TR_DEPTH), TRACE_DEPTH, 0
    dw -1, 0, 0, 0                             ; write: mask
    dq (trace_kbd + TR_MASK), TRACE_MASK, 0
    dw -1, 0, 0, 0                             ; write: record size
    dq (trace_kbd + TR_RECSZ), 8, 0
    ; cursors start at 0 (BSS zeroed by bootloader or hardware)

    dw -1, 0, 0,+1                             ; debugcon: 'T' (trace ready)
    dq 0xE9, 'T', 0

; ─── 5. return — development ends, life begins ───────────────
;
; After this, lattice_start enters the keyboard poll loop.
; The BSP becomes the keyboard handler — not by assignment,
; by being the first core to find work.

main_len: dd (main_len - main_walk)


; ═══════════════════════════════════════════════════════════════════
; KEYBOARD POLL — walk DNA for the keyboard trace
;
; This walk runs in a loop_back. It checks port 0x64 for a key,
; reads port 0x60 if ready, writes the scancode to debugcon.
; The walk IS the keyboard handler. No asm. Just bonds.
; ═══════════════════════════════════════════════════════════════════

section .rodata

global kbd_walk, kbd_walk_len

kbd_walk:
    ; read PS/2 status (non-local, port 0x64)
    dw +1, 0, 0,-1                             ; port_read(0x64) → pipeline
    dq 0x64, 0, 0

    ; test bit 0 (data ready?)
    dw  0, 0, 0,+3                             ; AND(pipeline, 1)
    dq  0, 1, 0

    ; no key? skip to end, keep looping
    dw 0x7FFE, 0, 0, 0                         ; skip_z 3: no key → skip to loop_back
    dq 3, 0, 0

    ; key ready — read scancode (non-local, port 0x60)
    dw +1, 0, 0,-1                             ; port_read(0x60) → pipeline
    dq 0x60, 0, 0

    ; write scancode to debugcon (proof of life)
    dw -1, 0, 0,+1                             ; port_write(0xE9, pipeline)
    dq 0xE9, PIPE, 0

    ; always non-zero (scancode is never 0) — but force it
    dw -1, 0, 0,-1                             ; add(1, 0) → pipeline = 1
    dq 1, 0, 0

    ; loop back to port_read(0x64)
    dw 0x7FFC, 0, 0, 0                         ; loop_back 6
    dq 6, 0, 0

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
; SCAFFOLDING — walk_sub bridges walker to sub-walks
; ═══════════════════════════════════════════════════════════════════

section .text

global walk_sub
walk_sub:
    mov rdi, rsi
    mov esi, edx
    extern walker
    call walker
    ret
