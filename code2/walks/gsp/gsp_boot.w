; gsp_boot.w — NVIDIA GSP firmware boot sequence
;
; Boots the GPU System Processor on NVIDIA Turing (TU102).
; Called after gpu_nvidia.w has detected GPU, loaded firmware,
; and parsed the pack index into known addresses.
;
; Sub-files:
;   gsp_vram.w  — VRAM discovery + FB layout computation
;   gsp_wpr.w   — RM_RISCV_UCODE_DESC, radix3, WPR metadata
;   gsp_sec2.w  — SEC2 falcon reset + PIO load + start + poll
;
; Firmware addresses (set by gpu_nvidia.w PART 3):
;   [0x9700] bl.bin addr          [0x9708] size
;   [0x9710] booter_load addr     [0x9718] size
;   [0x9720] bootloader addr      [0x9728] size
;   [0x9730] gsp.bin addr         [0x9738] size
;
; GPU info (set by gpu_nvidia.w PART 1/2):
;   [0x9608] BAR0    [0x9610] BAR1    [0x9618] fw RAM base
;
; Computed by gsp_vram.w (VRAM layout at 0x9780-0x97D8):
;   [0x9780] fb_size  [0x9788] vga_ws  [0x9790] frts  [0x9798] boot
;   [0x97A0] elf      [0x97A8] heap    [0x97B0] heap_size
;   [0x97B8] wpr2     [0x97C0] wpr2_size  [0x97C8] nonwpr
;   [0x97D0] fwimage_addr  [0x97D8] fwimage_size
;
; Computed by gsp_wpr.w (RM_RISCV_UCODE_DESC at 0x97E0-0x97F8):
;   [0x97E0-EC] monitorCode/Data/manifestOffset, appVersion
;   [0x97F0] bootloader_img_addr  [0x97F8] bootloader_img_size
;   WPR metadata at 0x2810000, radix3 at 0x2800000
;
; SEC2 registers (BAR0 + 0x840000):
;   +0x040 MBOX0  +0x044 MBOX1  +0x100 CPUCTL  +0x104 BOOTVEC
;   +0x108 HWCFG  +0x180 IMEM_PORT  +0x184 IMEM_DATA  +0x188 IMEM_TAG
;   +0x1C0 DMEM_PORT  +0x1C4 DMEM_DATA  +0x3C0 FALCON_RESET

; ═══════════════════════════════════════════════════════════════
; Skip everything if no NVIDIA GPU or no firmware
; ═══════════════════════════════════════════════════════════════

π     θ²ρρ  0x9600                   ; NVIDIA slot base
δ      θ²ρρ  0x00000000
skip_nz gsp_end                      ; no GPU → skip

π     θ²ρρ  0x9710                   ; booter_load.bin addr
δ      θ²ρρ  0x00000000
skip_nz gsp_end                      ; no firmware → skip

; ═══════════════════════════════════════════════════════════════
; Phase 1: VRAM discovery + FB layout
; ═══════════════════════════════════════════════════════════════
@include gsp_vram.w

; ═══════════════════════════════════════════════════════════════
; Phase 2: Parse bootloader, build radix3 + WPR metadata
; ═══════════════════════════════════════════════════════════════
@include gsp_wpr.w

; ═══════════════════════════════════════════════════════════════
; Phase 3: LibOS args, log buffers, shared memory, RM args
; ═══════════════════════════════════════════════════════════════
@include gsp_libos.w

; ═══════════════════════════════════════════════════════════════
; Phase 4-5: SEC2 falcon reset, PIO load, start, poll
; ═══════════════════════════════════════════════════════════════
@include gsp_sec2.w

; ═══════════════════════════════════════════════════════════════
; Phase 6: Write GSP mailbox, poll for INIT_DONE + sequencer
; ═══════════════════════════════════════════════════════════════
@include gsp_start.w

: gsp_end
