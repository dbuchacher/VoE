; speaker.asm — 440 Hz beep walk
; 4 port_write bonds. Pure constants.

walk_speaker_beep:
    dw -1, 0, 0,+1                             ; pw: PIT channel 2, mode 3
    dq 0x43, 0xB6, 0
    dw -1, 0, 0,+1                             ; pw: divisor low (0x0A98)
    dq 0x42, 0x98, 0
    dw -1, 0, 0,+1                             ; pw: divisor high
    dq 0x42, 0x0A, 0
    dw -1, 0, 0,+1                             ; pw: enable speaker
    dq 0x61, 0x03, 0
walk_speaker_beep_end:
