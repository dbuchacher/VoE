; kbd.asm — keyboard poll walk
;
; Polls the PS/2 keyboard controller forever.
; Port 0x64 = status register. Bit 0 = "a key was pressed, data ready."
; Port 0x60 = data register. Read it to get the scancode.
; Port 0xE9 = QEMU debugcon. Writing a byte here prints it to the host terminal.
;
; The walk loops: check status → if key ready, read scancode → write to debugcon → repeat.
; If no key, skip the read/write and loop back immediately.
;
; The add(1,0) at the end forces pipeline nonzero so loop_back always fires.
; loop_back only rewinds if pipeline != 0. Without this, a zero scancode would stop the loop.

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
