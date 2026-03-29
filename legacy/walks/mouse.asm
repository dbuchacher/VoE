; mouse.asm — PS/2 mouse: walk state machine + x86 cursor scaffolding
;
; The state machine is a WALK — read, test, write, skip. Pure bonds.
; The cursor drawing is x86 scaffolding — called via apply. Dissolves
; when the walker can do per-pixel computation natively.
;
; Packet assembly (3 bytes):
;   state 0: incoming byte → check bit 3 (sync), store as flags, advance
;   state 1: store as X delta, advance
;   state 2: store as Y delta, process packet (apply mouse_update)
;
; kbd walk saves the mouse byte to mouse_pending, then calls:
;   apply(walk_sub, mouse_walk, mouse_walk_len)

bits 64

%include "[0][tool][walk.inc][NASM macros to write walks as coordinates instead of hex].inc"

extern mouse_x, mouse_y, mouse_pkt, mouse_state, mouse_buttons
extern mouse_pending
extern cursor_save, cursor_drawn, cursor_ox, cursor_oy
extern stash_a

; Mouse walk data is in kbd.asm (same file = compile-time length constant).
; This file has only the x86 scaffolding: position update + cursor drawing.

; ═════════════════════════════════════════════════════════════════
; MOUSE_UPDATE — x86 scaffolding for position math + cursor draw
;
; Called via apply when a complete 3-byte packet is assembled.
; Reads mouse_pkt, does sign extension + clamping, updates position,
; erases old cursor, draws new cursor.
; Dissolves when atoms can do the math and walks can draw pixels.
; ═════════════════════════════════════════════════════════════════

section .text

CURSOR_W equ 21
CURSOR_H equ 21

global mouse_update
mouse_update:
    push rbx
    push r12
    push r13
    push rsi

    ; erase old cursor
    call erase_cursor

    ; update X: sign extend byte 1 using sign bit 4 of byte 0
    movzx eax, byte [mouse_pkt+1]
    test byte [mouse_pkt], 0x10
    jz .xpos
    or eax, 0xFFFFFF00
.xpos:
    add eax, [mouse_x]
    jns .xnn
    xor eax, eax
.xnn:
    cmp eax, 1023
    jle .xok
    mov eax, 1023
.xok:
    mov [mouse_x], eax

    ; update Y: negate (PS/2 positive=up, screen positive=down)
    movzx eax, byte [mouse_pkt+2]
    test byte [mouse_pkt], 0x20
    jz .ypos
    or eax, 0xFFFFFF00
.ypos:
    neg eax
    add eax, [mouse_y]
    jns .ynn
    xor eax, eax
.ynn:
    cmp eax, 767
    jle .yok
    mov eax, 767
.yok:
    mov [mouse_y], eax

    ; update buttons
    movzx eax, byte [mouse_pkt]
    and eax, 7
    mov [mouse_buttons], eax

    ; draw new cursor
    call draw_cursor

    pop rax
    pop r13
    pop r12
    pop rbx
    ret


; ═════════════════════════════════════════════════════════════════
; CURSOR DRAWING — 21×21 triangle, white fill, black outline
; ═════════════════════════════════════════════════════════════════

global erase_cursor
erase_cursor:
    push rbx
    push r14
    push r15

    cmp dword [cursor_drawn], 0
    je .er_ret

    mov r12d, [0x9100]
    test r12d, r12d
    jz .er_ret

    ; erase at the position where cursor was DRAWN
    mov r13d, [cursor_oy]
    cmp r13d, 768
    jae .er_done
    imul r13d, r13d, 1024
    mov eax, [cursor_ox]
    cmp eax, 1024
    jae .er_done
    add r13d, eax
    shl r13d, 2
    add r13, r12

    lea rdi, [cursor_save]
    mov r15d, CURSOR_H
.er_row:
    mov ebx, CURSOR_H
    sub ebx, r15d
    add ebx, [cursor_oy]
    cmp ebx, 768
    jae .er_done
    mov r14d, CURSOR_W
.er_col:
    dec r14d
    mov ebx, [cursor_ox]
    add ebx, r14d
    cmp ebx, 1024
    jge .er_skip
    mov eax, [rdi]
    mov ebx, r14d
    shl ebx, 2
    mov [r13 + rbx], eax
.er_skip:
    add rdi, 4
    test r14d, r14d
    jnz .er_col
    add r13, 1024 * 4
    dec r15d
    jnz .er_row
.er_done:
    mov dword [cursor_drawn], 0
.er_ret:
    pop r15
    pop r14
    pop rbx
    ret

global draw_cursor
draw_cursor:
    push rbx
    push r14
    push r15

    mov r12d, [0x9100]
    test r12d, r12d
    jz .dr_ret

    mov r13d, [mouse_y]
    cmp r13d, 768
    jae .dr_ret
    imul r13d, r13d, 1024
    mov eax, [mouse_x]
    cmp eax, 1024
    jae .dr_ret
    add r13d, eax
    shl r13d, 2
    add r13, r12

    ; save drawn position so erase uses the RIGHT place
    mov eax, [mouse_x]
    mov [cursor_ox], eax
    mov eax, [mouse_y]
    mov [cursor_oy], eax

    ; mark drawn before loop — fixes bottom-edge trail bug
    mov dword [cursor_drawn], 1

    lea rdi, [cursor_save]
    mov r15d, CURSOR_H
.dr_row:
    mov ebx, CURSOR_H
    sub ebx, r15d                              ; ebx = row (0..20)
    push rbx
    add ebx, [mouse_y]
    cmp ebx, 768
    pop rbx
    jge .dr_row_skip

    mov r14d, CURSOR_W
.dr_col:
    dec r14d
    mov eax, [mouse_x]
    add eax, r14d
    cmp eax, 1024
    jge .dr_save_skip
    mov eax, r14d
    shl eax, 2
    mov eax, [r13 + rax]
    mov [rdi], eax

    ; shape: 21×21 triangle. col <= row = inside.
    cmp r14d, ebx
    jg .dr_transparent
    test r14d, r14d
    jz .dr_outline
    cmp r14d, ebx
    je .dr_outline

.dr_fill:
    mov eax, r14d
    shl eax, 2
    mov dword [r13 + rax], 0x00FFFFFF          ; white fill
    jmp .dr_next

.dr_outline:
    mov eax, r14d
    shl eax, 2
    mov dword [r13 + rax], 0x00000000          ; black outline
    jmp .dr_next

.dr_transparent:
.dr_save_skip:
    add rdi, 4
    test r14d, r14d
    jnz .dr_col
    jmp .dr_row_advance

.dr_next:
    add rdi, 4
    test r14d, r14d
    jnz .dr_col

.dr_row_advance:
    add r13, 1024 * 4
    dec r15d
    jnz .dr_row

    pop r15
    pop r14
    pop rbx
    ret

.dr_row_skip:
    add rdi, CURSOR_W * 4
    add r13, 1024 * 4
    dec r15d
    jnz .dr_row

.dr_ret:
    pop r15
    pop r14
    pop rbx
    ret
