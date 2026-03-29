; ═══════════════════════════════════════════════════════════════
; PHASE 2: SEC2 falcon reset
; ═══════════════════════════════════════════════════════════════
;
; SEC2 base = BAR0 + 0x840000
; The booter_load firmware runs on SEC2, not GSP.

; Compute SEC2 base address
π     θ²ρρ  0x9608                   ; BAR0
π̄δ̄    θρ²ρ  0x00840000              ; + SEC2 offset
π̄      θ²ρρ  0x96C0                   ; save sec2_base

; Assert reset: write 1 to sec2_base + 0x3C0
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x000003C0
π̄      θ²ρρ  0x96F0                   ; save reset_reg addr
π₇    θ²ρρ·  0x96F0                  ; read current
δ̄₃     θρ²ρ  0x00000001              ; OR bit 0
π̄₇    θ²ρρ·  0x96F0                  ; write back (assert reset)

; wait ~10us
π̄      θ²²ρ  0x96E8  0x00000100
: sec2_rst_wait
π     θ²ρρ  0x96E8
π̄δ̄₃   θρ¹ρ  1
π̄      θ²ρρ  0x96E8
skip_z sec2_rst_done
loop_back sec2_rst_wait
: sec2_rst_done

; Deassert reset: clear bit 0
π₇    θ²ρρ·  0x96F0                  ; read
δ₃     θρ²ρ  0xFFFFFFFE              ; AND ~bit0
π̄₇    θ²ρρ·  0x96F0                  ; write

; Wait for memory scrub done: sec2_base + 0x10C, bits 1:2 = 0
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x0000010C
π̄      θ²ρρ  0x96F8                   ; save scrub_reg addr

: sec2_scrub_wait
π₇    θ²ρρ·  0x96F8
δ₃     θρ²ρ  0x00000006              ; AND scrub bits
loop_back sec2_scrub_wait             ; non-zero = still scrubbing

; Clear scrub register
π̄₇    θ²²ρ  0x96E8  0x00000000
π₇    θ²ρρ  0x96E8
π̄₇    θ²ρρ·  0x96F8                  ; write 0 to scrub reg

; Set DMA control bit 7 (non-VMM mode)
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000624
π̄      θ²ρρ  0x96F8
π̄₇    θ²²ρ  0x96E8  0x00000080
π₇    θ²ρρ  0x96E8
π̄₇    θ²ρρ·  0x96F8                  ; DMA_CTRL = 0x80

; ═══════════════════════════════════════════════════════════════
; PHASE 3: PIO load bl.bin onto SEC2 IMEM + DMEM
; ═══════════════════════════════════════════════════════════════
;
; bl.bin has nvfw_bin_hdr at offset 0:
;   +0x10 = data_offset (image data start in file)
;   +0x0C = header_offset → nvfw_bl_desc at that offset:
;     +0x04 = dmem_load_off
;     +0x08 = code_off (within image)
;     +0x0C = code_size
;     +0x10 = data_off (within image)
;     +0x14 = data_size
;
; Steps:
;   1. Parse bl.bin headers
;   2. Read HWCFG → compute boot_off (top of IMEM - code_size)
;   3. PIO write code to IMEM at boot_off
;   4. PIO write data to DMEM at dmem_load_off
;   5. Write BLD (boot loader descriptor) to DMEM at 0

; ── 3a. Parse bl.bin ──

; Read data_offset from bl.bin bin_hdr
π     θ²ρρ  0x9700                   ; bl_addr
π̄δ̄    θρ²ρ  0x00000010              ; + 0x10
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; bl_data_offset
π̄₇    θ²ρρ  0x96D0                   ; save bl_data_offset (dword)

; Read header_offset → internal header
π     θ²ρρ  0x9700
π̄δ̄    θρ²ρ  0x0000000C
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; bl_header_offset
; hdr_addr = bl_addr + header_offset
π̄δ̄    θ²ρρ·  0x9700                  ; + bl_addr = hdr_addr
π̄      θ²ρρ  0x96D8                   ; save hdr_addr

; Read code_off, code_size, data_off, data_size from nvfw_bl_desc
π     θ²ρρ  0x96D8
π̄δ̄    θρ²ρ  0x00000008              ; hdr + 8 = code_off
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x96E0                   ; save bl_code_off

π     θ²ρρ  0x96D8
π̄δ̄    θρ²ρ  0x0000000C              ; hdr + 12 = code_size
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x96E4                   ; save bl_code_size

π     θ²ρρ  0x96D8
π̄δ̄    θρ²ρ  0x00000010              ; hdr + 16 = data_off
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x96E8                   ; save bl_data_off_in_img

π     θ²ρρ  0x96D8
π̄δ̄    θρ²ρ  0x00000014              ; hdr + 20 = data_size
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x96EC                   ; save bl_data_size

; Compute bl code source: bl_addr + bl_data_offset + code_off
π₇    θ²ρρ  0x96D0                   ; bl_data_offset
π̄δ̄    θ²ρρ·  0x9700                  ; + bl_addr
π₇    θ²ρρ  0x96E0                   ; + code_off (it's a dword at 0x96E0)
; hmm — need to add two runtime values. Pipeline has bl_addr+data_off.
; Need to add code_off. But code_off is a dword at 0x96E0.
; π̄δ̄ θ²ρρ· 0x96E0 would do [0x96E0] + pipeline... but [0x96E0] is read as qword
; Actually: deref reads a qword from that address. But we stored a dword.
; The upper bytes at 0x96E4 contain bl_code_size. So reading qword from 0x96E0
; gives us code_off | (code_size << 32). Adding that to pipeline would be wrong.
;
; Safer: use a temp. Store bl_addr+data_off, then add code_off manually.
π̄      θ²ρρ  0x96F0                   ; save bl_addr + bl_data_offset
π₇    θ²ρρ  0x96E0                   ; pipeline = code_off (dword, zero-extended)
π̄δ̄    θ²ρρ·  0x96F0                  ; pipeline = [0x96F0] + code_off
π̄      θ²ρρ  0x96F0                   ; bl_code_src = bl_addr + data_off + code_off

; ── WAIT: we stored dwords at 0x96E0-0x96EF but π reads qwords from 0x96E0
; The dword read π₇ is safe — reads 4 bytes only. But we used π₇ above.
; Correction: π₇ θ²ρρ 0x96E0 reads dword from 0x96E0 → pipeline = code_off ✓
; Then π̄δ̄ θ²ρρ· 0x96F0 reads qword [0x96F0] → pipeline = that + code_off ✓
; ── This is correct. ──

; Compute bl data source: bl_addr + bl_data_offset + data_off_in_img
π₇    θ²ρρ  0x96D0                   ; bl_data_offset (dword)
π̄δ̄    θ²ρρ·  0x9700                  ; + bl_addr
π̄      θ²ρρ  0x96F8                   ; temp = bl_addr + data_offset
π₇    θ²ρρ  0x96E8                   ; data_off_in_img (dword)
π̄δ̄    θ²ρρ·  0x96F8                  ; + temp
π̄      θ²ρρ  0x96F8                   ; bl_data_src

; ── 3b. Read HWCFG, compute boot_off ──

; HWCFG = sec2_base + 0x108
π     θ²ρρ  0x96C0                   ; sec2_base
π̄δ̄    θρ²ρ  0x00000108
π̄      θ²ρρ  0x96D8
π₇    θ²ρρ·  0x96D8                  ; HWCFG value
δ₃     θρ²ρ  0x000001FF              ; AND 0x1FF
δ₇     θρ¹ρ  8                       ; << 8 = code_limit
; boot_off = code_limit - bl_code_size
π̄      θ²ρρ  0x96D8                   ; save code_limit
π₇    θ²ρρ  0x96E4                   ; bl_code_size (dword)
δ₁₃    θρρρ                          ; negate → pipeline = -code_size
π̄δ̄    θ²ρρ·  0x96D8                  ; + code_limit = boot_off
π̄      θ²ρρ  0x96D0                   ; save boot_off (reuse scratch)

; ── 3c. PIO write bl code to IMEM ──

; Set IMEM port control: non-secure | auto-increment | boot_off
; Value = BIT(24) | boot_off
π₇    θ²ρρ  0x96D0                   ; boot_off (dword)
δ̄₃     θρ²ρ  0x01000000              ; OR BIT(24)
; Write to sec2_base + 0x180
π̄      θ²ρρ  0x96D8                   ; save port_ctrl value
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000180
π̄      θ²ρρ  0x96E8                   ; save IMEM_PORT addr
π₇    θ²ρρ  0x96D8                   ; restore port_ctrl
π̄₇    θ²ρρ·  0x96E8                  ; write port control

; Compute IMEM_DATA and IMEM_TAG MMIO addresses
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000184
π̄      θ²ρρ  0x96D8                   ; IMEM_DATA addr

π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000188
π̄      θ²ρρ  0x96E8                   ; IMEM_TAG addr

; Starting tag = boot_off >> 8
π₇    θ²ρρ  0x96D0                   ; boot_off
δ̄₇     θρ¹ρ  8                       ; >> 8
π̄₇    θ²ρρ  0x96EC                   ; save current_tag (dword)

; Code loop: write bl_code_size bytes in 256-byte blocks
; Each block: write tag, then 64 dwords
; blocks = code_size / 256 = code_size >> 8
π₇    θ²ρρ  0x96E4                   ; bl_code_size
δ̄₇     θρ¹ρ  8                       ; >> 8 = block_count
π̄₇    θ²ρρ  0x96E0                   ; save block_count

; src_ptr = bl_code_src (saved at 0x96F0 from 3a)
; 0x96F0 already contains bl_code_src

: sec2_imem_block
; Write tag for this block
π₇    θ²ρρ  0x96EC                   ; current_tag
π̄₇    θ²ρρ·  0x96E8                  ; write to IMEM_TAG

; 64 dwords per block
π̄₇    θ²²ρ  0x96E4  0x00000040     ; dword_count = 64

: sec2_imem_dword
π₇    θ²ρρ·  0x96F0                  ; read dword from [src_ptr]
π̄₇    θ²ρρ·  0x96D8                  ; write to IMEM_DATA

; advance src_ptr by 4
π     θ²ρρ  0x96F0
π̄δ̄    θρ²ρ  0x00000004
π̄      θ²ρρ  0x96F0

; decrement dword count
π₇    θ²ρρ  0x96E4
π̄δ̄₃   θρ¹ρ  1
π̄₇    θ²ρρ  0x96E4
skip_z sec2_imem_dword_done
loop_back sec2_imem_dword
: sec2_imem_dword_done

; advance tag, decrement block count
π₇    θ²ρρ  0x96EC
π̄δ̄    θρ¹ρ  1
π̄₇    θ²ρρ  0x96EC

π₇    θ²ρρ  0x96E0
π̄δ̄₃   θρ¹ρ  1
π̄₇    θ²ρρ  0x96E0
skip_z sec2_imem_done
loop_back sec2_imem_block
: sec2_imem_done

; ── 3d. PIO write bl data to DMEM ──

; DMEM port control: auto-increment | dmem_load_off (=0)
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x000001C0
π̄      θ²ρρ  0x96E8                   ; DMEM_PORT addr

π̄₇    θ²²ρ  0x96D0  0x01000000     ; port_ctrl = BIT(24) | 0
π₇    θ²ρρ  0x96D0
π̄₇    θ²ρρ·  0x96E8                  ; write port control

; DMEM_DATA address
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x000001C4
π̄      θ²ρρ  0x96D0                   ; DMEM_DATA addr

; src_ptr = bl_data_src (saved at 0x96F8 from 3a)
; Re-read bl_data_size from header (0x96EC was clobbered by IMEM tag)
π     θ²ρρ  0x9700                   ; bl_addr
π̄δ̄    θρ²ρ  0x0000000C
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; bl_header_offset
π̄δ̄    θ²ρρ·  0x9700                  ; + bl_addr = hdr_addr
π̄δ̄    θρ²ρ  0x00000014              ; + 0x14 = data_size field
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; bl_data_size
δ̄₇     θρ¹ρ  2                       ; >> 2 = dword count
π̄₇    θ²ρρ  0x96E4                   ; save dword_count

: sec2_dmem_bl_dword
π₇    θ²ρρ·  0x96F8                  ; read dword from [bl_data_src]
π̄₇    θ²ρρ·  0x96D0                  ; write to DMEM_DATA

π     θ²ρρ  0x96F8
π̄δ̄    θρ²ρ  0x00000004
π̄      θ²ρρ  0x96F8

π₇    θ²ρρ  0x96E4
π̄δ̄₃   θρ¹ρ  1
π̄₇    θ²ρρ  0x96E4
skip_z sec2_dmem_bl_done
loop_back sec2_dmem_bl_dword
: sec2_dmem_bl_done

; ── 3e. Patch booter_load signature ──
;
; booter_load.bin hs_header_v2 (at file offset 0x18):
;   +0x00 = sig_prod_offset (0x3C)
;   +0x04 = sig_prod_size (0x10 = 16 bytes)
;   +0x08 = patch_loc_off → file[patch_loc_off] = DMEM offset to patch
;   +0x0C = patch_sig_off → file[patch_sig_off] = sig base offset
;
; We copy 16 bytes of signature into the image data at the patch location.
; sig_src = booter_addr + sig_prod_offset
; sig_dst = booter_addr + image_data_offset + [booter_addr + patch_loc_off]
;
; For TU102: sig at +0x3C (16 bytes), patched to image+0x8700
;
; Read sig_prod_offset from booter hs_header
π     θ²ρρ  0x9710                   ; booter_load addr
π̄δ̄    θρ²ρ  0x00000018              ; + 0x18 = hs_header
; Read sig_prod_offset (first field of hs_header)
π̄      θ²ρρ  0x96E0
π₇    θ²ρρ·  0x96E0                  ; sig_prod_offset (e.g., 0x3C)

; sig_src = booter_addr + sig_prod_offset
π̄δ̄    θ²ρρ·  0x9710                  ; + booter_addr
π̄      θ²ρρ  0x96E0                   ; save sig_src

; Read patch_loc_off (hs_header + 8)
π     θ²ρρ  0x9710
π̄δ̄    θρ²ρ  0x00000020              ; + 0x18 + 0x08 = 0x20
π̄      θ²ρρ  0x96E8
π₇    θ²ρρ·  0x96E8                  ; patch_loc_off

; Dereference: [booter_addr + patch_loc_off] = image offset to patch
π̄δ̄    θ²ρρ·  0x9710                  ; + booter_addr = addr of patch_loc value
π̄      θ²ρρ  0x96E8
π₇    θ²ρρ·  0x96E8                  ; patch_loc value (e.g., 0x8700)

; Read booter image_data_offset (bin_hdr + 0x10)
π̄      θ²ρρ  0x96EC                   ; save patch_loc (dword)
π     θ²ρρ  0x9710
π̄δ̄    θρ²ρ  0x00000010              ; + 0x10
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; image_data_offset (e.g., 0x88)

; sig_dst = booter_addr + image_data_offset + patch_loc
π̄δ̄    θ²ρρ·  0x9710                  ; + booter_addr
π₇    θ²ρρ  0x96EC                   ; + patch_loc
π̄δ̄    θ²ρρ·  0x96F0                  ; hmm, 0x96F0 was modified...

; Let me redo this more carefully
; 0x96F0 now holds addr we used to read img_data_off, not img_data_off itself.
; Pipeline holds img_data_offset after the read.
; So: pipeline = img_data_offset + booter_addr (from add above)
; Then add patch_loc:
π̄      θ²ρρ  0x96F0                   ; save booter_addr + img_data_offset
π₇    θ²ρρ  0x96EC                   ; patch_loc (dword)
π̄δ̄    θ²ρρ·  0x96F0                  ; + (booter_addr + img_data_offset)
π̄      θ²ρρ  0x96F0                   ; sig_dst

; Copy 4 dwords (16 bytes) from sig_src to sig_dst
; Simple unrolled copy: read from [sig_src], write to [sig_dst], advance both

π₇    θ²ρρ·  0x96E0                  ; read dword 0 from [sig_src]
π̄₇    θ²ρρ·  0x96F0                  ; write to [sig_dst]

; advance both by 4
π     θ²ρρ  0x96E0
π̄δ̄    θρ²ρ  0x00000004
π̄      θ²ρρ  0x96E0
π     θ²ρρ  0x96F0
π̄δ̄    θρ²ρ  0x00000004
π̄      θ²ρρ  0x96F0

π₇    θ²ρρ·  0x96E0
π̄₇    θ²ρρ·  0x96F0

π     θ²ρρ  0x96E0
π̄δ̄    θρ²ρ  0x00000004
π̄      θ²ρρ  0x96E0
π     θ²ρρ  0x96F0
π̄δ̄    θρ²ρ  0x00000004
π̄      θ²ρρ  0x96F0

π₇    θ²ρρ·  0x96E0
π̄₇    θ²ρρ·  0x96F0

π     θ²ρρ  0x96E0
π̄δ̄    θρ²ρ  0x00000004
π̄      θ²ρρ  0x96E0
π     θ²ρρ  0x96F0
π̄δ̄    θρ²ρ  0x00000004
π̄      θ²ρρ  0x96F0

π₇    θ²ρρ·  0x96E0
π̄₇    θ²ρρ·  0x96F0

; ── 3f. Write BLD (flcn_bl_dmem_desc_v2) to DMEM at offset 0 ──
;
; The BLD overwrites the first 84 bytes of bl.bin data at DMEM 0.
; It tells the bootloader (bl.bin) where to DMA booter_load from RAM.
;
; BLD layout (84 bytes = 21 dwords):
;   +0x00  reserved[4]       = 0,0,0,0
;   +0x10  signature[4]      = 0,0,0,0
;   +0x20  ctx_dma            = 4 (PHYS_SYS_NCOH)
;   +0x24  code_dma_base_lo   = booter_addr + img_data_off
;   +0x28  code_dma_base_hi   = 0
;   +0x2C  non_sec_code_off   (from booter hs_load_header)
;   +0x30  non_sec_code_size  (from booter hs_load_header)
;   +0x34  sec_code_off       (from booter hs_load_header)
;   +0x38  sec_code_size      (from booter hs_load_header)
;   +0x3C  code_entry_point   = 0
;   +0x40  data_dma_base_lo   = booter_addr + img_data_off + data_off
;   +0x44  data_dma_base_hi   = 0
;   +0x48  data_size          (from booter hs_load_header)
;   +0x4C  argc               = 0
;   +0x50  argv               = 0

; First, parse booter_load hs_load_header to get BLD field values
; hs_header at booter+0x18, hs_load_header at booter+[hs_hdr+0x1C]

; Read hs_load_header offset
π     θ²ρρ  0x9710
π̄δ̄    θρ²ρ  0x00000034              ; + 0x18 + 0x1C = 0x34
π̄      θ²ρρ  0x96E0
π₇    θ²ρρ·  0x96E0                  ; hs_load_header_offset in file

; hs_load_hdr_addr = booter_addr + hs_load_header_offset
π̄δ̄    θ²ρρ·  0x9710                  ; + booter_addr
π̄      θ²ρρ  0x96E0                   ; save hs_load_hdr_addr

; Read os_code_offset (+0), os_code_size (+4), os_data_offset (+8),
;       os_data_size (+12), num_apps (+16), app[0].offset (+20), app[0].size (+24)
; Store these at scratch 0x9750-0x976C for BLD construction

; os_code_offset (hs_load_hdr + 0)
π₇    θ²ρρ·  0x96E0
π̄₇    θ²ρρ  0x9750                   ; non_sec_code_off

; os_code_size (hs_load_hdr + 4)
π     θ²ρρ  0x96E0
π̄δ̄    θρ²ρ  0x00000004
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x9754                   ; non_sec_code_size

; os_data_offset (hs_load_hdr + 8)
π     θ²ρρ  0x96E0
π̄δ̄    θρ²ρ  0x00000008
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x9758                   ; os_data_offset

; os_data_size (hs_load_hdr + 12)
π     θ²ρρ  0x96E0
π̄δ̄    θρ²ρ  0x0000000C
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x975C                   ; data_size

; app[0].offset (hs_load_hdr + 20)
π     θ²ρρ  0x96E0
π̄δ̄    θρ²ρ  0x00000014
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x9760                   ; sec_code_off

; app[0].size (hs_load_hdr + 24)
π     θ²ρρ  0x96E0
π̄δ̄    θρ²ρ  0x00000018
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
π̄₇    θ²ρρ  0x9764                   ; sec_code_size

; Compute code_dma_base = booter_addr + img_data_offset
; img_data_offset is at booter + 0x10
π     θ²ρρ  0x9710
π̄δ̄    θρ²ρ  0x00000010
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; img_data_offset
π̄δ̄    θ²ρρ·  0x9710                  ; + booter_addr = code_dma_base
π̄      θ²ρρ  0x9768                   ; save code_dma_base

; Compute data_dma_base = code_dma_base + os_data_offset
π₇    θ²ρρ  0x9758                   ; os_data_offset (dword)
π̄δ̄    θ²ρρ·  0x9768                  ; + code_dma_base
π̄      θ²ρρ  0x976C                   ; save data_dma_base

; ── Now write BLD to DMEM via PIO ──

; Reset DMEM port: auto-increment | offset 0
π     θ²ρρ  0x96C0                   ; sec2_base
π̄δ̄    θρ²ρ  0x000001C0
π̄      θ²ρρ  0x96E8                   ; DMEM_PORT addr
π̄₇    θ²²ρ  0x96E0  0x01000000
π₇    θ²ρρ  0x96E0
π̄₇    θ²ρρ·  0x96E8                  ; write port control

; DMEM_DATA address
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x000001C4
π̄      θ²ρρ  0x96D0                   ; DMEM_DATA addr (reuse)

; Write BLD dwords to DMEM_DATA register (auto-increments)
; reserved[0..3] + signature[0..3] = 8 zeros
π̄₇    θ²²ρ  0x96E0  0x00000000
π₇    θ²ρρ  0x96E0                   ; pipeline = 0
π̄₇    θ²ρρ·  0x96D0                  ; reserved[0]
π̄₇    θ²ρρ·  0x96D0                  ; reserved[1]
π̄₇    θ²ρρ·  0x96D0                  ; reserved[2]
π̄₇    θ²ρρ·  0x96D0                  ; reserved[3]
π̄₇    θ²ρρ·  0x96D0                  ; signature[0]
π̄₇    θ²ρρ·  0x96D0                  ; signature[1]
π̄₇    θ²ρρ·  0x96D0                  ; signature[2]
π̄₇    θ²ρρ·  0x96D0                  ; signature[3]

; ctx_dma = 4 (PHYS_SYS_NCOH)
π̄₇    θ²²ρ  0x96E0  0x00000004
π₇    θ²ρρ  0x96E0
π̄₇    θ²ρρ·  0x96D0                  ; ctx_dma

; code_dma_base (u64: low32 then high32)
π₇    θ²ρρ  0x9768                   ; code_dma_base (dword, addr < 4GB)
π̄₇    θ²ρρ·  0x96D0                  ; code_dma_base_lo

π̄₇    θ²²ρ  0x96E0  0x00000000
π₇    θ²ρρ  0x96E0
π̄₇    θ²ρρ·  0x96D0                  ; code_dma_base_hi = 0

; non_sec_code_off
π₇    θ²ρρ  0x9750
π̄₇    θ²ρρ·  0x96D0

; non_sec_code_size
π₇    θ²ρρ  0x9754
π̄₇    θ²ρρ·  0x96D0

; sec_code_off
π₇    θ²ρρ  0x9760
π̄₇    θ²ρρ·  0x96D0

; sec_code_size
π₇    θ²ρρ  0x9764
π̄₇    θ²ρρ·  0x96D0

; code_entry_point = 0
π̄₇    θ²²ρ  0x96E0  0x00000000
π₇    θ²ρρ  0x96E0
π̄₇    θ²ρρ·  0x96D0

; data_dma_base (u64)
π₇    θ²ρρ  0x976C                   ; data_dma_base (dword)
π̄₇    θ²ρρ·  0x96D0                  ; data_dma_base_lo

π̄₇    θ²²ρ  0x96E0  0x00000000
π₇    θ²ρρ  0x96E0
π̄₇    θ²ρρ·  0x96D0                  ; data_dma_base_hi = 0

; data_size
π₇    θ²ρρ  0x975C
π̄₇    θ²ρρ·  0x96D0

; argc = 0, argv = 0
π̄₇    θ²²ρ  0x96E0  0x00000000
π₇    θ²ρρ  0x96E0
π̄₇    θ²ρρ·  0x96D0                  ; argc
π̄₇    θ²ρρ·  0x96D0                  ; argv

; ═══════════════════════════════════════════════════════════════
; PHASE 4: Start SEC2, poll for halt
; ═══════════════════════════════════════════════════════════════

; Write MBOX0 = radix3 level 0 address (0x02800000)
π     θ²ρρ  0x96C0                   ; sec2_base
π̄δ̄    θρ²ρ  0x00000040
π̄      θ²ρρ  0x96E0                   ; MBOX0 addr
π̄₇    θ²²ρ  0x96E8  0x02800000
π₇    θ²ρρ  0x96E8
π̄₇    θ²ρρ·  0x96E0                  ; MBOX0 = 0x02800000

; Write MBOX1 = 0 (address < 4GB)
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000044
π̄      θ²ρρ  0x96E0
π̄₇    θ²²ρ  0x96E8  0x00000000
π₇    θ²ρρ  0x96E8
π̄₇    θ²ρρ·  0x96E0                  ; MBOX1 = 0

; Write boot vector = boot_off
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000104
π̄      θ²ρρ  0x96E0                   ; BOOTVEC addr
π₇    θ²ρρ  0x96D0                   ; boot_off (saved earlier, but may be clobbered)

; Hmm, boot_off was at 0x96D0 but we reused that for DMEM_DATA addr.
; We need to recompute or save it elsewhere. Let me check...
; boot_off was saved at 0x96D0 in step 3b, then 0x96D0 was reused
; for DMEM_DATA addr in step 3d. So boot_off is lost.
;
; Recompute: boot_off = code_limit - bl_code_size
; But code_limit was also in 0x96D8 which was reused.
; This is the single-pipeline scratch gotcha.
;
; Solution: read HWCFG again and recompute.
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000108
π̄      θ²ρρ  0x96E8
π₇    θ²ρρ·  0x96E8
δ₃     θρ²ρ  0x000001FF
δ₇     θρ¹ρ  8                       ; code_limit
π̄      θ²ρρ  0x96E8
π₇    θ²ρρ  0x96E4                   ; bl_code_size (dword, if still valid)

; bl_code_size was at 0x96E4 but that was also reused as dword_count.
; Read it fresh from bl.bin header instead.
π     θ²ρρ  0x9700                   ; bl_addr
π̄δ̄    θρ²ρ  0x0000000C
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; bl_header_offset
π̄δ̄    θ²ρρ·  0x9700                  ; + bl_addr = hdr_addr
π̄δ̄    θρ²ρ  0x0000000C              ; + 0x0C = code_size field
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; bl_code_size
δ₁₃    θρρρ                          ; negate
π̄δ̄    θ²ρρ·  0x96E8                  ; + code_limit = boot_off
π̄₇    θ²ρρ·  0x96E0                  ; write boot_off to BOOTVEC

; Start SEC2: write 2 to CPUCTL
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000100
π̄      θ²ρρ  0x96E0                   ; CPUCTL addr
π̄₇    θ²²ρ  0x96E8  0x00000002
π₇    θ²ρρ  0x96E8
π̄₇    θ²ρρ·  0x96E0                  ; CPUCTL = 2 (start!)

; Poll CPUCTL for halt (bit 4)
: sec2_halt_poll
π₇    θ²ρρ·  0x96E0                  ; read CPUCTL
δ₃     θρ²ρ  0x00000010              ; AND bit 4
skip_nz sec2_halted
loop_back sec2_halt_poll
: sec2_halted

; Check result: MBOX0 should be 0 on success
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000040
π̄      θ²ρρ  0x96E0
π₇    θ²ρρ·  0x96E0                  ; MBOX0 result
δ      θ²ρρ  0x00000000
skip_nz sec2_ok

; SEC2 failed — print error to debugcon
π̄δ    θ¹¹ρ  0xE9  0x53              ; 'S'
π̄δ    θ¹¹ρ  0xE9  0x45              ; 'E'
π̄δ    θ¹¹ρ  0xE9  0x43              ; 'C'
π̄δ    θ¹¹ρ  0xE9  0x32              ; '2'
π̄δ    θ¹¹ρ  0xE9  0x21              ; '!'
π̄δ    θ¹¹ρ  0xE9  0x0A              ; newline
skip_nz gsp_end
skip_z gsp_end

: sec2_ok
; SEC2 booter completed successfully
π̄δ    θ¹¹ρ  0xE9  0x53              ; 'S'
π̄δ    θ¹¹ρ  0xE9  0x45              ; 'E'
π̄δ    θ¹¹ρ  0xE9  0x43              ; 'C'
π̄δ    θ¹¹ρ  0xE9  0x32              ; '2'
π̄δ    θ¹¹ρ  0xE9  0x6F              ; 'o'
π̄δ    θ¹¹ρ  0xE9  0x6B              ; 'k'
π̄δ    θ¹¹ρ  0xE9  0x0A              ; newline

; ═══════════════════════════════════════════════════════════════
; PHASE 5: Start GSP RISC-V [TODO]
; ═══════════════════════════════════════════════════════════════
;
; After SEC2 booter has loaded GSP firmware into VRAM WPR:
;   1. Write libOS init args to GSP MBOX0/MBOX1
;   2. Write app version to GSP falcon
;   3. Start GSP RISC-V: CPUCTL = 2
;   4. Poll for GSP_INIT_DONE on message queue
;
; Requires: WPR metadata (GspFwWprMeta) to be fully populated.
; This is blocked on VRAM size discovery and WPR layout computation.

; ═══════════════════════════════════════════════════════════════
; PHASE 6: RPC communication [TODO]
; ═══════════════════════════════════════════════════════════════
;
; After GSP_INIT_DONE:
;   GET_GSP_STATIC_INFO → display capabilities
;   INTR_GET_KERNEL_TABLE → interrupt vectors
;   Then display programming via RM RPCs
