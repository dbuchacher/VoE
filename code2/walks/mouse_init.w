; mouse_init.w — enable PS/2 mouse data reporting
;
; Sends commands to the PS/2 controller to enable the auxiliary
; (mouse) port and start data reporting. Must run during genesis
; before find_work polls for mouse packets.
;
; PS/2 protocol:
;   port 0x64 (command): write 0xA8 = enable auxiliary port
;   port 0x64 (command): write 0xD4 = next byte goes to mouse
;   port 0x60 (data):    write 0xF4 = enable data reporting
;   mouse ACKs with 0xFA on port 0x60

; ── enable auxiliary port ───────────────────────
; wait for input buffer empty (bit 1 of status = 0)
: mouse_wait1
πδ̄    θ¹ρρ   0x64
δ₃    θρ¹ρ   0x02
skip_z mouse_cmd1
loop_back mouse_wait1
: mouse_cmd1
π̄δ    θ¹¹ρ   0x64  0xA8           ; enable aux port

; ── send "write to mouse" prefix ────────────────
: mouse_wait2
πδ̄    θ¹ρρ   0x64
δ₃    θρ¹ρ   0x02
skip_z mouse_cmd2
loop_back mouse_wait2
: mouse_cmd2
π̄δ    θ¹¹ρ   0x64  0xD4           ; next byte → mouse

; ── send enable data reporting ──────────────────
: mouse_wait3
πδ̄    θ¹ρρ   0x64
δ₃    θρ¹ρ   0x02
skip_z mouse_cmd3
loop_back mouse_wait3
: mouse_cmd3
π̄δ    θ¹¹ρ   0x60  0xF4           ; enable reporting

; ── read and discard ACK (0xFA) ─────────────────
: mouse_wait_ack
πδ̄    θ¹ρρ   0x64
δ₃    θρ¹ρ   0x01
skip_nz mouse_got_ack
loop_back mouse_wait_ack
: mouse_got_ack
πδ̄    θ¹ρρ   0x60                 ; read ACK byte, discard

; ── init cursor position to center ──────────────
π̄₇    θ²²ρ   0x9B60  512         ; mouse_x = 512 (center of 1024)
π̄₇    θ²²ρ   0x9B68  384         ; mouse_y = 384 (center of 768)
π̄₇    θ²²ρ   0x9B70  512         ; old_mouse_x
π̄₇    θ²²ρ   0x9B78  384         ; old_mouse_y
π̄₇    θ²²ρ   0x9B80  0           ; packet byte count = 0
