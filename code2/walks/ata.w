; ata.w — ATA PIO disk read
;
; Reads firmware sectors from disk into RAM.
; Uses ATA PIO mode (port I/O to 0x1F0-0x1F7).
;
; Reads:
;   [0x9300] u32  firmware LBA start sector
;   [0x9304] u32  firmware sector count
;
; Output:
;   [0x9308] u64  firmware RAM address (0x600000)
;   [0x9310] u32  firmware size in bytes
;
; ATA PIO read protocol (28-bit LBA):
;   1. Poll 0x1F7 until BSY clear (bit 7 = 0)
;   2. Write drive select + LBA[27:24] to 0x1F6: 0xE0 | (lba >> 24)
;   3. Write sector count to 0x1F2
;   4. Write LBA[7:0] to 0x1F3
;   5. Write LBA[15:8] to 0x1F4
;   6. Write LBA[23:16] to 0x1F5
;   7. Write 0x20 (READ SECTORS) to 0x1F7
;   8. Poll 0x1F7 until DRQ set (bit 3 = 1) or ERR (bit 0 = 1)
;   9. Read 128 dwords from 0x1F0 (rep insd)
;  10. Repeat 8-9 for each sector
;
; Scratch:
;   [0x9580] u64  current LBA
;   [0x9588] u64  sectors remaining
;   [0x9590] u64  current dest address
;   [0x94F0] u64  temp

π₇    θ²ρρ  0x9304                   ; sector count
; skip if no firmware
π₇    θ²ρρ  0x9304                   ; sector count
δ      θ²ρρ  0x00000000
skip_nz ata_end                      ; count == 0 → skip

; init
π₇    θ²ρρ  0x9300                   ; LBA start
π̄      θ²ρρ  0x9580                   ; current_lba

π₇    θ²ρρ  0x9304                   ; sector count
π̄      θ²ρρ  0x9588                   ; remaining

π̄      θ²²ρ  0x9590  0x00600000     ; dest = 6MB

; store output
π̄      θ²²ρ  0x9308  0x00600000     ; fw RAM address

: ata_sector
; ── step 1: wait for BSY clear ───────────────────────────────
: ata_wait_bsy
πδ̄    θ²ρρ  0x01F7                    ; read status → pipeline
δ₃     θρ¹ρ  0x80                    ; pipeline &= BSY
loop_back ata_wait_bsy               ; non-zero = busy → loop

; ── step 2: drive select + LBA high ─────────────────────────
; 0xE0 | (lba >> 24) → port 0x1F6
π₇    θ²ρρ  0x9580                   ; pipeline = current LBA
δ̄₇     θρ¹ρ  24                      ; pipeline >>= 24
δ₃     θρ¹ρ  0x0F                    ; pipeline &= 0x0F (LBA bits 27:24)
δ̄₃     θρ¹ρ  0xE0                    ; pipeline |= 0xE0 (master, LBA mode)
π̄δ    θ²ρρ  0x01F6                   ; port_write(0x1F6, pipeline)

; ── step 3: sector count = 1 ─────────────────────────────────
π̄δ    θ²¹ρ  0x01F2  0x01              ; port_write(0x1F2, 1)

; ── step 4: LBA low ─────────────────────────────────────────
π₇    θ²ρρ  0x9580                   ; pipeline = LBA
δ₃     θρ¹ρ  0xFF                    ; pipeline &= 0xFF
π̄δ    θ²ρρ  0x01F3                    ; port_write(0x1F3, LBA[7:0])

; ── step 5: LBA mid ─────────────────────────────────────────
π₇    θ²ρρ  0x9580
δ̄₇     θρ¹ρ  8                       ; >> 8
δ₃     θρ¹ρ  0xFF
π̄δ    θ²ρρ  0x01F4                    ; port_write(0x1F4, LBA[15:8])

; ── step 6: LBA high ────────────────────────────────────────
π₇    θ²ρρ  0x9580
δ̄₇     θρ¹ρ  16                      ; >> 16
δ₃     θρ¹ρ  0xFF
π̄δ    θ²ρρ  0x01F5                    ; port_write(0x1F5, LBA[23:16])

; ── step 7: command = READ SECTORS (0x20) ────────────────────
π̄δ    θ²¹ρ  0x01F7  0x20              ; port_write(0x1F7, 0x20)

; ── step 8: wait for DRQ ─────────────────────────────────────
; poll until bit 3 (DRQ) set. Also check bit 0 (ERR).
: ata_wait_drq
πδ̄    θ²ρρ  0x01F7                    ; read status
π̄      θ²ρρ  0x94F0                   ; save status
δ₃     θρ¹ρ  0x08                    ; AND DRQ
skip_nz ata_drq_ok                   ; DRQ set → ready
; check ERR
π₇    θ²ρρ  0x94F0                   ; reload status
δ₃     θρ¹ρ  0x01                    ; AND ERR
skip_nz ata_end                      ; ERR set → abort
; neither DRQ nor ERR — keep polling
π₇    θ²ρρ  0x94F0                   ; reload (non-zero status for loop)
loop_back ata_wait_drq
: ata_drq_ok

; ── step 9: read 128 dwords from port 0x1F0 ─────────────────
; π∮δ̄ = port bulk read: arg0=dest, arg1=port, arg2=dword_count
π₇    θ²ρρ  0x9590                   ; pipeline = dest addr
π∮δ̄   θρ²²  0x000001F0  128          ; rep insd: read 128 dwords from 0x1F0 to [dest]

; ── advance ──────────────────────────────────────────────────
; LBA++
π₇    θ²ρρ  0x9580
π̄δ̄    θρ¹ρ  1                       ; pipeline += 1 (increment doesn't exist for u32 add)

; actually: ∮̄ is increment (+1). But that adds 1 to arg0 which is rdi.
; add θρ¹ρ 1 should work: add(pipeline, 1)
π̄      θ²ρρ  0x9580                   ; save LBA

; dest += 512
π₇    θ²ρρ  0x9590
π̄δ̄    θρ²ρ  0x00000200              ; pipeline += 512
π̄      θ²ρρ  0x9590

; sectors remaining -= 1
π₇    θ²ρρ  0x9588
π̄δ̄₃   θρ¹ρ  1                       ; pipeline -= 1
π̄      θ²ρρ  0x9588
skip_z ata_done                      ; 0 remaining → done
loop_back ata_sector

: ata_done
; store firmware size = sector_count * 512
π₇    θ²ρρ  0x9304                   ; original sector count
δ₇     θρ¹ρ  9                       ; SHL 9 = * 512
π̄₇    θ²ρρ  0x9310                   ; store fw size

: ata_end
