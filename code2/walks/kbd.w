; kbd.w — keyboard: poll PS/2, produce wave byte
;
; Called by find_work during idle. Polls once, returns immediately.
; If key available: scancode → wave byte → save to scratch.
; find_work handles rendering (substrate-level pixel blitting).
;
; Reads:
;   [0x9A30] u64  scancode → wave byte table (256 bytes)
;   [0x9A48] u64  wave byte → ASCII table (boundary converter)
;
; Scratch:
;   0x9B20  scancode
;   0x9B28  wave byte (the character coordinate)
;   0x9B58  "new char" flag (nonzero = kbd produced a character this poll)

; clear new-char flag
π̄₇    θ²²ρ   0x9B58  0

; ── poll PS/2 status port ───────────────────────
πδ̄    θ¹ρρ   0x64                 ; port_read(0x64) → status byte
δ₃    θρ¹ρ   0x01                 ; AND 0x01 = output buffer full bit
skip_z kbd_done                    ; no data → return

; ── read scancode ───────────────────────────────
πδ̄    θ¹ρρ   0x60                 ; port_read(0x60) → scancode
π̄     θ²ρρ   0x9B20               ; save scancode

; ── skip break codes (key release, bit 7 set) ──
δ₃    θρ²ρ   0x00000080           ; AND 0x80
skip_nz kbd_done                   ; break code → skip

; ── table lookup: wave byte = scancode_table[scancode] ──
π     θ²ρρ   0x9A30               ; pipeline = scancode table address
π̄δ̄   θρ²ρ·  0x9B20               ; pipeline += scancode
π₃    θρρρ                         ; pipeline = wave byte
π̄     θ²ρρ   0x9B28               ; save wave byte

; ── set new-char flag (always — 0x00 is space, valid) ──
π̄₇    θ²²ρ   0x9B58  1           ; flag = 1 (find_work will render)

; ── boundary convert wave → ASCII for debugcon (debug VM only) ──
; debugcon is a QEMU serial port that only speaks ASCII.
; on real hardware there is no debugcon — wave bytes go straight
; to the framebuffer renderer. this conversion is scaffolding.
π     θ²ρρ   0x9A48               ; pipeline = wave_to_ascii table address
π̄δ̄   θρ²ρ·  0x9B28               ; pipeline += wave byte
π₃    θρρρ                         ; pipeline = ASCII char
π̄δ    θ¹ρρ   0xE9                 ; port_write(0xE9, ASCII)

: kbd_done
