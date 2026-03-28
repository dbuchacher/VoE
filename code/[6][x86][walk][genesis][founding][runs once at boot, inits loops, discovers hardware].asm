; genesis.asm — the founding
;
; Runs once. Expresses hardware genes. Fills loops. Returns.
; After this, all cores are equal. Development ends. Life begins.
;
; Think of this as embryonic development. The egg divides, organs form,
; then the organism is born and genesis never runs again.
; If you're adding features to genesis, you're building main.asm. Don't.

bits 64

%include "[0][tool][walk.inc][NASM macros to write walks as coordinates instead of hex].inc"
%include "walks/loop.asm"

; ─── keyboard loop constants ──────────────────────────────────
LOOP_DEPTH equ 64      ; 64 slots in the keyboard loop
LOOP_MASK  equ (LOOP_DEPTH - 1)  ; = 63 = 0x3F

; ─── BSS (zeroed by bootloader, don't need to init to 0) ────

section .bss

; cursor_x/cursor_y FIRST in BSS, page-aligned, with full page guard.
; Something corrupts cursor_y during KVM MMIO FB writes. Root cause unknown.
; Isolating them at a page boundary with 4KB padding makes corruption
; require a page-crossing write — which should be impossible from any
; correctly-addressed operation.
alignb 4096
global cursor_x, cursor_y
cursor_x: resd 1
cursor_y: resd 1
          resb 4096 - 8   ; full page guard (nothing else on this page)

alignb 64                ; cache-line align the loop header
global loop_kbd
loop_kbd:      resb 64
loop_kbd_buf:  resb LOOP_DEPTH * 8

global stash_a, stash_b
stash_a: resq 1
stash_b: resq 1

MAX_LOOPS equ 16
global loop_list, loop_count
loop_list: resq MAX_LOOPS
loop_count: resq 1

global nvme_bar
nvme_bar: resq 1

global render_pending
render_pending: resq 1

global mouse_pending
mouse_pending: resq 1

global mouse_x, mouse_y, mouse_pkt, mouse_state, mouse_buttons
mouse_x:       resd 1
mouse_y:       resd 1
mouse_pkt:     resb 4
mouse_state:   resd 1
mouse_buttons: resd 1
global cursor_save, cursor_drawn
cursor_save:   resd 16*16
cursor_drawn:  resd 1

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

; 4. init keyboard loop — fill in the loop header fields
;    The loop header is 64 bytes in BSS (already zeroed = cursors start at 0).
;    We write: buffer address, depth, mask, and record size.
;    After this, the loop is ready for read/write operations.
    w -1,0,0,0                              ; write: loop_kbd.buf_addr = loop_kbd_buf
    db F(U32,U32,P)
    dd (loop_kbd + LP_BUFADDR)
    dd loop_kbd_buf

    w -1,0,0,0                              ; write: loop_kbd.depth = 64
    db F(U32,U8,P)
    dd (loop_kbd + LP_DEPTH)
    db LOOP_DEPTH

    w -1,0,0,0                              ; write: loop_kbd.mask = 63
    db F(U32,U8,P)
    dd (loop_kbd + LP_MASK)
    db LOOP_MASK

    w -1,0,0,0                              ; write: loop_kbd.record_size = 8 bytes
    db F(U32,U8,P)
    dd (loop_kbd + LP_RECSZ)
    db 8

; 5. register loop — add keyboard loop to the global loop list
;    loop_list is a fixed array of loop pointers. loop_count says how many.
;    The bind drain (not built yet) will scan this list to find work.
    w -1,0,0,0                              ; write: loop_list[0] = &loop_kbd
    db F(U32,U32,P)
    dd loop_list
    dd loop_kbd

    w -1,0,0,0                              ; write: loop_count = 1
    db F(U32,U8,P)
    dd loop_count
    db 1

; 5b. loop self-test — write 'L' to loop, read back, echo to debugcon
;     If the loop primitive works: prints 'L'. If broken: silence or garbage.
;     Uses the keyboard loop we just initialized above.
    w -1,0,0,-1                             ; add(0x4C, 0) → pipeline = 'L' (0x4C)
    db F(U8,U8,P), 0x4C, 0
    loop_write loop_kbd                     ; write 'L' to keyboard loop

    loop_empty loop_kbd                     ; pipeline = 1 (empty) or 0 (has data)
    skip_nz (.test_end - .test_read)        ; if empty after write, something broke — skip
.test_read:
    loop_read loop_kbd                      ; read from loop → pipeline (should be 'L')
    w -1,0,0,+1                             ; port_write(0xE9, pipeline) — echo to debugcon
    db F(U8,P,P), 0xE9
.test_end:

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

; 6b. mouse gene — enable PS/2 AUX port, init mouse, center cursor
;     apply(mouse_init, 0): inline x86 handles the port I/O sequence.
;     prints 'M' on success, 'm' on failure.
    w 0,+1,0,0
    db F(U32,U8,P)
    dd mouse_init
    db 0

; 7. render "nomos" — wave byte coordinates
;    wave byte: [7:6]=T [5:4]=D [3:2]=M [1:0]=Q  (00=0, 01=+1, 11=-1)
;    n: (1,0,-1,+1) = 0x4D — square, center dot, filled
;    o: (1,-1,+1,-1) = 0x77 — circle, right dot, outline
;    m: (1,+1,-1,+1) = 0x5D — square, left dot, filled
;    s: (1,0,0,-1) = 0x43 — diamond, center dot, outline
    w 0,+1,0,0
    db F(U32,U8,P)
    dd render_char
    db 0x4D                                     ; 'n'
    w 0,+1,0,0
    db F(U32,U8,P)
    dd render_char
    db 0x77                                     ; 'o'
    w 0,+1,0,0
    db F(U32,U8,P)
    dd render_char
    db 0x5D                                     ; 'm'
    w 0,+1,0,0
    db F(U32,U8,P)
    dd render_char
    db 0x77                                     ; 'o'
    w 0,+1,0,0
    db F(U32,U8,P)
    dd render_char
    db 0x43                                     ; 's'

; 8. done — signal genesis complete
    w -1,0,0,+1                             ; port_write(0xE9, 'T') — 'T' for "loop ready"
    db F(U8,U8,P), 0xE9, 'T'

main_len: dd (main_len - main_walk)

; ─── includes ────────────────────────────────────────────────

section .rodata
%include "walks/speaker.asm"

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

; ─── render_char — Latin bitmap + SDF fallback ─────────────────
;
; Called via apply: rsi = wave byte
;   bits [7:6]=T  [5:4]=D  [3:2]=M  [1:0]=Q
;   00=0  01=+1  11=-1
;
; Special wave bytes (non-letter, T ≠ +1):
;   0x00 = silence (skip)
;   0x01 = space  +W (advance cursor)
;   0x04 = enter  +R (newline)
;   0xC0 = backspace -P (erase backward)
;
; Latin path: reads render_pending (raw scancode from kbd walk),
;   indexes font_by_scancode[scancode × 16] directly. If first
;   byte != 0, renders 8×16 bitmap font scaled 2× (each bitmap
;   pixel = 2×2 screen pixels → fills the 16×32 cell). Falls
;   through to SDF if unmapped (16 zero bytes).
;
; SDF path (fallback for non-Latin wave bytes):
;   M selects body shape:   +1=circle (vowel)  0=diamond (fricative)  -1=square (stop)
;   D positions marker dot: +1→left(4)  0→center(8)  -1→right(12)
;   Q controls fill mode:   +1=filled  -1=outline
;   16×32 cell. center=(8,16), radius=6.

global render_char
render_char:
    push rbx
    push r12
    push r13
    push r14
    push r15
    push rsi

    mov ebx, esi                              ; wave byte in ebx

    ; ── special key handling ──
    test ebx, ebx
    jz .done                                  ; 0x00 = silence → return (walk filters these)
    cmp bl, 0x04
    je .newline                               ; +R = enter
    cmp bl, 0xC0
    je .backspace                             ; -P = backspace
    cmp bl, 0x01
    je .done_advance                          ; +W = space

    mov r12d, [0x9100]
    test r12d, r12d
    jz .done

    ; framebuffer position (with bounds clamping — root cause of
    ; cursor_y corruption still unknown, PIC masking didn't fix it)
    mov r13d, [cursor_y]
    cmp r13d, 768
    jb .cy_ok                                 ; unsigned: catches negative corruption too
    xor r13d, r13d
    mov [cursor_y], r13d
.cy_ok:
    imul r13d, r13d, 1024
    mov eax, [cursor_x]
    cmp eax, 1024
    jb .cx_ok                                 ; unsigned: catches negative corruption too
    xor eax, eax
    mov [cursor_x], eax
.cx_ok:
    add r13d, eax
    shl r13d, 2
    add r13, r12

    ; ── Latin bitmap path: scancode → glyph (direct, no ASCII) ──
    ; render_pending holds the raw PS/2 scancode saved by kbd walk.
    ; font_by_scancode: 128 entries × 16 bytes, indexed by scancode.
    ; Unmapped scancodes have 16 zero bytes. Mapped glyphs may have
    ; zero top rows (lowercase letters start mid-cell). Check bytes 4-11
    ; where the glyph body lives — any nonzero means glyph exists.
    mov eax, [render_pending]
    and eax, 0x7F                             ; mask to 0-127 (safety)
    shl eax, 4                                ; × 16 bytes per glyph
    lea r14, [font_by_scancode]
    add r14, rax                              ; r14 = glyph data pointer
    lea r14, [font_by_scancode]
    add r14, rax                              ; r14 = glyph data pointer
    mov eax, [r14+4]                          ; bytes 4-7
    or eax, [r14+8]                           ; OR with bytes 8-11
    test eax, eax                             ; any nonzero = has glyph
    jz .sdf_path

    mov r15d, 16                              ; 16 bitmap rows
.bm_row:
    movzx eax, byte [r14]
    inc r14

    mov ecx, 8
.bm_bit1:
    dec ecx
    bt eax, ecx
    jnc .bm_skip1
    mov edx, 7
    sub edx, ecx
    shl edx, 3
    mov dword [r13 + rdx], 0xFFFFFFFF
    mov dword [r13 + rdx + 4], 0xFFFFFFFF
    mov dword [r13 + rdx + 4096], 0xFFFFFFFF
    mov dword [r13 + rdx + 4100], 0xFFFFFFFF
.bm_skip1:
    test ecx, ecx
    jnz .bm_bit1

    add r13, 1024 * 4 * 2
    dec r15d
    jnz .bm_row

    jmp .done_advance

.sdf_path:
    ; ── computed SDF: shape from wave byte coordinates ──
    ; ebx = wave byte: [7:6]=T [5:4]=D [3:2]=M [1:0]=Q
    ; 2-bit decode: shl 30 + sar 30 maps 00→0, 01→+1, 11→-1
    ; M selects body: +1=circle  0=diamond  -1=square
    ; D positions marker dot. Q: +1=filled  -1=outline
    ; center=(8,16), radius=6

    mov ecx, 32                               ; row counter (32..1)
.row:
    mov edx, 16                               ; col counter (16..1)
.col:
    dec edx

    ; dx = |col - 8|
    mov eax, edx
    sub eax, 8
    mov esi, eax
    sar esi, 31
    xor eax, esi
    sub eax, esi
    mov esi, eax                              ; esi = dx

    ; dy = |row_from_top - 16|
    mov eax, 32
    sub eax, ecx
    sub eax, 16
    mov edi, eax
    sar edi, 31
    xor eax, edi
    sub eax, edi
    mov edi, eax                              ; edi = dy

    ; order: esi=dmax, edi=dmin
    cmp esi, edi
    jge .ordered
    xchg esi, edi
.ordered:

    ; ── body distance from M ──
    ; M = bits [3:2] of wave byte
    mov eax, ebx
    shr eax, 2
    and eax, 3
    shl eax, 30
    sar eax, 30                               ; eax = M: -1, 0, or +1

    mov r8d, esi                              ; start with dmax
    cmp eax, 0
    jg .circle
    jl .square
    add r8d, edi                              ; diamond: dmax + dmin
    jmp .dist_ok
.circle:
    mov eax, edi
    shr eax, 1
    add r8d, eax                              ; circle: dmax + dmin/2
    jmp .dist_ok
.square:
.dist_ok:
    sub r8d, 6                                ; - radius

    ; ── marker dot: D = bits [5:4] ──
    ; marker_cx = 8 - D*4
    mov eax, ebx
    shr eax, 4
    and eax, 3
    shl eax, 30
    sar eax, 30                               ; eax = D
    shl eax, 2                                ; D*4
    mov esi, 8
    sub esi, eax                              ; marker_cx

    ; |col - marker_cx|
    mov eax, edx
    sub eax, esi
    mov esi, eax
    sar esi, 31
    xor eax, esi
    sub eax, esi
    mov esi, eax

    ; |row_from_top - 4|
    mov eax, 32
    sub eax, ecx
    sub eax, 4
    mov edi, eax
    sar edi, 31
    xor eax, edi
    sub eax, edi

    ; marker_dist = max(|dx|, |dy|) - 2
    cmp esi, eax
    jge .mmax
    mov esi, eax
.mmax:
    sub esi, 2

    ; union: dist = min(body, marker)
    cmp r8d, esi
    jle .union_ok
    mov r8d, esi
.union_ok:

    ; ── Q = bits [1:0]: voiceless = outline ──
    mov eax, ebx
    and eax, 3
    shl eax, 30
    sar eax, 30                               ; eax = Q
    cmp eax, 0
    jge .no_outline
    mov eax, r8d
    mov esi, eax
    sar esi, 31
    xor eax, esi
    sub eax, esi                              ; |dist|
    dec eax
    mov r8d, eax
.no_outline:

    ; ── pixel output ──
    cmp r8d, 1
    jge .skip                                 ; outside
    cmp r8d, -1
    jle .solid                                ; fully inside
    mov esi, 0x99AABB                         ; edge pixel
    jmp .write
.solid:
    mov esi, 0xFFFFFFFF                       ; white
.write:
    mov eax, edx
    shl eax, 2
    mov [r13 + rax], esi

.skip:
    test edx, edx
    jnz .col

    add r13, 1024 * 4
    dec ecx
    jnz .row

.done_advance:
    mov eax, [cursor_x]
    add eax, 16
    cmp eax, 1024
    jl .store_x
    xor eax, eax
    mov ecx, [cursor_y]
    add ecx, 32
    cmp ecx, 768
    jl .store_y
    xor ecx, ecx
.store_y:
    mov [cursor_y], ecx
.store_x:
    mov [cursor_x], eax

.done:
    pop rax
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

.newline:
    mov dword [cursor_x], 0
    mov eax, [cursor_y]
    add eax, 32
    cmp eax, 768
    jl .nl_store
    xor eax, eax
.nl_store:
    mov [cursor_y], eax
    jmp .done

.backspace:
    mov eax, [cursor_x]
    sub eax, 16
    jns .bs_store
    xor eax, eax
.bs_store:
    mov [cursor_x], eax

    ; clear the cell at the new cursor position (fill with blue background)
    mov r12d, [0x9100]
    test r12d, r12d
    jz .done
    mov r13d, [cursor_y]
    cmp r13d, 768
    jl .bs_cy_ok
    xor r13d, r13d
.bs_cy_ok:
    imul r13d, r13d, 1024
    mov eax, [cursor_x]
    cmp eax, 1024
    jl .bs_cx_ok
    xor eax, eax
.bs_cx_ok:
    add r13d, eax
    shl r13d, 2
    add r13, r12
    mov ecx, 32                               ; 32 scanlines
.bs_clear_row:
    mov edx, 16                               ; 16 pixels wide
.bs_clear_col:
    dec edx
    mov eax, edx
    shl eax, 2
    mov dword [r13 + rax], 0x00336699         ; blue background
    test edx, edx
    jnz .bs_clear_col
    add r13, 1024 * 4
    dec ecx
    jnz .bs_clear_row
    jmp .done

; ─── mouse_init — PS/2 AUX port initialization ─────────────────
;
; Called via apply: rsi = ignored (0).
; Enables the PS/2 auxiliary (mouse) port, sends reset + enable.
; Initializes mouse_x/mouse_y to screen center.
; Prints 'M' on success, 'm' on failure.

global mouse_init
mouse_init:
    push rbx
    push r12

    mov dword [mouse_x], 512
    mov dword [mouse_y], 384
    mov dword [mouse_state], 0
    mov dword [mouse_buttons], 0

    ; enable AUX port
    call .wait_ibf
    mov al, 0xA8
    out 0x64, al

    ; read controller config
    call .wait_ibf
    mov al, 0x20
    out 0x64, al
    call .wait_obf
    in al, 0x60
    mov bl, al

    ; modify config: set bit 1 (IRQ12 enable), clear bit 5 (AUX clock)
    or bl, 0x02
    and bl, ~0x20

    ; write config back
    call .wait_ibf
    mov al, 0x60
    out 0x64, al
    call .wait_ibf
    mov al, bl
    out 0x60, al

    ; reset mouse (0xFF)
    mov al, 0xFF
    call .send_mouse_cmd
    jc .mi_fail
    call .read_mouse_byte       ; self-test (0xAA)
    jc .mi_fail
    cmp al, 0xAA
    jne .mi_fail
    call .read_mouse_byte       ; device ID (0x00)
    jc .mi_fail

    ; set defaults (0xF6)
    mov al, 0xF6
    call .send_mouse_cmd
    jc .mi_fail

    ; enable reporting (0xF4)
    mov al, 0xF4
    call .send_mouse_cmd
    jc .mi_fail

    mov dx, 0xE9
    mov al, 'M'
    out dx, al
    jmp .mi_done

.mi_fail:
    mov dx, 0xE9
    mov al, 'm'
    out dx, al

.mi_done:
    pop r12
    pop rbx
    ret

.wait_ibf:
    mov r12d, 0x10000
.ibf_spin:
    dec r12d
    jz .ibf_out
    in al, 0x64
    test al, 0x02
    jnz .ibf_spin
.ibf_out:
    ret

.wait_obf:
    mov r12d, 0x10000
.obf_spin:
    dec r12d
    jz .obf_out
    in al, 0x64
    test al, 0x01
    jz .obf_spin
.obf_out:
    ret

.send_mouse_cmd:
    push rax
    call .wait_ibf
    mov al, 0xD4
    out 0x64, al
    call .wait_ibf
    pop rax
    out 0x60, al
    call .read_mouse_byte
    jc .smc_fail
    cmp al, 0xFA
    jne .smc_fail
    clc
    ret
.smc_fail:
    stc
    ret

.read_mouse_byte:
    mov r12d, 0x100000
.rmb_spin:
    dec r12d
    jz .rmb_timeout
    in al, 0x64
    test al, 0x01
    jz .rmb_spin
    test al, 0x20
    jz .rmb_discard
    in al, 0x60
    clc
    ret
.rmb_discard:
    in al, 0x60
    jmp .rmb_spin
.rmb_timeout:
    stc
    ret


; ─── externs ─────────────────────────────────────────────────

extern walker_wave
extern kbd_walk, kbd_walk_len
extern font_by_scancode
