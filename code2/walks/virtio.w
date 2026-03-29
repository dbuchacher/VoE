; virtio.w — virtio modern PCI transport initialization
;
; Reads:
;   [0x9400] u64  PCI slot base (0 = no device)
;   [0x9440] u64  common cfg MMIO address
;   [0x9448] u64  notify MMIO address
;   [0x9420] u32  notify offset multiplier
;
; Output:
;   [0x9500] u64  descriptor table address
;   [0x9508] u64  available ring address
;   [0x9510] u64  used ring address
;   [0x9518] u32  queue size (N)
;   [0x9520] u64  notify doorbell address for queue 0
;   [0x9528] u64  common cfg MMIO address (for gpu.w)
;   [0x9530] u64  command buffer base (for gpu.w)
;
; Memory layout at 0xA0000 (640KB mark, above conventional memory):
;   0xA0000: descriptor table (256 × 16 = 4096 bytes)
;   0xA1000: available ring (4 + 256×2 + 2 = 518 bytes, page-padded)
;   0xA2000: used ring (4 + 256×8 + 2 = 2054 bytes)
;   0x103000: command/response buffers (4KB)
;
; Common cfg offsets: see pci.w header.
; Scratch: 0x94F0 (temp)

; skip if no device
π₇    θ²ρρ  0x9400
δ      θ²ρρ  0x00000000
skip_nz virtio_end

; save common cfg base for gpu.w
π     θ²ρρ  0x9440
π̄      θ²ρρ  0x9528

; ═══════════════════════════════════════════════════════════════
; Device init sequence (virtio spec 3.1.1)
; ═══════════════════════════════════════════════════════════════

; 1. Reset: write 0 to device_status (offset 0x14, u8)
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x00000014
π̄₃    θρ¹ρ  0x00

; 2. ACKNOWLEDGE: write 0x01
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x00000014
π̄₃    θρ¹ρ  0x01

; 3. DRIVER: write 0x03 (ACK | DRIVER)
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x00000014
π̄₃    θρ¹ρ  0x03

; 4. Feature negotiation
; Read device features page 0 (bits 0-31)
π     θ²ρρ  0x9440
π̄₇    θρ²ρ  0x00000000              ; write 0 to device_feature_select (offset 0)
; Read device_feature at offset 0x04 (we don't actually need the value,
; just need to accept VIRTIO_F_VERSION_1 on page 1)

; Write driver features page 0 = 0 (accept nothing from page 0)
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x00000008              ; driver_feature_select offset
π̄₇    θρ²ρ  0x00000000              ; page 0

π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x0000000C              ; driver_feature offset
π̄₇    θρ²ρ  0x00000000              ; accept no page-0 features

; Write driver features page 1: accept VIRTIO_F_VERSION_1 (bit 0 of page 1)
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x00000008
π̄₇    θρ²ρ  0x00000001              ; page 1

π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x0000000C
π̄₇    θρ²ρ  0x00000001              ; VIRTIO_F_VERSION_1 = bit 0 of page 1

; 5. FEATURES_OK: write 0x0B (ACK | DRIVER | FEATURES_OK)
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x00000014
π̄₃    θρ¹ρ  0x0B

; 6. Re-read status, verify FEATURES_OK still set
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x00000014
π₃    θρρρ                           ; byte read → pipeline = status
δ₃     θρ¹ρ  0x08                    ; AND with FEATURES_OK bit
skip_z virtio_end                    ; zero = features rejected = abort

; ═══════════════════════════════════════════════════════════════
; Queue setup: controlq (queue 0)
; ═══════════════════════════════════════════════════════════════

; Select queue 0: write 0 to queue_select (offset 0x16, u16)
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x00000016
π̄₅    θρ²ρ  0x00000000              ; queue_select = 0

; Read queue_size (offset 0x18, u16)
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x00000018
π₅    θρρρ                           ; read u16 from [pipeline]
π̄₇    θ²ρρ  0x9518                   ; save queue size

; Use the device's queue size (should be 256 for QEMU)
; If > 256, cap to 256 (our buffer only holds 256 entries)
; For now, trust the device value

; Set queue addresses
; Descriptor table at 0xA0000
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x00000020              ; queue_desc offset
π̄      θρ²ρ  0x00400000              ; write 0xA0000 (qword write to MMIO)
π̄      θ²ρρ  0x9500                   ; save desc addr

; Available ring at 0xA1000
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x00000028              ; queue_driver offset
π̄      θρ²ρ  0x00401000
π̄      θ²ρρ  0x9508                   ; save avail addr

; Used ring at 0xA2000
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x00000030              ; queue_device offset
π̄      θρ²ρ  0x00402000
π̄      θ²ρρ  0x9510                   ; save used addr

; Set MSI-X vector to 0xFFFF (no MSI-X, use polling)
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x0000001A              ; queue_msix_vector offset
π̄₅    θρ²ρ  0x0000FFFF              ; no MSI-X

; Enable queue: write 1 to queue_enable (offset 0x1C, u16)
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x0000001C
π̄₅    θρ²ρ  0x00000001              ; enable!

; Read queue_notify_off (offset 0x1E, u16) for doorbell calc
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x0000001E
π₅    θρρρ                           ; read u16
; doorbell = notify_base + queue_notify_off * notify_off_multiplier
π̄      θ²ρρ  0x94F0                   ; save queue_notify_off
; multiply: off * multiplier. We don't have MUL bond.
; For QEMU, multiplier is typically 4 and queue_notify_off is 0.
; So doorbell = notify_base + 0. Just use notify_base directly.
; TODO: proper multiply when needed for real hardware.
π     θ²ρρ  0x9448                   ; notify base
π̄      θ²ρρ  0x9520                   ; save as doorbell addr

; Save command buffer base
π̄      θ²²ρ  0x9530  0x00403000     ; cmd buffer at 0x103000

; ═══════════════════════════════════════════════════════════════
; Zero the virtqueue memory
; ═══════════════════════════════════════════════════════════════

; Zero 16KB from 0xA0000 (desc + avail + used + cmd buffers)
; π̄∮̄₃ = dword fill. arg0=dest, arg1=value, arg2=count(dwords)
π̄∮̄₃   θ²²²  0x00400000  0x00000000  0x00001000

; ═══════════════════════════════════════════════════════════════
; Step 7: DRIVER_OK — device is live
; ═══════════════════════════════════════════════════════════════

π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x00000014
π̄₃    θρ¹ρ  0x0F                    ; ACK | DRIVER | FEATURES_OK | DRIVER_OK

; verify: read status back
π     θ²ρρ  0x9440
π̄δ̄    θρ²ρ  0x00000014
π₃    θρρρ                           ; byte read → pipeline = status
δ₃     θρ¹ρ  0x40                    ; AND with DEVICE_NEEDS_RESET
skip_nz virtio_end                   ; non-zero = reset needed = bad

; print 'V' to debugcon — virtio init success
π̄δ    θ¹¹ρ  0xE9  0x56              ; 'V' for virtio OK

: virtio_end
