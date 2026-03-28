; kbd.asm — keyboard poll walk (ORIGINAL — debugging GUI crash)

%include "[0][tool][walk.inc][NASM macros to write walks as coordinates instead of hex].inc"

section .rodata

global kbd_walk, kbd_walk_len

kbd_walk:
    w +1,0,0,-1                             ; port_read(0x64): read PS/2 status byte → pipeline
    db F(U8,P,P), 0x64

    w 0,0,0,+3                              ; AND(pipeline, 1): isolate bit 0 (data ready flag)
    db F(P,U8,P), 1

    skip_z 6                                ; if bit 0 == 0 (no key), skip next 6 bytes (read + write)

    w +1,0,0,-1                             ; port_read(0x60): read the scancode → pipeline
    db F(U8,P,P), 0x60

    w -1,0,0,+1                             ; port_write(0xE9, pipeline): send scancode to debugcon
    db F(U8,P,P), 0xE9

    w -1,0,0,-1                             ; add(1, 0): pipeline = 1 (force nonzero for loop_back)
    db F(U8,U8,P), 1, 0

    loop_back 22                            ; rewind 22 bytes → back to port_read(0x64)
kbd_walk_end:
kbd_walk_len: dd (kbd_walk_end - kbd_walk)
