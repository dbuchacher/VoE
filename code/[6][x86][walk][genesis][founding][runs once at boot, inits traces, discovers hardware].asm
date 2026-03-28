; genesis.asm — the founding
;
; Runs once. Expresses hardware genes. Fills traces. Returns.
; After this, all cores are equal. Development ends. Life begins.
;
; Think of this as embryonic development. The egg divides, organs form,
; then the organism is born and genesis never runs again.
; If you're adding features to genesis, you're building main.asm. Don't.

bits 64

%include "[0][tool][walk.inc][NASM macros to write walks as coordinates instead of hex].inc"

; ─── trace layout constants ──────────────────────────────────
; A trace is a 64-byte header (cache-line aligned) describing a ring buffer.
; These offsets are byte positions within that header.
TR_WCURSOR equ 0        ; write cursor: where the next write goes
TR_RCURSOR equ 8        ; read cursor: where the next read comes from
TR_BUFADDR equ 16       ; pointer to the actual ring buffer memory
TR_DEPTH   equ 24       ; how many slots in the ring (power of 2)
TR_MASK    equ 32       ; depth - 1, used to wrap cursors (cursor AND mask = slot index)
TR_RECSZ   equ 40       ; bytes per record in this trace

TRACE_DEPTH equ 64      ; 64 slots in the keyboard trace
TRACE_MASK  equ (TRACE_DEPTH - 1)  ; = 63 = 0x3F

; ─── BSS (zeroed by bootloader, don't need to init to 0) ────

section .bss

alignb 64                ; cache-line align the trace header
trace_kbd:      resb 64              ; keyboard trace header (64 bytes)
trace_kbd_buf:  resb TRACE_DEPTH * 8 ; keyboard ring buffer (64 slots × 8 bytes)

global stash_a, stash_b
stash_a: resq 1          ; temp storage for walks (save/restore pipeline values)
stash_b: resq 1

MAX_TRACES equ 16
global trace_list, trace_count
trace_list: resq MAX_TRACES  ; array of pointers to trace headers
trace_count: resq 1          ; how many traces exist

global cursor_x, cursor_y
cursor_x: resd 1         ; text cursor position for bitmap renderer (pixels)
cursor_y: resd 1

global nvme_bar
nvme_bar: resq 1         ; NVMe controller BAR0 address (0 if not found)

; ─── genesis walk ────────────────────────────────────────────
; This is the founding walk. It runs once and returns.
; Each section initializes one piece of hardware or data structure.
; The walker executes these bonds top to bottom, then genesis is done.

section .rodata

global main_walk, main_len

main_walk:

; 1. proof of life — write "genesis\n" to debugcon (port 0xE9)
;    port 0xE9 is QEMU's debug console. writing a byte prints it to the host terminal.
;    this is the first sign that the walker is alive and executing bonds.
    w -1,0,0,+1                             ; port_write(0xE9, 'g')
    db F(U8,U8,P), 0xE9, 'g'
    w -1,0,0,+1
    db F(U8,U8,P), 0xE9, 'e'
    w -1,0,0,+1
    db F(U8,U8,P), 0xE9, 'n'
    w -1,0,0,+1
    db F(U8,U8,P), 0xE9, 'e'
    w -1,0,0,+1
    db F(U8,U8,P), 0xE9, 's'
    w -1,0,0,+1
    db F(U8,U8,P), 0xE9, 'i'
    w -1,0,0,+1
    db F(U8,U8,P), 0xE9, 's'
    w -1,0,0,+1
    db F(U8,U8,P), 0xE9, 10                ;   10 = newline

; 2. clear pixel buffer — fill the screen with blue
;    The MBR set up VESA 1024×768×32 and stored the framebuffer address at 0x9100.
;    We read that address. If it's 0 (headless/multiboot), skip the fill.
;    filld writes a 32-bit color to every pixel. 0x00336699 = a nice blue.
    w +7,0,0,0                              ; read dword at address 0x9100 → pipeline
    db F(U32,P,P)                           ;   (T=+7 = extended read, dword width)
    dd 0x9100
    skip_z 11                               ; pipeline == 0 means no framebuffer, skip 11 bytes
    w -1,0,-3,0                             ; filld(pipeline, color, count)
    db F(P,U32,U32)                         ;   pipeline = FB addr, then color, then pixel count
    dd 0x00336699                           ;   blue
    dd (1024*768)                           ;   1024×768 = 786432 pixels

; 3. speaker beep — 440 Hz through the PC speaker
;    apply calls walk_sub, which bridges to walker_wave to run the speaker walk.
;    walk_sub is scaffolding — dissolves when the equation handles nested walks.
    w 0,+1,0,0                              ; apply(walk_sub, speaker_walk, length)
    db F(U32,U32,U32)
    dd walk_sub
    dd walk_speaker_beep
    dd (walk_speaker_beep_end - walk_speaker_beep)

; 4. init keyboard trace — fill in the trace header fields
;    The trace header is 64 bytes in BSS (already zeroed = cursors start at 0).
;    We write: buffer address, depth, mask, and record size.
;    After this, the trace is ready for read/write operations.
    w -1,0,0,0                              ; write: trace_kbd.buf_addr = trace_kbd_buf
    db F(U32,U32,P)
    dd (trace_kbd + TR_BUFADDR)
    dd trace_kbd_buf

    w -1,0,0,0                              ; write: trace_kbd.depth = 64
    db F(U32,U8,P)
    dd (trace_kbd + TR_DEPTH)
    db TRACE_DEPTH

    w -1,0,0,0                              ; write: trace_kbd.mask = 63
    db F(U32,U8,P)
    dd (trace_kbd + TR_MASK)
    db TRACE_MASK

    w -1,0,0,0                              ; write: trace_kbd.record_size = 8 bytes
    db F(U32,U8,P)
    dd (trace_kbd + TR_RECSZ)
    db 8

; 5. register trace — add keyboard trace to the global trace list
;    trace_list is a fixed array of trace pointers. trace_count says how many.
;    The bind drain (not built yet) will scan this list to find work.
    w -1,0,0,0                              ; write: trace_list[0] = &trace_kbd
    db F(U32,U32,P)
    dd trace_list
    dd trace_kbd

    w -1,0,0,0                              ; write: trace_count = 1
    db F(U32,U8,P)
    dd trace_count
    db 1

; 6. NVMe discovery — probe PCI slots 3-6 for an NVMe controller
;    If found, stores BAR0 in nvme_bar. Prints 'N' (found) or 'n' (not found).
%include "walks/pci.asm"
    pci_probe_dev 3
    pci_probe_dev 4
    pci_probe_dev 5
    pci_probe_dev 6

    w +1,0,0,0                              ; read nvme_bar → pipeline
    db F(U32,P,P)
    dd nvme_bar
    skip_z 4                                ; nvme_bar == 0 → skip 'N' (4 bytes)
    w -1,0,0,+1
    db F(U8,U8,P), 0xE9, 'N'               ;   NVMe found
    skip_nz 4                               ; nvme_bar != 0 → skip 'n' (4 bytes)
    w -1,0,0,+1
    db F(U8,U8,P), 0xE9, 'n'               ;   NVMe not found

; 7. render "nomos" — draw text on the framebuffer using the bitmap renderer
;    apply calls render_char (asm scaffolding) with each character code.
;    render_char reads the glyph bitmap, plots pixels, advances the cursor.
;    This is scaffolding — dissolves when SDF rendering replaces it.
    w 0,+1,0,0                              ; apply(render_char, 'n')
    db F(U32,U8,P)
    dd render_char
    db 'n'
    w 0,+1,0,0
    db F(U32,U8,P)
    dd render_char
    db 'o'
    w 0,+1,0,0
    db F(U32,U8,P)
    dd render_char
    db 'm'
    w 0,+1,0,0
    db F(U32,U8,P)
    dd render_char
    db 'o'
    w 0,+1,0,0
    db F(U32,U8,P)
    dd render_char
    db 's'

; 8. done — signal genesis complete
    w -1,0,0,+1                             ; port_write(0xE9, 'T') — 'T' for "trace ready"
    db F(U8,U8,P), 0xE9, 'T'

main_len: dd (main_len - main_walk)

; ─── includes ────────────────────────────────────────────────

section .rodata
%include "walks/speaker.asm"

align 16
global glyph_sdf
glyph_sdf:
    incbin "[data][glyph][8x16 bitmap font, scaffolding, dissolves with SDF].ha"

; ─── scaffolding: walk_sub — bridges apply bond to walker_wave ──
;
; Called via apply: rsi = walk data, rdx = walk length
; Dissolves when the equation can handle nested walks natively.

section .text

global walk_sub
walk_sub:
    mov rdi, rsi
    mov esi, edx
    call walker_wave
    ret

; ─── scaffolding: render_char — bitmap glyph renderer ──────────
;
; Called via apply: rsi = char code
; Reads cursor_x, cursor_y, glyph_sdf, FB addr from known addresses.
; Renders 8×16 bitmap glyph in white on current background.
; Advances cursor. Handles newline (0x0A) and wrapping.
;
; Scaffolding — dissolves when SDF rendering (math walks) replaces it.

global render_char
render_char:
    push rbx
    push r12
    push r13
    push rsi                              ; save char for return

    movzx eax, sil

    cmp al, 0x0A
    je .newline
    cmp al, 0x08
    je .backspace
    cmp al, 0x20
    jb .done

    mov r12d, [0x9100]
    test r12d, r12d
    jz .done

    shl eax, 4
    lea rbx, [glyph_sdf + rax]

    mov r13d, [cursor_y]
    imul r13d, r13d, 1024
    add r13d, [cursor_x]
    shl r13d, 2
    add r13, r12

    mov ecx, 16
.row:
    movzx eax, byte [rbx]

    test al, 0x80
    jz .s7
    mov dword [r13], 0xFFFFFFFF
.s7:
    test al, 0x40
    jz .s6
    mov dword [r13+4], 0xFFFFFFFF
.s6:
    test al, 0x20
    jz .s5
    mov dword [r13+8], 0xFFFFFFFF
.s5:
    test al, 0x10
    jz .s4
    mov dword [r13+12], 0xFFFFFFFF
.s4:
    test al, 0x08
    jz .s3
    mov dword [r13+16], 0xFFFFFFFF
.s3:
    test al, 0x04
    jz .s2
    mov dword [r13+20], 0xFFFFFFFF
.s2:
    test al, 0x02
    jz .s1
    mov dword [r13+24], 0xFFFFFFFF
.s1:
    test al, 0x01
    jz .s0
    mov dword [r13+28], 0xFFFFFFFF
.s0:
    inc rbx
    add r13, 1024*4
    dec ecx
    jnz .row

    mov eax, [cursor_x]
    add eax, 8
    cmp eax, 1024
    jl .store_x
    xor eax, eax
    mov ecx, [cursor_y]
    add ecx, 16
    cmp ecx, 768
    jl .store_y
    xor ecx, ecx
.store_y:
    mov [cursor_y], ecx
.store_x:
    mov [cursor_x], eax

.done:
    pop rax
    pop r13
    pop r12
    pop rbx
    ret

.newline:
    mov dword [cursor_x], 0
    mov eax, [cursor_y]
    add eax, 16
    cmp eax, 768
    jl .nl_store
    xor eax, eax
.nl_store:
    mov [cursor_y], eax
    jmp .done

.backspace:
    mov eax, [cursor_x]
    sub eax, 8
    jns .bs_store
    xor eax, eax
.bs_store:
    mov [cursor_x], eax
    jmp .done

; ─── externs ─────────────────────────────────────────────────

extern walker_wave
extern kbd_walk, kbd_walk_len
