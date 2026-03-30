; gpu_nvidia.w — NVIDIA GPU firmware load + init skeleton
;
; Detects NVIDIA GPU, loads firmware from disk, prepares for GSP boot.
; Currently a skeleton — register-level GSP init TBD.
;
; PCI detection: vendor 0x10DE (NVIDIA)
; Supported devices: TU102 (0x1E04, RTX 2080 Ti)
;
; Reads:
;   [0x9400] u64  PCI slot base (from pci.w — but pci.w looks for virtio)
;   [0x9318] u32  GPU firmware LBA start
;   [0x931C] u32  GPU firmware sector count
;
; Output:
;   [0x9600] u64  NVIDIA PCI slot base (0 = not found)
;   [0x9608] u64  NVIDIA BAR0 MMIO address
;   [0x9610] u64  NVIDIA BAR1 MMIO address (VRAM aperture)
;   [0x9618] u64  GPU firmware RAM address (loaded by ATA)
;   [0x9620] u32  GPU firmware size
;
; Scratch:
;   [0x9680] u64  scan slot address
;   [0x9688] u64  scan counter
;   [0x96F0] u64  temp

; ═══════════════════════════════════════════════════════════════
; PART 1: Scan PCI for NVIDIA GPU
; ═══════════════════════════════════════════════════════════════

π̄      θ²²ρ  0x9680  0x80000000     ; slot 0
π̄      θ²²ρ  0x9688  0x00000020     ; 32 slots
π̄      θ²²ρ  0x9600  0x00000000     ; not found

: nv_scan
π₇    θ²ρρ  0x9680
π̄₇δ   θ²ρρ  0x0CF8
π₇δ̄   θ²ρρ  0x0CFC

; check vendor == 0x10DE (NVIDIA)
δ₃     θρ²ρ  0x0000FFFF
δ      θ²ρρ  0x000010DE
skip_nz nv_found

π₇    θ²ρρ  0x9680
π̄δ̄    θρ²ρ  0x00000800
π̄      θ²ρρ  0x9680

π₇    θ²ρρ  0x9688
π̄δ̄₃   θρ¹ρ  1
π̄      θ²ρρ  0x9688
skip_z nv_not_found
loop_back nv_scan

: nv_found
π₇    θ²ρρ  0x9680
π̄      θ²ρρ  0x9600                   ; store NVIDIA slot base

; read BAR0 (offset 0x10) — GPU register MMIO
π₇    θ²ρρ  0x9680
π̄δ̄    θρ²ρ  0x00000010
π̄₇δ   θ²ρρ  0x0CF8
π₇δ̄   θ²ρρ  0x0CFC
δ₃     θρ²ρ  0xFFFFFFF0
π̄      θ²ρρ  0x9608                   ; BAR0

; read BAR1 (offset 0x14) — VRAM aperture
π₇    θ²ρρ  0x9680
π̄δ̄    θρ²ρ  0x00000014
π̄₇δ   θ²ρρ  0x0CF8
π₇δ̄   θ²ρρ  0x0CFC
δ₃     θρ²ρ  0xFFFFFFF0
π̄      θ²ρρ  0x9610                   ; BAR1

; enable PCI bus master + memory space
π₇    θ²ρρ  0x9680
π̄δ̄    θρ²ρ  0x00000004
π̄₇δ   θ²ρρ  0x0CF8
π₇δ̄   θ²ρρ  0x0CFC
δ̄₃     θρ²ρ  0x00000007              ; OR IO + MEM + BUS_MASTER
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ  0x9680
π̄δ̄    θρ²ρ  0x00000004
π̄₇δ   θ²ρρ  0x0CF8
π₇    θ²ρρ  0x96F0
π̄₇δ   θ²ρρ  0x0CFC                  ; write back

; ═══════════════════════════════════════════════════════════════
; PART 2: Load GPU firmware from disk via ATA PIO
; ═══════════════════════════════════════════════════════════════

; skip if no GPU firmware on disk
π₇    θ²ρρ  0x931C                   ; GPU sector count
δ      θ²ρρ  0x00000000
skip_nz nv_skip_ata

; GPU firmware destination: 0x800000 (8MB — above CPU firmware at 6MB)
π̄      θ²²ρ  0x96A0  0x00800000     ; dest
π₇    θ²ρρ  0x9318                   ; LBA start
π̄      θ²ρρ  0x96A8                   ; current LBA
π₇    θ²ρρ  0x931C                   ; sector count
π̄      θ²ρρ  0x96B0                   ; remaining

: nv_ata_sector
; BSY wait
: nv_ata_bsy
πδ̄    θ²ρρ  0x01F7
δ₃     θρ¹ρ  0x80
loop_back nv_ata_bsy

; drive select
π₇    θ²ρρ  0x96A8
δ̄₇     θρ¹ρ  24
δ₃     θρ¹ρ  0x0F
δ̄₃     θρ¹ρ  0xE0
π̄δ    θ²ρρ  0x01F6

; sector count = 1
π̄δ    θ²¹ρ  0x01F2  0x01

; LBA bytes
π₇    θ²ρρ  0x96A8
δ₃     θρ¹ρ  0xFF
π̄δ    θ²ρρ  0x01F3

π₇    θ²ρρ  0x96A8
δ̄₇     θρ¹ρ  8
δ₃     θρ¹ρ  0xFF
π̄δ    θ²ρρ  0x01F4

π₇    θ²ρρ  0x96A8
δ̄₇     θρ¹ρ  16
δ₃     θρ¹ρ  0xFF
π̄δ    θ²ρρ  0x01F5

; command
π̄δ    θ²¹ρ  0x01F7  0x20

; DRQ wait
: nv_ata_drq
πδ̄    θ²ρρ  0x01F7
π̄      θ²ρρ  0x96F0
δ₃     θρ¹ρ  0x08
skip_nz nv_ata_drq_ok
π₇    θ²ρρ  0x96F0
δ₃     θρ¹ρ  0x01
skip_nz nv_skip_ata
π₇    θ²ρρ  0x96F0
loop_back nv_ata_drq
: nv_ata_drq_ok

; read 128 dwords
π₇    θ²ρρ  0x96A0
π∮δ̄   θρ²²  0x000001F0  128

; advance
π₇    θ²ρρ  0x96A8
π̄δ̄    θρ¹ρ  1
π̄      θ²ρρ  0x96A8
π₇    θ²ρρ  0x96A0
π̄δ̄    θρ²ρ  0x00000200
π̄      θ²ρρ  0x96A0
π₇    θ²ρρ  0x96B0
π̄δ̄₃   θρ¹ρ  1
π̄      θ²ρρ  0x96B0
skip_z nv_ata_done
loop_back nv_ata_sector

: nv_ata_done
π̄      θ²²ρ  0x9618  0x00800000     ; store GPU fw RAM address
π₇    θ²ρρ  0x931C
δ₇     θρ¹ρ  9                       ; * 512
π̄₇    θ²ρρ  0x9620                   ; store GPU fw size

: nv_skip_ata

; ═══════════════════════════════════════════════════════════════
; PART 3: Parse firmware pack index
; ═══════════════════════════════════════════════════════════════
;
; The packed gpu_firmware.bin has a directory of files.
; We extract RAM addresses of the 4 files GSP boot needs:
;   bl.bin (tag "bl__")        → [0x9700] addr, [0x9708] size
;   booter_load.bin ("bolo")   → [0x9710] addr, [0x9718] size
;   bootloader.bin ("boot")    → [0x9720] addr, [0x9728] size
;   gsp.bin ("gsp_")           → [0x9730] addr, [0x9738] size
;
; Pack format: FWGP header + device entries + file directory.
; File entries are alphabetical. Known indices for TU102:
;   0=bl, 1=booter_load, 3=bootloader, 13=gsp
;
; Uses [0x96F0-0x96F8] as temp scratch.

; skip if no firmware loaded
π     θ²ρρ  0x9618                   ; GPU fw RAM address
δ      θ²ρρ  0x00000000
skip_nz nv_skip_parse

; verify FWGP magic
π     θ²ρρ  0x9618                   ; fw_base
π̄      θ²ρρ  0x96F0                   ; save fw_base
π₇    θ²ρρ·  0x96F0                  ; read magic
δ      θ²ρρ  0x46574750              ; == "FWGP"?
skip_nz nv_skip_parse                ; bad magic → skip

; read dir_offset from device 0 entry (at fw_base + 16)
π     θ²ρρ  0x9618
π̄δ̄    θρ²ρ  0x00000010
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; dir_offset value

; dir_base = fw_base + dir_offset
π̄δ̄    θ²ρρ·  0x9618                  ; + fw_base = dir_base
π̄      θ²ρρ  0x96F8                   ; save dir_base

; ── bl.bin — entry 0: offset at dir_base+4, size at dir_base+8 ──

π     θ²ρρ  0x96F8                   ; dir_base
π̄δ̄    θρ²ρ  0x00000004              ; + 0*12 + 4
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; data_offset
π̄δ̄    θ²ρρ·  0x9618                  ; + fw_base = RAM addr
π̄      θ²ρρ  0x9700                   ; bl.bin addr

π     θ²ρρ  0x96F8
π̄δ̄    θρ²ρ  0x00000008              ; + 0*12 + 8
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; size
π̄₇    θ²ρρ  0x9708                   ; bl.bin size (dword)

; ── booter_load.bin — entry 1: at dir_base+16/+20 ──

π     θ²ρρ  0x96F8
π̄δ̄    θρ²ρ  0x00000010              ; + 1*12 + 4 = 16
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄δ̄    θ²ρρ·  0x9618
π̄      θ²ρρ  0x9710                   ; booter_load.bin addr

π     θ²ρρ  0x96F8
π̄δ̄    θρ²ρ  0x00000014              ; + 1*12 + 8 = 20
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x9718                   ; booter_load.bin size

; ── bootloader.bin — entry 3: at dir_base+40/+44 ──

π     θ²ρρ  0x96F8
π̄δ̄    θρ²ρ  0x00000028              ; + 3*12 + 4 = 40
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄δ̄    θ²ρρ·  0x9618
π̄      θ²ρρ  0x9720                   ; bootloader.bin addr

π     θ²ρρ  0x96F8
π̄δ̄    θρ²ρ  0x0000002C              ; + 3*12 + 8 = 44
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x9728                   ; bootloader.bin size

; ── gsp.bin — entry 13: at dir_base+160/+164 ──

π     θ²ρρ  0x96F8
π̄δ̄    θρ²ρ  0x000000A0              ; + 13*12 + 4 = 160
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄δ̄    θ²ρρ·  0x9618
π̄      θ²ρρ  0x9730                   ; gsp.bin addr

π     θ²ρρ  0x96F8
π̄δ̄    θρ²ρ  0x000000A4              ; + 13*12 + 8 = 164
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x9738                   ; gsp.bin size

: nv_skip_parse

; ═══════════════════════════════════════════════════════════════
; PART 4: GSP boot sequence
; ═══════════════════════════════════════════════════════════════
@include gsp/gsp_boot.w

skip_z nv_end
skip_nz nv_end

: nv_not_found
: nv_end
