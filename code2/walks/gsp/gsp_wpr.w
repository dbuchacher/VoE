; gsp_wpr.w — WPR metadata + radix3 page table + RM_RISCV_UCODE_DESC
;
; Builds the WPR metadata struct at 0x2810000 (256 bytes).
; Builds radix3 page table at 0x2800000 mapping the full WPR2 region.
;
; Requires: all 0x9780-0x97D8 from gsp_vram.w
;           [0x9720] bootloader.bin addr, [0x9710] booter_load addr
;           [0x9730] gsp.bin addr
;
; Output:
;   [0x97E0] monitorCodeOffset  (u32, from RM_RISCV_UCODE_DESC)
;   [0x97E4] monitorDataOffset  (u32)
;   [0x97E8] manifestOffset     (u32)
;   [0x97EC] appVersion         (u32)
;   WPR metadata at 0x2810000
;   Radix3 page table at 0x2800000
;
; Scratch: 0x96C0-0x96FF

; ═══════════════════════════════════════════════════════════════
; Parse RM_RISCV_UCODE_DESC from bootloader.bin
; ═══════════════════════════════════════════════════════════════
;
; bootloader.bin has nvfw_bin_hdr at offset 0.
; RM_RISCV_UCODE_DESC starts at header_offset (bin_hdr + 0x0C).
; Fields we need (at offsets within RM_RISCV_UCODE_DESC):
;   +0x1C = appVersion (u32)
;   +0x20 = manifestOffset (u32)
;   +0x28 = monitorCodeOffset (u32) — wait, let me check
;
; Actually the struct layout from the research:
;   +0x00 version, +0x04 bootloaderOffset, +0x08 bootloaderSize,
;   +0x0C bootloaderParamOffset, +0x10 bootloaderParamSize,
;   +0x14 riscvElfOffset, +0x18 riscvElfSize,
;   +0x1C appVersion,
;   +0x20 manifestOffset, +0x24 manifestSize,
;   +0x28 monitorDataOffset, +0x2C monitorDataSize,
;   +0x30 monitorCodeOffset, +0x34 monitorCodeSize

; Read header_offset from bin_hdr
π     θ²ρρ  0x9720                   ; bootloader_addr
π̄δ̄    θρ²ρ  0x0000000C
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; header_offset

; desc_addr = bootloader_addr + header_offset
π̄δ̄    θ²ρρ·  0x9720                  ; + bootloader_addr
π̄      θ²ρρ  0x96C0                   ; save desc_addr

; appVersion at desc + 0x1C
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x0000001C
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x97EC                   ; appVersion

; manifestOffset at desc + 0x20
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000020
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x97E8                   ; manifestOffset

; monitorDataOffset at desc + 0x28
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000028
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x97E4                   ; monitorDataOffset

; monitorCodeOffset at desc + 0x30
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000030
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x97E0                   ; monitorCodeOffset

; bootloader image addr = bootloader_addr + data_offset
π     θ²ρρ  0x9720
π̄δ̄    θρ²ρ  0x00000010
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; data_offset
π̄δ̄    θ²ρρ·  0x9720                  ; + bootloader_addr
π̄      θ²ρρ  0x97F0                   ; bootloader_img_addr (system RAM)

π     θ²ρρ  0x9720
π̄δ̄    θρ²ρ  0x00000014
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x97F8                   ; bootloader_img_size (dword)

; ═══════════════════════════════════════════════════════════════
; Build WPR metadata at 0x2810000 (256 bytes)
; ═══════════════════════════════════════════════════════════════
;
; Zero the page first
π̄∮̄₃   θ²²²  0x02810000  0x00000000  1024

; Write fields as qwords
; Each field: store value at 0x2810000 + offset

; +0x00: magic = 0xdc3aae21371a60b3
π̄      θ²²ρ  0x2810000  0x371A60B3  ; low dword (via dword store + qword store)
; Actually, we need a 64-bit store. π̄ stores a qword (pipeline).
; I need pipeline = 0xdc3aae21371a60b3.
; Can't load a 64-bit immediate in one bond. Use two dword writes.
π̄₇    θ²²ρ  0x2810000  0x371A60B3  ; magic low
π̄₇    θ²²ρ  0x2810004  0xDC3AAE21  ; magic high

; +0x08: revision = 1
π̄₇    θ²²ρ  0x2810008  0x00000001  ; revision low
π̄₇    θ²²ρ  0x281000C  0x00000000  ; revision high

; +0x10: sysmemAddrOfRadix3Elf = 0x02800000 (radix3 level-0)
π̄₇    θ²²ρ  0x2810010  0x02800000
π̄₇    θ²²ρ  0x2810014  0x00000000

; +0x18: sizeOfRadix3Elf = fwimage_size
π₇    θ²ρρ  0x97D8                   ; fwimage_size low 32
π̄₇    θ²ρρ  0x2810018
π̄₇    θ²²ρ  0x281001C  0x00000000  ; high = 0

; +0x20: sysmemAddrOfBootloader = bootloader_img_addr
π     θ²ρρ  0x97F0                   ; bootloader_img_addr (qword)
π̄₇    θ²ρρ  0x2810020               ; low 32
π̄₇    θ²²ρ  0x2810024  0x00000000  ; high = 0 (addr < 4GB)

; +0x28: sizeOfBootloader = bootloader_img_size
π₇    θ²ρρ  0x97F8                   ; dword
π̄₇    θ²ρρ  0x2810028
π̄₇    θ²²ρ  0x281002C  0x00000000

; +0x30: bootloaderCodeOffset = monitorCodeOffset
π₇    θ²ρρ  0x97E0
π̄₇    θ²ρρ  0x2810030
π̄₇    θ²²ρ  0x2810034  0x00000000

; +0x38: bootloaderDataOffset = monitorDataOffset
π₇    θ²ρρ  0x97E4
π̄₇    θ²ρρ  0x2810038
π̄₇    θ²²ρ  0x281003C  0x00000000

; +0x40: bootloaderManifestOffset = manifestOffset
π₇    θ²ρρ  0x97E8
π̄₇    θ²ρρ  0x2810040
π̄₇    θ²²ρ  0x2810044  0x00000000

; +0x48: sysmemAddrOfSignature = 0 (not provided, booter should handle)
; +0x50: sizeOfSignature = 0
; (already zeroed)

; +0x58: gspFwRsvdStart = nonwpr_addr (= heap region start)
π     θ²ρρ  0x97C8                   ; nonwpr_addr
π̄₇    θ²ρρ  0x2810058               ; low
π̄₇    θ²²ρ  0x281005C  0x00000000  ; high (VRAM < 4GB for TU102? no, 8GB)
; For VRAM > 4GB, high dword is nonzero. Need to write proper u64.
; Let me fix this: store full qword via two dword writes from pipeline.
; pipeline holds nonwpr_addr (a 64-bit value from qword read).
; π̄₇ writes low 32 bits. Need to also write high 32 bits.
; High 32 = nonwpr_addr >> 32. But pipeline was modified by the dword write...
; Actually π̄₇ (dword write) returns the value written. So pipeline = low32.
; I need to re-read nonwpr_addr and shift for high bits.
π     θ²ρρ  0x97C8
δ̄₇     θρ¹ρ  32                      ; >> 32 = high dword
π̄₇    θ²ρρ  0x281005C

; +0x60: nonWprHeapOffset = nonwpr_addr
π     θ²ρρ  0x97C8
π̄₇    θ²ρρ  0x2810060
π     θ²ρρ  0x97C8
δ̄₇     θρ¹ρ  32
π̄₇    θ²ρρ  0x2810064

; +0x68: nonWprHeapSize = 0x100000
π̄₇    θ²²ρ  0x2810068  0x00100000
π̄₇    θ²²ρ  0x281006C  0x00000000

; +0x70: gspFwWprStart = wpr2_addr
π     θ²ρρ  0x97B8
π̄₇    θ²ρρ  0x2810070
π     θ²ρρ  0x97B8
δ̄₇     θρ¹ρ  32
π̄₇    θ²ρρ  0x2810074

; +0x78: gspFwHeapOffset = heap_addr
π     θ²ρρ  0x97A8
π̄₇    θ²ρρ  0x2810078
π     θ²ρρ  0x97A8
δ̄₇     θρ¹ρ  32
π̄₇    θ²ρρ  0x281007C

; +0x80: gspFwHeapSize = heap_size
π     θ²ρρ  0x97B0
π̄₇    θ²ρρ  0x2810080
π     θ²ρρ  0x97B0
δ̄₇     θρ¹ρ  32
π̄₇    θ²ρρ  0x2810084

; +0x88: gspFwOffset = elf_addr
π     θ²ρρ  0x97A0
π̄₇    θ²ρρ  0x2810088
π     θ²ρρ  0x97A0
δ̄₇     θρ¹ρ  32
π̄₇    θ²ρρ  0x281008C

; +0x90: bootBinOffset = boot_addr
π     θ²ρρ  0x9798
π̄₇    θ²ρρ  0x2810090
π     θ²ρρ  0x9798
δ̄₇     θρ¹ρ  32
π̄₇    θ²ρρ  0x2810094

; +0x98: frtsOffset = frts_addr
π     θ²ρρ  0x9790
π̄₇    θ²ρρ  0x2810098
π     θ²ρρ  0x9790
δ̄₇     θρ¹ρ  32
π̄₇    θ²ρρ  0x281009C

; +0xA0: frtsSize = 0x100000
π̄₇    θ²²ρ  0x28100A0  0x00100000
π̄₇    θ²²ρ  0x28100A4  0x00000000

; +0xA8: gspFwWprEnd = ALIGN_DOWN(vga_workspace_addr, 0x20000)
π     θ²ρρ  0x9788                   ; vga_workspace_addr
δ₃     θρ²ρ  0xFFFE0000              ; ALIGN_DOWN low bits
π̄₇    θ²ρρ  0x28100A8
π     θ²ρρ  0x9788
δ̄₇     θρ¹ρ  32                      ; high bits unchanged by low alignment
π̄₇    θ²ρρ  0x28100AC

; +0xB0: fbSize = fb_size
π     θ²ρρ  0x9780
π̄₇    θ²ρρ  0x28100B0
π     θ²ρρ  0x9780
δ̄₇     θρ¹ρ  32
π̄₇    θ²ρρ  0x28100B4

; +0xB8: vgaWorkspaceOffset = vga_workspace_addr
π     θ²ρρ  0x9788
π̄₇    θ²ρρ  0x28100B8
π     θ²ρρ  0x9788
δ̄₇     θρ¹ρ  32
π̄₇    θ²ρρ  0x28100BC

; +0xC0: vgaWorkspaceSize = fb_size - vga_workspace_addr
π     θ²ρρ  0x9788                   ; vga_workspace_addr
δ₁₃    θρρρ
π̄δ̄    θ²ρρ·  0x9780                  ; + fb_size = fb_size - vga
π̄₇    θ²ρρ  0x28100C0               ; low
π     θ²ρρ  0x9788
δ₁₃    θρρρ
π̄δ̄    θ²ρρ·  0x9780
δ̄₇     θρ¹ρ  32
π̄₇    θ²ρρ  0x28100C4               ; high

; +0xC8 onwards: bootCount=0, partitions=0, etc — already zeroed

; ═══════════════════════════════════════════════════════════════
; Build radix3 page table for WPR2 region
; ═══════════════════════════════════════════════════════════════
;
; The radix3 maps wpr2_size bytes of system RAM.
; Sparse mapping:
;   - Page 0 (offset 0 from wpr2): WPR metadata at 0x2810000
;   - Pages at (elf_addr - wpr2_addr)/4096: gsp.bin fwimage pages
;   - Pages at (boot_addr - wpr2_addr)/4096: bootloader image pages
;   - All other pages: zeroed page at 0x2820000

; Zero the page table area + zero page (80KB should be plenty)
π̄∮̄₃   θ²²²  0x02800000  0x00000000  20480

; Total pages in WPR2
π     θ²ρρ  0x97C0                   ; wpr2_size
π̄δ̄    θρ²ρ  0x00000FFF
δ̄₇     θρ¹ρ  12                      ; ceil(wpr2_size / 4096)
π̄      θ²ρρ  0x96C0                   ; total_pages

; ── Fill ALL level-2 entries with zero page (0x2820000) ──
; Then overwrite specific entries for actual data pages.

; Level 2 starts at 0x2802000
; Each entry is u64 (8 bytes)
π̄      θ²²ρ  0x96C8  0x02802000     ; level2_ptr
π     θ²ρρ  0x96C0                   ; total_pages
π̄      θ²ρρ  0x96D0                   ; remaining

; The zero page is at 0x2820000 (pre-zeroed above)
π̄      θ²²ρ  0x96D8  0x02820000     ; zero_page_addr

: gsp_r3_fill_zero
π     θ²ρρ  0x96D8                   ; zero_page_addr
π̄      θ²ρρ·  0x96C8                  ; write to [level2_ptr]
; advance level2_ptr
π     θ²ρρ  0x96C8
π̄δ̄    θρ²ρ  0x00000008
π̄      θ²ρρ  0x96C8
; decrement
π     θ²ρρ  0x96D0
π̄δ̄₃   θρ¹ρ  1
π̄      θ²ρρ  0x96D0
skip_z gsp_r3_fill_zero_done
loop_back gsp_r3_fill_zero
: gsp_r3_fill_zero_done

; ── Overwrite page 0: WPR metadata at 0x2810000 ──
π̄      θ²²ρ  0x2802000  0x02810000  ; level2[0] = WPR meta page

; ── Overwrite ELF pages: fwimage at gsp_addr + 0x40 ──
; ELF offset in WPR2 = elf_addr - wpr2_addr
π     θ²ρρ  0x97B8                   ; wpr2_addr
δ₁₃    θρρρ
π̄δ̄    θ²ρρ·  0x97A0                  ; + elf_addr = elf_addr - wpr2_addr
δ̄₇     θρ¹ρ  12                      ; / 4096 = elf_page_index
π̄      θ²ρρ  0x96C0                   ; save elf_page_index

; level2 entry address for elf_page_index
; entry_addr = 0x2802000 + elf_page_index * 8
δ₇     θρ¹ρ  3                       ; * 8
π̄δ̄    θρ²ρ  0x02802000              ; + level2_base
π̄      θ²ρρ  0x96C8                   ; save elf_entry_addr

; Number of ELF pages
π     θ²ρρ  0x97D8                   ; fwimage_size
π̄δ̄    θρ²ρ  0x00000FFF
δ̄₇     θρ¹ρ  12
π̄      θ²ρρ  0x96D0                   ; elf_page_count

; Source: fwimage pages in system RAM
π     θ²ρρ  0x97D0                   ; fwimage_addr (system RAM)
π̄      θ²ρρ  0x96D8                   ; current_page_addr

: gsp_r3_elf_loop
π     θ²ρρ  0x96D8                   ; page addr
π̄      θ²ρρ·  0x96C8                  ; write to [entry_addr]

; advance
π     θ²ρρ  0x96D8
π̄δ̄    θρ²ρ  0x00001000              ; next page
π̄      θ²ρρ  0x96D8
π     θ²ρρ  0x96C8
π̄δ̄    θρ²ρ  0x00000008              ; next entry
π̄      θ²ρρ  0x96C8

π     θ²ρρ  0x96D0
π̄δ̄₃   θρ¹ρ  1
π̄      θ²ρρ  0x96D0
skip_z gsp_r3_elf_done
loop_back gsp_r3_elf_loop
: gsp_r3_elf_done

; ── Overwrite bootloader pages ──
; boot offset in WPR2 = boot_addr - wpr2_addr
π     θ²ρρ  0x97B8
δ₁₃    θρρρ
π̄δ̄    θ²ρρ·  0x9798                  ; + boot_addr = boot_addr - wpr2_addr
δ̄₇     θρ¹ρ  12                      ; / 4096 = boot_page_index
δ₇     θρ¹ρ  3                       ; * 8
π̄δ̄    θρ²ρ  0x02802000
π̄      θ²ρρ  0x96C8                   ; boot_entry_addr

; boot pages
π₇    θ²ρρ  0x97F8                   ; bootloader_img_size (dword)
π̄δ̄    θρ²ρ  0x00000FFF
δ̄₇     θρ¹ρ  12
π̄      θ²ρρ  0x96D0                   ; boot_page_count

π     θ²ρρ  0x97F0                   ; bootloader_img_addr
π̄      θ²ρρ  0x96D8

: gsp_r3_boot_loop
π     θ²ρρ  0x96D8
π̄      θ²ρρ·  0x96C8

π     θ²ρρ  0x96D8
π̄δ̄    θρ²ρ  0x00001000
π̄      θ²ρρ  0x96D8
π     θ²ρρ  0x96C8
π̄δ̄    θρ²ρ  0x00000008
π̄      θ²ρρ  0x96C8

π     θ²ρρ  0x96D0
π̄δ̄₃   θρ¹ρ  1
π̄      θ²ρρ  0x96D0
skip_z gsp_r3_boot_done
loop_back gsp_r3_boot_loop
: gsp_r3_boot_done

; ── Build level 1 ──
; L2 pages needed = ceil(total_pages / 512)
π     θ²ρρ  0x96C0                   ; total_pages (saved earlier... wait, was it clobbered?)
; total_pages was at 0x96C0, which we reused for elf_page_index. Recompute.
π     θ²ρρ  0x97C0                   ; wpr2_size
π̄δ̄    θρ²ρ  0x00000FFF
δ̄₇     θρ¹ρ  12
π̄      θ²ρρ  0x96C0                   ; total_pages (recomputed)

π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x000001FF
δ̄₇     θρ¹ρ  9                       ; ceil(total_pages / 512) = L2_page_count
π̄      θ²ρρ  0x96D0                   ; L2_page_count

π̄      θ²²ρ  0x96C8  0x02801000     ; level1_ptr
π̄      θ²²ρ  0x96D8  0x02802000     ; current L2 page addr

: gsp_r3_l1_loop
π     θ²ρρ  0x96D8
π̄      θ²ρρ·  0x96C8                  ; write to [level1_ptr]

π     θ²ρρ  0x96D8
π̄δ̄    θρ²ρ  0x00001000
π̄      θ²ρρ  0x96D8
π     θ²ρρ  0x96C8
π̄δ̄    θρ²ρ  0x00000008
π̄      θ²ρρ  0x96C8

π     θ²ρρ  0x96D0
π̄δ̄₃   θρ¹ρ  1
π̄      θ²ρρ  0x96D0
skip_z gsp_r3_l1_done
loop_back gsp_r3_l1_loop
: gsp_r3_l1_done

; Level 0: single entry → level 1
π̄      θ²²ρ  0x2800000  0x02801000
