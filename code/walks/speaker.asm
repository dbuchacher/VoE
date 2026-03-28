; speaker.asm — 440 Hz beep walk
;
; Makes the PC speaker beep at 440 Hz (concert A).
; Uses the PIT (Programmable Interval Timer) chip on port 0x42/0x43.
; Port 0x61 controls the speaker gate — bit 0 = timer, bit 1 = speaker.
; 4 port_write bonds. Pure constants. No pipeline, no branching.

walk_speaker_beep:
    w -1,0,0,+1                             ; port_write(0x43, 0xB6)
    db F(U8,U8,P), 0x43, 0xB6              ;   PIT command: channel 2, lobyte/hibyte, mode 3 (square wave)

    w -1,0,0,+1                             ; port_write(0x42, 0x98)
    db F(U8,U8,P), 0x42, 0x98              ;   divisor low byte: 1193182 / 440 = 0x0A98

    w -1,0,0,+1                             ; port_write(0x42, 0x0A)
    db F(U8,U8,P), 0x42, 0x0A              ;   divisor high byte

    w -1,0,0,+1                             ; port_write(0x61, 0x03)
    db F(U8,U8,P), 0x61, 0x03              ;   speaker gate ON: bit 0 (timer) + bit 1 (speaker)
walk_speaker_beep_end:
