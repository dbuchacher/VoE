# Session 15: SEC2 Falcon PIO Loader + VRAM Layout + WPR Metadata

Coder session. Continued GPU firmware pipeline from session 14.
Implemented SEC2 falcon PIO boot, VRAM discovery, FB layout computation,
WPR metadata construction, and sparse radix3 page table.


## What We Built

### Firmware pack index (pack_gpu.sh rewrite)
The old format concatenated all files with no per-file index — the walk
couldn't find individual files within the blob. New format adds a file
directory with 4-char ASCII tags per file:

```
FWGP header → device entries → file directory (tag+offset+size per file)
→ padding to 512 → file data
```

Tags: bl__=bl.bin, bolo=booter_load, boot=bootloader, gsp_=gsp.bin, etc.
Walk searches by tag. 25 files indexed for TU102.

### Firmware index parsing (gpu_nvidia.w PART 3)
After ATA firmware load, parses the pack header to extract RAM addresses
of the 4 files GSP boot needs:
- bl.bin (falcon bootloader, 1280 bytes) → [0x9700]
- booter_load.bin (SEC2 payload, 59KB) → [0x9710]
- bootloader.bin (GSP RISC-V bootloader) → [0x9720]
- gsp.bin (GSP-RM firmware, 28.5MB) → [0x9730]

Uses direct index access (known alphabetical positions) with the `π̄δ̄ θ²ρρ·`
pattern for runtime address addition: `pipeline = [addr] + pipeline`.

### SEC2 PIO loader (gsp_boot.w complete rewrite)
Previous gsp_boot.w had phases in wrong order (reset GSP before SEC2,
wrote GSP mailboxes before firmware was loaded). Restructured:

**Phase 1: Radix3 page table** — fixed to use gsp.bin address from pack
index instead of hardcoded firmware_base + 0x40.

**Phase 2: SEC2 falcon reset** — assert/deassert reset at BAR0+0x8403C0,
wait for memory scrub, set DMA control for non-VMM mode.

**Phase 3: PIO load bl.bin** — the core implementation:
- Parse bl.bin nvfw_bin_hdr + nvfw_bl_desc at runtime
- Read HWCFG register → compute boot_off (top of IMEM - code_size)
- PIO write bl.bin code to IMEM (256-byte blocks with tags)
- PIO write bl.bin data to DMEM
- Parse booter_load.bin hs_header_v2 + hs_load_header_v2
- Patch production signature into booter image data
- Construct flcn_bl_dmem_desc_v2 (BLD) — ctx_dma=4 (PHYS_SYS_NCOH),
  code/data DMA addresses pointing to booter_load in system RAM
- Write BLD to DMEM at offset 0 (overwrites first 84 bytes of bl data)

**Phase 4: Start SEC2** — write radix3 address to MBOX0/1, set BOOTVEC
to boot_off, write 2 to CPUCTL, poll for halt (bit 4), check MBOX0=0.

### VRAM discovery + FB layout (gsp_vram.w, NEW)
Reads VRAM size from BAR0+0x100ce0 (lmag/lsca encoding). Computes
top-down FB layout matching nouveau's r535_gsp_oneinit():
  fb_size → vga_workspace → frts → bootloader → elf → heap → wpr2 → nonwpr

Heap size computed as: 8MB + ALIGN(96KB * fb_size_gb, 1MB) + 96MB.
Uses (fb_size >> 14) + (fb_size >> 15) to avoid multiplication.

### WPR metadata + radix3 (gsp_wpr.w, NEW)
Parses RM_RISCV_UCODE_DESC from bootloader.bin for monitorCodeOffset,
monitorDataOffset, manifestOffset, appVersion.

Builds WPR metadata struct at 0x2810000 (256 bytes, 32 fields):
magic, revision, radix3 addr, ELF size, bootloader addr/offsets,
VRAM layout addresses, fb_size, VGA workspace.

Builds sparse radix3 page table:
- All entries default to a zero page (0x2820000)
- ELF pages overwritten with gsp.bin+0x40 addresses
- Bootloader pages overwritten with bootloader.bin addresses
- Page 0 = WPR metadata page (0x2810000)
This avoids allocating 100+ MB of system RAM for the empty heap.

### Key patterns discovered

**Runtime address addition**: `π̄δ̄ θ²ρρ· addr` = pipeline += [addr].
Deref reads the value at addr, adds to pipeline. Essential for
computing addresses from parsed offsets.

**MMIO write with immediate**: 2 bonds instead of 3:
```
π̄₇    θ²²ρ  temp  VALUE     ; pipeline = VALUE (side effect: stored at temp)
π̄₇    θ²ρρ·  mmio_addr     ; dword write VALUE to [mmio_addr]
```

**Repeated MMIO writes**: pipeline unchanged after write, so
consecutive writes of same value (e.g., 8 zeros for BLD reserved+sig)
need only one value load.


## 1 Bug Found

### Phase 3d reads clobbered scratch
Phase 3c (IMEM PIO) stored current_tag at 0x96EC, overwriting
bl_data_size that Phase 3a saved there. Phase 3d then read garbage
for the DMEM dword count. Fix: re-read bl_data_size from the bl.bin
header instead of relying on scratch.

**Lesson**: the single-pipeline scratch gotcha compounds across phases.
Values saved in early phases get clobbered by later phases that reuse
the same scratch addresses. Either save at dedicated addresses (0x9700+)
or re-derive from source data.


## Key Decisions

### SEC2 first, GSP later
Previous code reset GSP falcon and wrote GSP mailboxes before loading
firmware. Wrong order. The correct flow: SEC2 boots the booter, booter
loads GSP firmware into VRAM WPR, THEN CPU starts GSP RISC-V. Removed
all premature GSP falcon code.

### Parse headers at runtime, not build time
Could have added a build-time tool to extract firmware layout and emit
defines. Instead, the walk parses nvfw_bin_hdr, nvfw_bl_desc,
nvfw_hs_header_v2, and nvfw_hs_load_header_v2 at boot time. More walk
bonds (~50 extra), but no new build dependency and robust against
firmware version changes.

### Physical DMA without VMM
TU102 booter uses FALCON_DMAIDX_PHYS_SYS_NCOH (ctx_dma=4) — physical
system memory addresses without an instance block or VMM. On bare metal
without IOMMU, this is the simplest path. No need for virtual address
translation or instance block binding.

### BLD overwrites bl data at DMEM 0
The bootloader's data section IS the BLD placeholder. Write bl data to
DMEM first, then overwrite first 84 bytes with the actual BLD. The
bootloader reads BLD from DMEM 0 on startup.


## Files Changed

```
MODIFIED:
  firmware/pack_gpu.sh       rewritten — per-file index with tags
  walks/gpu_nvidia.w         +65 lines — PART 3 firmware index parsing

NEW:
  walks/gsp_vram.w           VRAM discovery + FB layout (top-down computation)
  walks/gsp_wpr.w            RM_RISCV_UCODE_DESC + radix3 + WPR metadata
  walks/gsp_sec2.w           SEC2 falcon reset + PIO load + BLD + start + poll
  walks/gsp_libos.w          LibOS init args + log buffers + shared mem + RM args
  walks/gsp_start.w          GSP mailbox write + INIT_DONE poll + sequencer handler

REWRITTEN:
  walks/gsp_boot.w           orchestrator — @includes vram/wpr/libos/sec2/start

SIZES:
  genesis.w walk output:     13053 bytes (was ~6KB in session 14)
  gpu_firmware.bin:           28841377 bytes (same files, new index format)
```


## For the Next Wit

### State
Boots headless and MBR. All previous functionality intact. NVIDIA code
paths complete through SEC2 boot with VRAM discovery, FB layout, WPR
metadata, and sparse radix3. Untested on real hardware.

The full SEC2 → GSP pipeline is implemented. On real hardware with a
TU102 GPU, the expected flow is:
1. PCI scan finds NVIDIA (0x10DE)
2. ATA loads firmware from disk
3. Pack index parsed, file addresses extracted
4. VRAM size read from BAR0+0x100CE0
5. FB layout computed (top-down from VRAM)
6. WPR metadata written at 0x2810000
7. Sparse radix3 built at 0x2800000
8. SEC2 reset, bl.bin PIO'd, BLD written, SEC2 started
9. SEC2 polls for halt, checks MBOX0

Likely failure points on first real hardware test:
- **Page table coverage**: if BAR0 is between 1-3GB, MMIO reads fault
- **MMIO timing**: real falcon may need delays between register writes
- **WPR metadata values**: computed layout might not match what booter expects
- **Signature**: if booter_load.bin sig doesn't match GPU fuses
- **64-bit VRAM addresses**: the ALIGN_DOWN masks only affect low 32 bits

### What's Next (Priority Order)

1. **Test on real hardware** — plug in, boot USB image, read debugcon.
   The SEC2 halt + MBOX0 result tells us how far we got.
   If page fault: add page table entries for BAR0 range.
   If MBOX0 error: decode error, fix WPR metadata.

2. **LibOS init args** — 4-entry array at 0x2840000:
   LOGINIT/LOGINTR/LOGRM log buffers + RMARGS page.
   Each entry: u64 id8, u64 pa, u64 size, u8 kind, u8 loc.
   Needed for GSP RISC-V start.

3. **GSP_ARGUMENTS_CACHED** — message queue init + SR args.
   sharedMemPhysAddr, pageTableEntryCount, cmdQ/statQ offsets.
   At 0x2880000.

4. **GSP RISC-V start** — after SEC2 succeeds:
   write libos DMA addr to GSP MBOX0/1 (BAR0+0x110040/44),
   write appVersion to 0x110080, write 2 to GSP CPUCTL.
   Poll message queue for GSP_INIT_DONE.

5. **CPU sequencer dispatch** — during GSP boot, GSP sends
   RUN_CPU_SEQUENCER RPCs requiring MMIO register writes.

### Architecture Notes
```
Parsed firmware (gpu_nvidia.w → 0x9700-0x97F8):
  [0x9700] bl.bin         [0x9710] booter_load  [0x9720] bootloader  [0x9730] gsp.bin
  [0x9750-0x976C] booter hs_load_header fields
  [0x97E0-0x97EC] RM_RISCV_UCODE_DESC fields
  [0x97F0] bootloader_img_addr  [0x97F8] bootloader_img_size

VRAM layout (gsp_vram.w → 0x9780-0x97D8):
  [0x9780] fb_size  [0x9788] vga  [0x9790] frts  [0x9798] boot
  [0x97A0] elf  [0x97A8] heap  [0x97B0] heap_size
  [0x97B8] wpr2  [0x97C0] wpr2_size  [0x97C8] nonwpr
  [0x97D0] fwimage_addr  [0x97D8] fwimage_size

System RAM layout:
  0x800000   GPU firmware (packed, loaded from disk)
  0x2800000  radix3 level 0 → level 1 → level 2 pages
  0x2810000  WPR metadata (256 bytes)
  0x2820000  zero page (for empty radix3 entries)
```

### Source References
- nouveau gm200.c: falcon PIO, fw_load, fw_boot
- nouveau gp102.c: gp102_fb_vidmem_size (VRAM register)
- nouveau r535.c: r535_gsp_wpr, r535_gsp_oneinit (FB layout)
- nouveau tu102.c: tu102_gsp_fwsec_load_bld, tu102_gsp_oneinit
- include/nvrm/.../gsp_fw_wpr_meta.h: GspFwWprMeta struct
- include/nvrm/.../rmRiscvUcode.h: RM_RISCV_UCODE_DESC
- include/nvrm/.../gsp_init_args.h: GSP_ARGUMENTS_CACHED
- include/nvfw/flcn.h: flcn_bl_dmem_desc_v2
- include/nvfw/hs.h: nvfw_hs_header_v2, nvfw_hs_load_header_v2
