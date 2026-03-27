; speaker.asm — 440 Hz beep walk (wave bytes)
; 4 port_write bonds. Pure constants. 16 bytes (was 128).

walk_speaker_beep:
    db 0xC1, 0x05, 0x43, 0xB6              ; pw: PIT channel 2, mode 3
    db 0xC1, 0x05, 0x42, 0x98              ; pw: divisor low (0x0A98)
    db 0xC1, 0x05, 0x42, 0x0A              ; pw: divisor high
    db 0xC1, 0x05, 0x61, 0x03              ; pw: enable speaker
walk_speaker_beep_end:
