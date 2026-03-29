; kbd_consumer.asm — keyboard loop consumer
;
; Called by drain walk when the keyboard loop has data.
; The record is a wave byte (phonetic coordinate).
; This walk renders it via render_char.
;
; The data from loop_read is in loop_kbd's scratch_a [48].
; Load it, call render_char, done.

bits 64

%include "[0][tool][walk.inc][NASM macros to write walks as coordinates instead of hex].inc"

extern render_char
extern loop_kbd

LP_SCRATCHA equ 48

section .rodata

global kbd_consumer, kbd_consumer_len

kbd_consumer:
    ; debug: consumer entered
    w -1,0,0,+1
    db F(U8,U8,P), 0xE9, 'K'

    ; load the record from loop_kbd scratch_a → pipeline
    w +1,0,0,0                              ; read [loop_kbd + LP_SCRATCHA] → wave byte
    db F(U32,P,P)
    dd (loop_kbd + LP_SCRATCHA)

    ; skip if zero (empty/silence)
    skip_z (.done - .render)

.render:
    ; render the character
    w 0,+1,0,0                              ; apply(render_char, wave_byte)
    db F(U32,P,P)
    dd render_char

.done:
kbd_consumer_end:
kbd_consumer_len: dd (kbd_consumer_end - kbd_consumer)
