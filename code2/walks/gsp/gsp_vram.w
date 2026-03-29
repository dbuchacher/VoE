; gsp_vram.w — VRAM discovery + FB layout computation
;
; Reads VRAM size from GPU register, computes top-down layout.
; All values stored in scratch 0x9780-0x97DF.
;
; Requires: [0x9608] BAR0, [0x9730] gsp.bin addr, [0x9738] gsp.bin size
;           [0x9720] bootloader.bin addr, [0x9728] bootloader.bin size
;
; Output:
;   [0x9780] fb_size              (u64, total VRAM)
;   [0x9788] vga_workspace_addr   (u64, VRAM)
;   [0x9790] frts_addr            (u64, VRAM)
;   [0x9798] boot_addr            (u64, VRAM, bootloader)
;   [0x97A0] elf_addr             (u64, VRAM, GSP-RM ELF)
;   [0x97A8] heap_addr            (u64, VRAM)
;   [0x97B0] heap_size            (u64)
;   [0x97B8] wpr2_addr            (u64, VRAM)
;   [0x97C0] wpr2_size            (u64)
;   [0x97C8] nonwpr_addr          (u64, VRAM)
;   [0x97D0] fwimage_addr         (u64, system RAM — gsp.bin + 0x40)
;   [0x97D8] fwimage_size         (u64)
;
; Scratch: 0x96C0-0x96FF

; ═══════════════════════════════════════════════════════════════
; VRAM size: BAR0 + 0x100ce0
; ═══════════════════════════════════════════════════════════════

π     θ²ρρ  0x9608                   ; BAR0
π̄δ̄    θρ²ρ  0x00100CE0
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; read VRAM config register

; lmag = (val >> 4) & 0x3F
π̄      θ²ρρ  0x96F8                   ; save raw value
δ̄₇     θρ¹ρ  4                       ; >> 4
δ₃     θρ²ρ  0x0000003F              ; & 0x3F = lmag
π̄      θ²ρρ  0x96E0                   ; save lmag

; lsca = raw & 0x0F
π₇    θ²ρρ  0x96F8                   ; raw value (dword)
δ₃     θρ¹ρ  0x0F                    ; & 0x0F = lsca
π̄δ̄    θρ¹ρ  20                       ; + 20 = shift amount
π̄₇    θ²ρρ  0x96E8                   ; save shift amount (dword)

; fb_size = lmag << (lsca + 20)
; Need variable shift. Walk SHL (δ₇) takes immediate shift.
; Alternative: loop lmag << 1 shift_amount times.
; Or: compute 1 << shift, then multiply... no multiply.
; Simplest: shift lmag left by 20, then shift by lsca.
π₇    θ²ρρ  0x96E0                   ; lmag (dword, fits in 6 bits)
δ₇     θρ¹ρ  20                       ; << 20 = lmag * 1MB

; Now shift left by lsca (0-15). Loop: shift by 1, lsca times.
π̄      θ²ρρ  0x96F0                   ; save partial result

π₇    θ²ρρ  0x96F8                   ; raw value
δ₃     θρ¹ρ  0x0F                    ; lsca
π̄₇    θ²ρρ  0x96E8                   ; lsca counter
δ      θ²ρρ  0x00000000
skip_nz gsp_vram_shift_done          ; if lsca==0, skip loop

: gsp_vram_shift
π     θ²ρρ  0x96F0
δ₇     θρ¹ρ  1                       ; << 1
π̄      θ²ρρ  0x96F0                   ; update

π₇    θ²ρρ  0x96E8
π̄δ̄₃   θρ¹ρ  1
π̄₇    θ²ρρ  0x96E8
skip_z gsp_vram_shift_done
loop_back gsp_vram_shift
: gsp_vram_shift_done

; fb_size at [0x96F0], copy to [0x9780]
π     θ²ρρ  0x96F0
π̄      θ²ρρ  0x9780                   ; fb_size

; ═══════════════════════════════════════════════════════════════
; VGA workspace: fb_size - 1MB (simplified, skip display reg check)
; ═══════════════════════════════════════════════════════════════

π     θ²ρρ  0x9780
π̄δ̄₃   θρ²ρ  0x00100000              ; - 1MB
π̄      θ²ρρ  0x9788                   ; vga_workspace_addr

; ═══════════════════════════════════════════════════════════════
; FRTS: ALIGN_DOWN(vga, 0x20000) - 1MB
; ═══════════════════════════════════════════════════════════════

π     θ²ρρ  0x9788
δ₃     θρ²ρ  0xFFFE0000              ; ALIGN_DOWN(vga, 128K) — low 32 only
; For VRAM > 4GB, this AND only affects low 32 bits. That's fine because
; the alignment is 128KB — only low bits matter.
π̄δ̄₃   θρ²ρ  0x00100000              ; - 1MB = frts_addr
π̄      θ²ρρ  0x9790                   ; frts_addr

; ═══════════════════════════════════════════════════════════════
; Parse bootloader.bin for boot_size
; ═══════════════════════════════════════════════════════════════

; bootloader.bin has nvfw_bin_hdr at offset 0
; data_offset at +0x10, data_size at +0x14
π     θ²ρρ  0x9720                   ; bootloader_addr
π̄δ̄    θρ²ρ  0x00000014
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; bootloader data_size
π̄₇    θ²ρρ  0x96E0                   ; save boot_size (dword)

; boot_addr = ALIGN_DOWN(frts_addr - boot_size, 0x1000)
π₇    θ²ρρ  0x96E0                   ; boot_size (dword)
δ₁₃    θρρρ                          ; negate
π̄δ̄    θ²ρρ·  0x9790                  ; + frts_addr = frts_addr - boot_size
δ₃     θρ²ρ  0xFFFFF000              ; ALIGN_DOWN 4K
π̄      θ²ρρ  0x9798                   ; boot_addr

; ═══════════════════════════════════════════════════════════════
; ELF: elf_addr = ALIGN_DOWN(boot_addr - elf_size, 0x10000)
; ═══════════════════════════════════════════════════════════════

; elf_size = gsp.bin .fwimage size ≈ gsp.bin size (close enough)
; More precisely: gsp.bin has .fwimage at offset 0x40
; fwimage_size ≈ gsp_size - 0x40
π₇    θ²ρρ  0x9738                   ; gsp_size (dword)
π̄δ̄₃   θρ²ρ  0x00000040              ; - 0x40
π̄      θ²ρρ  0x97D8                   ; fwimage_size

; fwimage_addr in system RAM
π     θ²ρρ  0x9730                   ; gsp_addr
π̄δ̄    θρ²ρ  0x00000040
π̄      θ²ρρ  0x97D0                   ; fwimage_addr

; elf_addr = ALIGN_DOWN(boot_addr - fwimage_size, 0x10000)
π     θ²ρρ  0x97D8                   ; fwimage_size
δ₁₃    θρρρ                          ; negate
π̄δ̄    θ²ρρ·  0x9798                  ; + boot_addr
δ₃     θρ²ρ  0xFFFF0000              ; ALIGN_DOWN 64K
π̄      θ²ρρ  0x97A0                   ; elf_addr

; ═══════════════════════════════════════════════════════════════
; Heap: compute heap_size, then heap_addr
; ═══════════════════════════════════════════════════════════════
;
; heap_size = 8MB + ALIGN(96KB * fb_size_gb, 1MB) + 96MB
; 96KB * fb_size_gb = (fb_size >> 14) + (fb_size >> 15)

π     θ²ρρ  0x9780                   ; fb_size
δ̄₇     θρ¹ρ  14                      ; >> 14
π̄      θ²ρρ  0x96F0                   ; save

π     θ²ρρ  0x9780
δ̄₇     θρ¹ρ  15                      ; >> 15
π̄δ̄    θ²ρρ·  0x96F0                  ; + (fb_size >> 14) = fb_size_gb * 96K

; ALIGN to 1MB
π̄δ̄    θρ²ρ  0x000FFFFF              ; + 0xFFFFF
δ₃     θρ²ρ  0xFFF00000              ; & ~0xFFFFF

; + 8MB + 96MB = + 0x6800000
π̄δ̄    θρ²ρ  0x06800000
π̄      θ²ρρ  0x97B0                   ; heap_size

; heap_addr = ALIGN_DOWN(elf_addr - heap_size, 0x100000)
π     θ²ρρ  0x97B0                   ; heap_size
δ₁₃    θρρρ
π̄δ̄    θ²ρρ·  0x97A0                  ; + elf_addr = elf_addr - heap_size
δ₃     θρ²ρ  0xFFF00000              ; ALIGN_DOWN 1MB
π̄      θ²ρρ  0x97A8                   ; heap_addr

; Recompute heap_size for alignment
π     θ²ρρ  0x97A0                   ; elf_addr
π̄      θ²ρρ  0x96F0
π     θ²ρρ  0x97A8                   ; heap_addr
δ₁₃    θρρρ
π̄δ̄    θ²ρρ·  0x96F0                  ; elf_addr - heap_addr
δ₃     θρ²ρ  0xFFF00000              ; ALIGN_DOWN 1MB
π̄      θ²ρρ  0x97B0                   ; updated heap_size

; ═══════════════════════════════════════════════════════════════
; WPR2: wpr2_addr = ALIGN_DOWN(heap_addr - 256, 0x100000)
; ═══════════════════════════════════════════════════════════════

π     θ²ρρ  0x97A8                   ; heap_addr
π̄δ̄₃   θρ²ρ  0x00000100              ; - 256
δ₃     θρ²ρ  0xFFF00000              ; ALIGN_DOWN 1MB
π̄      θ²ρρ  0x97B8                   ; wpr2_addr

; wpr2_size = frts_addr + 0x100000 - wpr2_addr
π     θ²ρρ  0x9790                   ; frts_addr
π̄δ̄    θρ²ρ  0x00100000              ; + frts_size (1MB)
π̄      θ²ρρ  0x96F0
π     θ²ρρ  0x97B8                   ; wpr2_addr
δ₁₃    θρρρ
π̄δ̄    θ²ρρ·  0x96F0                  ; frts_end - wpr2_addr = wpr2_size
π̄      θ²ρρ  0x97C0                   ; wpr2_size

; nonwpr_addr = wpr2_addr - 1MB
π     θ²ρρ  0x97B8
π̄δ̄₃   θρ²ρ  0x00100000
π̄      θ²ρρ  0x97C8                   ; nonwpr_addr
