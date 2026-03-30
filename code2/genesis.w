; γ — genesis
;
; display init + firmware + PCI + virtio + GPU + boot confirmation

; ── display ──────────────────────────────────────────────────

π₇    θ²ρρ  0x9100
skip_z fill_end

: fill
π̄∮̄₃   θρ²²  0x00008080  786432
: fill_end

; ── ATA PIO: load firmware from disk ─────────────────────────
@include walks/ata.w

; ── CPU microcode update ─────────────────────────────────────
@include walks/ucode.w

; ── PCI scan for virtio-gpu ──────────────────────────────────
@include walks/pci.w

; ── virtio transport init ────────────────────────────────────
@include walks/virtio.w

; ── virtio-GPU commands ──────────────────────────────────────
@include walks/gpu.w

; ── NVIDIA GPU detection + firmware load ─────────────────────
@include walks/gpu_nvidia.w

; ── boot confirmation ────────────────────────────────────────

π̄δ    θ¹¹ρ  0xE9  0x56
π̄δ    θ¹¹ρ  0xE9  0x6F
π̄δ    θ¹¹ρ  0xE9  0x45
π̄δ    θ¹¹ρ  0xE9  0x0A

; ── loop allocation ─────────────────────────────────────────
;
; Loop header at 0x80000 (fixed physical address, below kernel).
; Buffer at 0x80040. depth=16, mask=15, record_size=1.
; Pre-load 3 records: 'H', 'I', '\n'.
;
; NOTE: BSS loop_region (via [0x9200]) is corrupted on MBR boot.
; Using fixed address until root cause found.
;
; Scratch:
;   0x9B00 = loop header address (persists for consumer walk)
;   0x9B08 = buffer address      (persists for consumer walk)

π̄     θ²²ρ   0x9B00  0x00080000   ; 0x9B00 = header addr = 0x80000
π̄     θ²²ρ   0x9B08  0x00080040   ; 0x9B08 = buffer addr = 0x80040

; ── write header fields ──
; [+0x00] write_cursor = 3
π     θ²ρρ   0x9B00
π̄     θρ³ρ   3

; [+0x10] buffer_address
π     θ²ρρ   0x9B00
π̄δ̄   θρ²ρ   0x10
π̄    θρ²ρ·  0x9B08

; [+0x18] depth = 16
π     θ²ρρ   0x9B00
π̄δ̄   θρ²ρ   0x18
π̄     θρ³ρ   16

; [+0x20] mask = 15
π     θ²ρρ   0x9B00
π̄δ̄   θρ²ρ   0x20
π̄     θρ³ρ   15

; [+0x28] record_size = 1
π     θ²ρρ   0x9B00
π̄δ̄   θρ²ρ   0x28
π̄     θρ³ρ   1

; ── write test records to buffer ──
π     θ²ρρ   0x9B08
π̄₃   θρ¹ρ   0x48                  ; buf[0] = 'H'

π     θ²ρρ   0x9B08
π̄δ̄   θρ²ρ   1
π̄₃   θρ¹ρ   0x49                  ; buf[1] = 'I'

π     θ²ρρ   0x9B08
π̄δ̄   θρ²ρ   2
π̄₃   θρ¹ρ   0x0A                  ; buf[2] = '\n'

; ── register loop in registry ──
; Registry addresses stored by handoff at 0x9A00+

; loop_list[0] = header addr
π     θ²ρρ   0x9B00
π̄    θ²ρρ·  0x9A00

; consumer_walk_list[0] = test_consumer ptr
π     θ²ρρ   0x9A20
π̄    θ²ρρ·  0x9A10

; consumer_wlen_list[0] = test_consumer len
π₇    θ²ρρ   0x9A28
π̄    θ²ρρ·  0x9A18

; loop_count = 1
π     θ²ρρ   0x9A08
π̄     θρ³ρ   1

; ── cursor init ─────────────────────────────────────────────
π̄₇    θ²²ρ   0x9B30  0           ; cursor_x = 0
π̄₇    θ²²ρ   0x9B38  0           ; cursor_y = 0
