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

alignb 64                ; cache-line align the loop header
global loop_kbd
loop_kbd:      resb 64              ; keyboard loop header (64 bytes)
loop_kbd_buf:  resb LOOP_DEPTH * 8 ; keyboard ring buffer (64 slots × 8 bytes)

global stash_a, stash_b
stash_a: resq 1          ; temp storage for walks (save/restore pipeline values)
stash_b: resq 1

MAX_LOOPS equ 16
global loop_list, loop_count
loop_list: resq MAX_LOOPS  ; array of pointers to loop headers
loop_count: resq 1          ; how many loops exist

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

; 7. render "nomos" — computed SDF from phonetic coordinates
;    n: alveolar nasal voiced       = (T=1, D=0, M=-1, Q=1) → square + center dot
;    o: back vowel rounded          = (T=1, D=-1, M=1, Q=-1) → circle outline + right dot
;    m: labial nasal voiced         = (T=1, D=1, M=-1, Q=1) → square + left dot
;    s: alveolar fricative voiceless = (T=1, D=0, M=0, Q=-1) → diamond outline + center dot
%define PH(t,d,m,q) (((t) & 0xFF) | (((d) & 0xFF) << 8) | (((m) & 0xFF) << 16) | (((q) & 0xFF) << 24))
    w 0,+1,0,0
    db F(U32,U32,P)
    dd render_char
    dd PH(1, 0,-1, 1)                      ; 'n' — square, center dot, filled
    w 0,+1,0,0
    db F(U32,U32,P)
    dd render_char
    dd PH(1,-1, 1,-1)                      ; 'o' — circle, right dot, outline
    w 0,+1,0,0
    db F(U32,U32,P)
    dd render_char
    dd PH(1, 1,-1, 1)                      ; 'm' — square, left dot, filled
    w 0,+1,0,0
    db F(U32,U32,P)
    dd render_char
    dd PH(1,-1, 1,-1)                      ; 'o' — circle, right dot, outline
    w 0,+1,0,0
    db F(U32,U32,P)
    dd render_char
    dd PH(1, 0, 0,-1)                      ; 's' — diamond, center dot, outline

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

; ─── render_char — computed SDF from phonetic coordinates ──────
;
; Called via apply: rsi = packed coordinate dword
;   byte 0 = T, byte 1 = D, byte 2 = M, byte 3 = Q
; Computes shape from coordinates. No font data.
; 16×32 cell. Minimal version first — white rectangle with shape cutout.

global render_char
render_char:
    push rbx
    push r12
    push r13
    push rsi

    mov ebx, esi

    test ebx, ebx
    jz .done_advance
    cmp ebx, 0x0A000000
    je .newline
    cmp ebx, 0x08000000
    je .backspace

    mov r12d, [0x9100]
    test r12d, r12d
    jz .done

    ; framebuffer position
    mov r13d, [cursor_y]
    imul r13d, r13d, 1024
    add r13d, [cursor_x]
    shl r13d, 2
    add r13, r12

    ; ── computed SDF: shape from coordinates ──
    ; M (byte 2) selects body shape:
    ;   +1 = circle (vowel):   dist = max(dx,dy) + min(dx,dy)/2 - r
    ;    0 = diamond (fricative): dist = dx + dy - r
    ;   -1 = square (stop):    dist = max(dx,dy) - r
    ; D (byte 1) positions marker dot: +1→left(4), 0→center(8), -1→right(12)
    ; Q (byte 3): +1=filled, -1=outline
    ; center=(8,16), radius=6

    mov ecx, 32                           ; row counter (32..1)
.row:
    mov edx, 16                           ; col counter (16..1)
.col:
    dec edx

    ; dx = |col - 8|
    mov eax, edx
    sub eax, 8
    mov esi, eax
    sar esi, 31
    xor eax, esi
    sub eax, esi                          ; eax = |col - 8|
    mov esi, eax                          ; esi = dx

    ; dy = |row_from_top - 16|  (row counter counts down, row_from_top = 32 - ecx)
    mov eax, 32
    sub eax, ecx                          ; row_from_top
    sub eax, 16                           ; row - center_y
    mov edi, eax
    sar edi, 31
    xor eax, edi
    sub eax, edi                          ; eax = dy
    mov edi, eax

    ; order: esi=dmax, edi=dmin
    cmp esi, edi
    jge .ordered
    xchg esi, edi
.ordered:

    ; ── body distance from M ──
    mov eax, ebx
    shr eax, 16
    movsx eax, al                         ; M = openness

    mov r8d, esi                          ; start with dmax
    cmp eax, 0
    jg .circle
    jl .square
    add r8d, edi                          ; diamond: dmax + dmin
    jmp .dist_ok
.circle:
    mov eax, edi
    shr eax, 1
    add r8d, eax                          ; circle: dmax + dmin/2
    jmp .dist_ok
.square:
.dist_ok:
    sub r8d, 6                            ; - radius
    ; r8d = body distance (signed)

    ; ── marker dot: small square at top of cell ──
    ; D (byte 1): marker_cx = 8 - D*4
    mov eax, ebx
    shr eax, 8
    movsx eax, al                         ; D
    shl eax, 2                            ; D*4
    mov esi, 8
    sub esi, eax                          ; marker_cx

    ; |col - marker_cx|
    mov eax, edx                          ; col
    sub eax, esi
    mov esi, eax
    sar esi, 31
    xor eax, esi
    sub eax, esi                          ; |col - marker_cx|
    mov esi, eax

    ; |row_from_top - 4|
    mov eax, 32
    sub eax, ecx                          ; row_from_top
    sub eax, 4                            ; - marker_cy
    mov edi, eax
    sar edi, 31
    xor eax, edi
    sub eax, edi                          ; |row - 4|

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

    ; ── Q: voiceless = outline ──
    mov eax, ebx
    shr eax, 24
    movsx eax, al                         ; Q
    cmp eax, 0
    jge .no_outline
    mov eax, r8d
    mov esi, eax
    sar esi, 31
    xor eax, esi
    sub eax, esi                          ; |dist|
    dec eax
    mov r8d, eax
.no_outline:

    ; ── pixel output ──
    cmp r8d, 1
    jge .skip                             ; outside

    cmp r8d, -1
    jle .solid                            ; fully inside

    ; edge: blend (simplified — just use 50% gray for edge pixels)
    mov esi, 0x99AABB                     ; light blend color
    jmp .write

.solid:
    mov esi, 0xFFFFFFFF                   ; white
.write:
    mov eax, edx                          ; col
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
    jmp .done

; ─── externs ─────────────────────────────────────────────────

extern walker_wave
extern kbd_walk, kbd_walk_len
