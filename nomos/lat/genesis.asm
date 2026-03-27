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

; stash for walk temp storage (single-core; per-trace scratch later)
global stash_a, stash_b
stash_a: resq 1
stash_b: resq 1

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

    db 0xFE                                 ; skip_z: no VESA → skip filld
    dd 11                                   ; filld is 11 bytes

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
; KEYBOARD WALK — input + output through trace
;
; Input:  poll PS/2, write scancode to keyboard trace
; Output: read from keyboard trace, write to debugcon
; Loops forever. Proves trace read/write primitive.
;
; Wave bytes used:
;   0x43 = port_read (+1,0,0,-1)    0xC1 = port_write (-1,0,0,+1)
;   0xC0 = write (-1,0,0,0)         0x40 = read (+1,0,0,0)
;   0x02 ext Q=3 = AND              0x02 ext Q=7 = SHL
;   0xC3 = add (-1,0,0,-1)          0x0C = increment (0,0,-1,0)
;   0xFE = skip_z                   0xFD = skip_nz
;   0xFC = loop_back
;
; Flags bit 6 = indirect arg1 (dereference pointer)
;
; Offsets:
;    0: port_read(0x64)            3    ─┐
;    3: AND(pipe, 1)               4     │ input section
;    7: skip_z(33)                 5     │ skip to output if no key
;   12: port_read(0x60)            3     │
;   15: write(stash_a, pipe)       6     │ save scancode
;   21: read(wcursor)              6     │ trace write
;   27: AND(pipe, mask)            4     │
;   31: SHL(pipe, 3)               4     │
;   35: add(kbd_buf, pipe)         6     │ = slot address
;   41: write(pipe, *stash_a)      6     │ write scancode to slot
;   47: read(wcursor)              6     │ advance write cursor
;   53: increment(pipe)            2     │
;   55: write(wcursor, pipe)       6    ─┘
;   61: read(rcursor)              6    ─┐
;   67: write(stash_a, pipe)       6     │ output section
;   73: read(wcursor)              6     │ trace read
;   79: test(pipe, *stash_a)       6     │
;   85: skip_nz(26)                5     │ skip output if empty
;   90: read(stash_a)              6     │
;   96: AND(pipe, mask)            4     │
;  100: SHL(pipe, 3)               4     │
;  104: add(kbd_buf, pipe)         6     │ = slot address
;  110: read(pipe)                 2     │ read data
;  112: port_write(0xE9, pipe)     3     │ output to debugcon
;  115: read(rcursor)              6     │ advance read cursor
;  121: increment(pipe)            2     │
;  123: write(rcursor, pipe)       6    ─┘
;  129: add(1, 0)                  4     force non-zero
;  133: loop_back(133)             5     rewind to start
;  138: end
; ═══════════════════════════════════════════════════════════════════

section .rodata

global kbd_walk, kbd_walk_len

kbd_walk:

; ─── INPUT: poll keyboard, write to trace ───────────────────

    ; check PS/2 status
    db 0x43, 0x01, 0x64                    ;  0: port_read(0x64)
    db 0x02, 3, 0x04, 1                    ;  3: AND(pipe, 1)
    db 0xFE                                ;  7: skip_z — no key? skip to output
    dd 49                                  ;     skip 49 bytes (12..60 = input section)

    ; key ready — read scancode and write to trace
    db 0x43, 0x01, 0x60                    ; 12: port_read(0x60) → scancode
    db 0xC0, 0x02                          ; 15: write(stash_a, pipe) — save scancode
    dd stash_a

    ; compute write slot: wcursor & mask * 8 + buf
    db 0x40, 0x02                          ; 21: read(trace_kbd + WCURSOR)
    dd (trace_kbd + TR_WCURSOR)
    db 0x02, 3, 0x04, TRACE_MASK          ; 27: AND(pipe, 63)
    db 0x02, 7, 0x04, 3                   ; 31: SHL(pipe, 3)
    db 0xC3, 0x02                          ; 35: add(trace_kbd_buf, pipe)
    dd trace_kbd_buf

    ; write scancode to slot (indirect: arg1 = *stash_a)
    db 0xC0, 0x48                          ; 41: write(pipe, *stash_a)
    dd stash_a

    ; advance write cursor
    db 0x40, 0x02                          ; 47: read(trace_kbd + WCURSOR)
    dd (trace_kbd + TR_WCURSOR)
    db 0x0C, 0x00                          ; 53: increment(pipe)
    db 0xC0, 0x02                          ; 55: write(trace_kbd + WCURSOR, pipe)
    dd (trace_kbd + TR_WCURSOR)

; ─── OUTPUT: read from trace, write to debugcon ─────────────

    ; check if trace has data: compare read_cursor vs write_cursor
    db 0x40, 0x02                          ; 61: read(trace_kbd + RCURSOR)
    dd (trace_kbd + TR_RCURSOR)
    db 0xC0, 0x02                          ; 67: write(stash_a, pipe) — save rcursor
    dd stash_a
    db 0x40, 0x02                          ; 73: read(trace_kbd + WCURSOR)
    dd (trace_kbd + TR_WCURSOR)
    db 0x01, 0x48                          ; 79: test(pipe, *stash_a) — 1 if empty
    dd stash_a

    ; empty? skip output (39 bytes to add)
    db 0xFD                                ; 85: skip_nz — empty? skip output
    dd 39

    ; not empty — compute read slot
    db 0x40, 0x02                          ; 90: read(stash_a) — restore rcursor
    dd stash_a
    db 0x02, 3, 0x04, TRACE_MASK          ; 96: AND(pipe, 63)
    db 0x02, 7, 0x04, 3                   ;100: SHL(pipe, 3)
    db 0xC3, 0x02                          ;104: add(trace_kbd_buf, pipe)
    dd trace_kbd_buf

    ; read data from slot
    db 0x40, 0x00                          ;110: read(pipe) — deref slot addr

    ; output to debugcon
    db 0xC1, 0x01, 0xE9                    ;112: port_write(0xE9, pipe)

    ; advance read cursor
    db 0x40, 0x02                          ;115: read(trace_kbd + RCURSOR)
    dd (trace_kbd + TR_RCURSOR)
    db 0x0C, 0x00                          ;121: increment(pipe)
    db 0xC0, 0x02                          ;123: write(trace_kbd + RCURSOR, pipe)
    dd (trace_kbd + TR_RCURSOR)

; ─── LOOP ───────────────────────────────────────────────────

    db 0xC3, 0x05, 1, 0                    ;129: add(1, 0) → pipe = 1
    db 0xFC                                ;133: loop_back
    dd 133                                 ;     rewind 133 bytes to start

kbd_walk_end:
kbd_walk_len: dd (kbd_walk_end - kbd_walk)


; ═══════════════════════════════════════════════════════════════════
; GENES — only for hardware that needs configuration
; ═══════════════════════════════════════════════════════════════════

section .rodata

%include "lat/hw/speaker.asm"

; ─── FONT DATA ─────────────────────────────────────────────────
align 16
global glyph_sdf
glyph_sdf:
    incbin "lat/glyph.ha"


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
