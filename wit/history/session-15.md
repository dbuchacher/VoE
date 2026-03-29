# Session 15: Full NVIDIA GSP Boot Pipeline + UEFI Boot + FIRST BARE METAL BOOT

Coder session. Continued GPU firmware pipeline from session 14.
Went from "SEC2 PIO loader skeleton" to "complete GSP boot pipeline"
to "UEFI bootloader" to "FIRST BOOT ON REAL HARDWARE" in one session.

Teal screen on bare metal. TU102 RTX 2080 Ti. Physical pixels.

7 new walk files (13KB), matching nouveau's ~2000-line GSP boot.
UEFI PE32+ stub with GOP framebuffer query. Single-section PE fix
that took 4 iterations to diagnose (cross-section [rel] displacement
error — NASM file offsets vs PE virtual addresses).


## What We Built

### 1. Firmware pack index (pack_gpu.sh rewrite)
Old format concatenated all files with no per-file index. New format:
```
FWGP header → device entries → file directory (tag+offset+size) → data
```
4-char ASCII tags: bl__=bl.bin, bolo=booter_load, boot=bootloader,
gsp_=gsp.bin, etc. 25 files indexed for TU102.

**Gotcha**: bash command substitution strips null bytes. Tags like
"bl\0\0" silently lose the nulls. Fix: use printable padding ("bl__").

### 2. Firmware index parsing (gpu_nvidia.w PART 3)
After ATA firmware load, parses pack header using direct index access
(known alphabetical positions) to extract 4 file addresses:
- bl.bin (1280 bytes) → [0x9700]
- booter_load.bin (59KB) → [0x9710]
- bootloader.bin (4KB) → [0x9720]
- gsp.bin (28.5MB) → [0x9730]

**Key pattern**: `π̄δ̄ θ²ρρ· addr` = runtime add. Reads [addr], adds to
pipeline. This is the backbone of every computed address in the pipeline.

### 3. VRAM discovery + FB layout (gsp_vram.w)
Reads VRAM size from BAR0+0x100ce0:
```
data = read32(BAR0 + 0x100ce0)
lmag = (data >> 4) & 0x3F
lsca = data & 0x0F
fb_size = lmag << (lsca + 20)
```
Variable shift (lsca) implemented as a loop: shift left by 1, lsca times.

Top-down FB layout matching nouveau's r535_gsp_oneinit():
```
fb_size (top)
  vga_workspace = fb_size - 1MB    (simplified, skips display reg check)
  frts = ALIGN_DOWN(vga, 128K) - 1MB
  boot = ALIGN_DOWN(frts - boot_size, 4K)
  elf = ALIGN_DOWN(boot - fwimage_size, 64K)
  heap = ALIGN_DOWN(elf - heap_size, 1MB)
  wpr2 = ALIGN_DOWN(heap - 256, 1MB)
  nonwpr = wpr2 - 1MB
```

Heap size: 8MB + ALIGN(96KB × fb_size_gb, 1MB) + 96MB.
**Multiplication trick**: 96KB × N = (fb_size >> 14) + (fb_size >> 15).
Two shifts and an add instead of a multiply (which walks can't do).

### 4. WPR metadata + radix3 (gsp_wpr.w)
**RM_RISCV_UCODE_DESC parsing** from bootloader.bin: extracts
monitorCodeOffset, monitorDataOffset, manifestOffset, appVersion
by chaining header_offset → desc_addr → field reads.

**WPR metadata** at 0x2810000 (GspFwWprMeta, 256 bytes, 32 fields):
```
+0x00 magic = 0xdc3aae21371a60b3
+0x08 revision = 1
+0x10 sysmemAddrOfRadix3Elf = 0x2800000
+0x18 sizeOfRadix3Elf = fwimage_size
+0x20 sysmemAddrOfBootloader = bootloader_img_addr
+0x28 sizeOfBootloader
+0x30 bootloaderCodeOffset = monitorCodeOffset
+0x38 bootloaderDataOffset = monitorDataOffset
+0x40 bootloaderManifestOffset = manifestOffset
+0x58 gspFwRsvdStart = nonwpr_addr
+0x60 nonWprHeapOffset = nonwpr_addr
+0x68 nonWprHeapSize = 1MB
+0x70 gspFwWprStart = wpr2_addr
+0x78 gspFwHeapOffset = heap_addr
+0x80 gspFwHeapSize
+0x88 gspFwOffset = elf_addr
+0x90 bootBinOffset = boot_addr
+0x98 frtsOffset = frts_addr
+0xA0 frtsSize = 1MB
+0xA8 gspFwWprEnd = ALIGN_DOWN(vga, 128K)
+0xB0 fbSize
+0xB8 vgaWorkspaceOffset
+0xC0 vgaWorkspaceSize
```

**64-bit VRAM address writes**: pipeline holds 64-bit value from qword
read. `π̄₇` writes low 32 bits. For high 32: re-read, `δ̄₇ θρ¹ρ 32`
(shift right 32), write. Two dword writes per u64 field.

**Sparse radix3 page table** at 0x2800000:
1. Fill ALL level-2 entries with a zero page address (0x2820000)
2. Overwrite ELF pages with gsp.bin+0x40 addresses
3. Overwrite bootloader pages with bootloader.bin addresses
4. Set page 0 = WPR metadata page (0x2810000)
5. Build level 1 from level 2 page addresses
6. Level 0 → level 1

This avoids allocating 100+ MB of system RAM for the empty heap.
Only data pages (~7000 for ELF + 1 bootloader + 1 metadata) point to
real data; everything else points to the pre-zeroed page.

### 5. LibOS init args + shared memory (gsp_libos.w)
**Log buffers**: 3 × 64KB at 0x2850000/0x2860000/0x2870000.
Each has internal PTE array at buffer+8 (16 entries, each = buffer + i×4096).
PTE loop: 10 bonds per iteration × 16 iterations × 3 buffers.

**Libos init args**: 4 entries × 64 bytes at 0x2840000:
```
Entry 0: "LOGINIT" (0x4C4F47494E495400) → 0x2850000, 64KB, CONTIGUOUS, SYSMEM
Entry 1: "LOGINTR" (0x4C4F47494E545200) → 0x2860000, 64KB
Entry 2: "LOGRM"   (0x4C4F47524D000000) → 0x2870000, 64KB
Entry 3: "RMARGS"  (0x524D415247530000) → 0x2880000, 4KB
```

**id8 encoding gotcha**: characters packed MSB-first into u64. In
little-endian memory, the dword order is reversed. "LOGINIT":
low dword = 0x4E495400 ("TINI"), high dword = 0x4C4F4749 ("IGOL").

**Shared memory** at 0x2890000:
- PTE array: 129 entries (1 page PTEs + 64 cmdq pages + 64 statq pages)
- Command queue at +0x1000 (256KB, header initialized by CPU)
- Status queue at +0x41000 (256KB, header initialized by GSP)

**Command queue header** (CPU initializes):
```
version=0, size=0x40000, entryOff=0x1000, msgSize=0x1000,
msgCount=63, writePtr=0, flags=1, rxHdrOff=0x20, rx.readPtr=0
```

**GSP_ARGUMENTS_CACHED** at 0x2880000:
```
sharedMemPhysAddr = 0x2890000
pageTableEntryCount = 129
cmdQueueOffset = 0x1000
statQueueOffset = 0x41000
oldLevel=0, flags=0, bInPMTransition=0 (cold boot)
```

### 6. SEC2 PIO loader (gsp_sec2.w)
Restructured from session 14's gsp_boot.w. Previous version had phases
in wrong order (reset GSP before SEC2, wrote GSP mailboxes before
firmware was loaded).

**Phase flow**:
1. Parse bl.bin nvfw_bin_hdr → data_offset, then nvfw_bl_desc → code_off,
   code_size, data_off, data_size
2. Read SEC2 HWCFG → code_limit = (val & 0x1FF) << 8
3. boot_off = code_limit - code_size (bl code goes at top of IMEM)
4. PIO write bl.bin code to IMEM in 256-byte blocks with tags
5. PIO write bl.bin data to DMEM
6. Parse booter_load.bin headers (nvfw_bin_hdr → nvfw_hs_header_v2 →
   nvfw_hs_load_header_v2) for BLD field values
7. Patch production signature into booter image data:
   sig at booter+sig_prod_offset → booter+img_data_offset+[booter+patch_loc]
8. Construct flcn_bl_dmem_desc_v2 (84 bytes) via DMEM PIO:
   ctx_dma=4 (PHYS_SYS_NCOH), code_dma_base/data_dma_base pointing to
   booter_load in system RAM
9. Write radix3 addr to SEC2 MBOX0/1
10. Recompute boot_off (scratch was clobbered), write to BOOTVEC
11. Write 2 to SEC2 CPUCTL → start
12. Poll CPUCTL bit 4 (halt), check MBOX0 == 0

**IMEM PIO protocol** (per 256-byte block):
```
WR32(SEC2 + 0x180, BIT(24) | imem_addr)   ; port control
WR32(SEC2 + 0x188, tag)                    ; block tag
for i in 0..63:                            ; 64 dwords
    WR32(SEC2 + 0x184, data[i])            ; auto-increments
```

**DMEM PIO protocol**:
```
WR32(SEC2 + 0x1C0, BIT(24) | dmem_addr)   ; port control
for i in 0..N:
    WR32(SEC2 + 0x1C4, data[i])            ; auto-increments
```

**BLD overwrites bl data**: bl.bin data (256 bytes) written to DMEM
first, then BLD (84 bytes) overwrites bytes 0-83. The bootloader's
data section IS the BLD placeholder.

### 7. GSP start + sequencer handler (gsp_start.w)
After SEC2 halts, the booter has loaded GSP firmware into VRAM and
started GSP RISC-V in a limited mode. CPU writes libos args address
to GSP falcon mailboxes (BAR0 + 0x110040/44), then polls the message
queue for INIT_DONE.

**Message entry layout** (at msgq + 0x1000 + rptr × 0x1000):
```
+0x00: r535_gsp_msg header (0x30 bytes: auth_tag, aad, checksum, seq, count)
+0x30: nvfw_gsp_rpc header (0x20 bytes)
  +0x3C: function ID (0x1001=INIT_DONE, 0x1002=RUN_CPU_SEQUENCER)
+0x50: RPC payload
  For sequencer: +0x54=cmdIndex, +0x78=commandBuffer[]
```

**Poll loop**: read msgq tx.writePtr at msgq+0x14, compare to our rptr
(stored at cmdq+0x20). If different, read entry, dispatch by function ID.
After processing, advance rptr modulo 63, write to cmdq rx.readPtr.

**CPU sequencer** (opcodes 0-8):
| Opcode | Name | Payload (dwords) | Implementation |
|--------|------|-----------------|----------------|
| 0 | REG_WRITE | addr, val (2) | write val to BAR0+addr |
| 1 | REG_MODIFY | addr, mask, val (3) | **STUBBED** — can't AND two runtime values |
| 2 | REG_POLL | addr, mask, val, timeout, error (5) | Fixed-iteration wait (no runtime mask) |
| 3 | DELAY_US | val (1) | Spin loop |
| 4 | REG_STORE | addr, slot (2) | Skipped (saves to regSaveArea) |
| 5 | CORE_RESET | none (0) | Reset GSP falcon (0x3C0 bit 0) + DMA ctrl + scrub |
| 6 | CORE_START | none (0) | Check CPUCTL bit 6 → write 2 to CPUCTL or CPUCTL_ALIAS |
| 7 | CORE_WAIT_HALT | none (0) | Poll CPUCTL bit 4 |
| 8 | CORE_RESUME | none (0) | Skipped |

**Command buffer format**: each command = opcode (1 dword) + payload
(variable dwords). `ptr += 1 + payload_size_dwords(opcode)`.

**Debug characters** printed to debugcon during boot:
- 's' = sequencer received
- 'R' = core reset
- 'G' = core start
- '?' = unknown message
- "SEC2ok" / "SEC2!" = SEC2 result
- "GSPok" = INIT_DONE received


### 8. UEFI bootloader (uefi.asm, NEW)
MBR boot failed on real hardware: BIOS jumped to 07C0:0000 instead of
0000:7C00 (fixed with far jump normalization from GRUB), then the
32-bit protected mode transition crashed. Switched to UEFI.

UEFI gives us 64-bit mode, GOP framebuffer, and proper memory mapping.
The stub:
1. Queries GOP via LocateProtocol → gets framebuffer base, resolution, pitch
2. Stores FB info at 0x9100 (same format as MBR path — genesis doesn't care)
3. Gets memory map → ExitBootServices (with retry loop for stale MapKey)
4. Copies embedded kernel ELF LOAD segments to physical addresses (0x100000+)
5. Sets up 1GB huge page tables (0-4GB identity mapped — covers ALL BAR0 ranges)
6. Masks PIC IRQs, copies AP trampoline, builds GDT
7. Jumps directly to long_mode (skipping x86.asm's 32-bit _start)

**Critical fix: single-section PE.** The original two-section PE (.text + .data)
had broken [rel] addressing. NASM computes RIP-relative displacements using
file offsets, but UEFI maps PE sections to virtual addresses (RVAs). When
sections have different file-to-RVA offsets, every cross-section [rel] reference
points to the wrong address. QEMU's OVMF hid this by loading the file linearly.
Real hardware exposed it immediately (black screen, hang).

Fix: put ALL code, data, and the embedded kernel in ONE .text section.
No cross-section references = no displacement error. Works on both
QEMU OVMF and real hardware.

**Build: `bash build/run uefi`** (QEMU with OVMF) or **`bash build/run uefi-img`**
(FAT32 ESP image for USB). Kernel symbols extracted via `nm` at build time
(LONG_MODE_ADDR, TRAMP_CODE, TRAMP_END, GDTR_ADDR).

### 9. FIRST BOOT ON REAL HARDWARE

Teal screen. RTX 2080 Ti. Physical pixels on a physical monitor.

Boot chain: UEFI firmware → BOOTX64.EFI → GOP framebuffer →
ExitBootServices → ELF copy → page tables → long_mode → BSS zero →
AP wake → lattice_start → JIT init → walker runs genesis → teal fill.

4 cores awake. 1GB huge pages covering 0-4GB. BAR0 at 0xFA000000 mapped.
No GPU firmware on ESP yet, so NVIDIA GSP walks skip. But PCI scan
finds the TU102 (vendor 0x10DE). The full pipeline is ready — just
needs firmware files on the ESP.


## Bugs Found

### 1. Phase 3d reads clobbered scratch
Phase 3c (IMEM PIO) stored current_tag at 0x96EC, overwriting
bl_data_size from Phase 3a. Fix: re-read from bl.bin header.

**Lesson**: single-pipeline scratch clobber compounds across phases.
Save persistent values at dedicated addresses (0x9700+) or re-derive.

### 2. Null bytes in bash command substitution
`TAG=$(file_tag "bl")` where file_tag outputs `printf 'bl\x00\x00'`
strips the nulls silently. The packed firmware header had corrupt tags.
Fix: use printable padding ("bl__" instead of "bl\0\0").


## Key Decisions

### SEC2 first, GSP later
Previous code (session 14's gsp_boot.w) reset GSP falcon and wrote GSP
mailboxes before loading firmware. Wrong order. Correct flow:
SEC2 boots booter → booter loads GSP into VRAM → CPU writes libos to GSP
mailbox → GSP sends sequencer commands → CPU executes → GSP sends INIT_DONE.

### Parse headers at runtime
Walk parses nvfw_bin_hdr, nvfw_bl_desc, nvfw_hs_header_v2,
nvfw_hs_load_header_v2, RM_RISCV_UCODE_DESC at boot time. ~50 extra bonds
but robust against firmware version changes. No build-time parser needed.

### Physical DMA without VMM
BLD ctx_dma=4 (FALCON_DMAIDX_PHYS_SYS_NCOH). Physical system memory
addresses without instance block binding. Simplest path on bare metal.

### Sparse radix3
Only ~7003 pages of real data (ELF + bootloader + metadata). WPR2 total
could be ~25000 pages. Empty entries point to one pre-zeroed page instead
of allocating 100+ MB. Memory-efficient.

### Multiplication via bit shifts
96KB × N = (fb_size >> 14) + (fb_size >> 15). Walks have no multiply bond.
Shift-and-add decomposes 96K = 64K + 32K into two power-of-two shifts.

### Skip REG_MODIFY for initial bring-up
Single-pipeline can't AND two runtime values (no deref-AND bond).
REG_MODIFY needs (read & ~mask) | val — three runtime values combined.
Stubbed for now. Most cold boot sequencer commands are REG_WRITE and
CORE_*. If a REG_MODIFY is sent and we skip it, GSP may hang.

**Future fix options**:
1. Add a runtime AND bond to the walker (δ₃ with deref arg)
2. Compute the AND via XOR/OR/complement chain (4-5 extra bonds)
3. Use a scratch-based bit manipulator (loop over 32 bits)


## Limitations / Known Issues

- **REG_MODIFY stubbed**: skips 3-dword payload, doesn't execute RMW
- **REG_POLL heuristic**: fixed-iteration wait, no (read & mask) == val check
- **No timeout**: poll loops spin forever on failure (SEC2 halt, INIT_DONE)
- **VGA workspace simplified**: uses fb_size - 1MB, doesn't read BAR0+0x625f04
- **ALIGN_DOWN on 64-bit**: mask only affects low 32 bits. For VRAM > 4GB,
  alignment that crosses the 4GB boundary would be wrong. Fine for offsets
  within the WPR2 region (always < 4GB from top of VRAM).
- **Page table coverage**: pd0 maps 0-1GB, pd3 maps 3-4GB. If GPU BAR0
  is between 1-3GB, MMIO reads page fault.
- **REG_STORE ignored**: doesn't save register values (regSaveArea slots).
  If a later command references a stored value, it'll read 0.


## Files Changed

```
MODIFIED:
  firmware/pack_gpu.sh         rewritten — per-file index with tags
  walks/gpu_nvidia.w           +65 lines — PART 3 firmware index parsing
  legacy/x86.asm               global long_mode, debug marker
  legacy/boot.asm              GRUB-style fixes (far jmp, LBA check, INT 10h)
  build/run                    +uefi/uefi-img modes, nm symbol extraction

NEW:
  legacy/uefi.asm              UEFI PE32+ stub — GOP + ExitBS + ELF copy + long_mode jump

NEW (walks/gsp/):
  gsp_boot.w                   orchestrator — skip guards + 6 @includes
  gsp_vram.w                   VRAM discovery + FB layout computation
  gsp_wpr.w                    RM_RISCV_UCODE_DESC + radix3 + WPR metadata
  gsp_libos.w                  LibOS args + log buffers + shared mem + RM args
  gsp_sec2.w                   SEC2 falcon reset + PIO load + BLD + start + poll
  gsp_start.w                  GSP mailbox + INIT_DONE poll + sequencer handler

SIZES:
  genesis.w walk output:       13053 bytes (was ~6KB in session 14)
  BOOTX64.EFI:                ~106KB (kernel embedded)
  gpu_firmware.bin:             28841377 bytes (same files, new index format)
```


## Memory Map

### System RAM (used by GSP boot)
```
0x0800000  GPU firmware (packed, loaded from disk by ATA walk)
0x2800000  radix3 level 0 page (8 bytes used)
0x2801000  radix3 level 1 page (up to 512 entries)
0x2802000  radix3 level 2 pages (up to ~50 pages)
0x2810000  WPR metadata (GspFwWprMeta, 256 bytes)
0x2820000  zero page (all empty radix3 entries point here)
0x2840000  libos init args (4 × 64 bytes)
0x2850000  LOGINIT log buffer (64KB, PTEs at +8)
0x2860000  LOGINTR log buffer (64KB)
0x2870000  LOGRM log buffer (64KB)
0x2880000  GSP_ARGUMENTS_CACHED (4KB)
0x2890000  shared memory:
  +0x0000  PTE array (4KB, 129 entries)
  +0x1000  command queue (256KB, CPU→GSP, header init'd by CPU)
  +0x41000 status queue (256KB, GSP→CPU, header init'd by GSP)
0x2911000  end of shared memory
```

### Scratch Memory (0x9000-0x9800 region)
```
0x9100     VESA FB info (from MBR)
0x9200     loop_region base
0x9300     CPU firmware: LBA, sectors, RAM addr, size
0x9318     GPU firmware: LBA, sectors
0x9400+    PCI results (virtio)
0x9500+    virtqueue info
0x9560     display dimensions
0x9600     NVIDIA: slot, BAR0, BAR1, fw_addr, fw_size
0x96C0-FF  per-phase scratch (clobbered freely between phases)
0x9700     bl.bin: addr, size
0x9710     booter_load: addr, size
0x9720     bootloader: addr, size
0x9730     gsp.bin: addr, size
0x9750-6C  booter hs_load_header fields (for BLD)
0x9768     code_dma_base, data_dma_base
0x9780     fb_size (VRAM total)
0x9788     vga_workspace_addr
0x9790     frts_addr
0x9798     boot_addr (bootloader in VRAM)
0x97A0     elf_addr (GSP-RM ELF in VRAM)
0x97A8     heap_addr
0x97B0     heap_size
0x97B8     wpr2_addr
0x97C0     wpr2_size
0x97C8     nonwpr_addr
0x97D0     fwimage_addr (system RAM)
0x97D8     fwimage_size
0x97E0     monitorCodeOffset
0x97E4     monitorDataOffset
0x97E8     manifestOffset
0x97EC     appVersion
0x97F0     bootloader_img_addr
0x97F8     bootloader_img_size
```

### NVIDIA Register Map (BAR0 offsets)
```
SEC2 falcon (BAR0 + 0x840000):
  +0x040 MBOX0          +0x044 MBOX1
  +0x100 CPUCTL          +0x104 BOOTVEC
  +0x108 HWCFG           +0x10C MEM_SCRUB
  +0x180 IMEM_PORT0      +0x184 IMEM_DATA0      +0x188 IMEM_TAG0
  +0x1C0 DMEM_PORT0      +0x1C4 DMEM_DATA0
  +0x3C0 FALCON_RESET    +0x624 DMA_CTRL

GSP falcon (BAR0 + 0x110000):
  +0x040 MBOX0 (libos args addr low)
  +0x044 MBOX1 (libos args addr high)
  +0x080 APP_VERSION
  +0x100 CPUCTL           +0x104 BOOTVEC
  +0x10C MEM_SCRUB        +0x130 CPUCTL_ALIAS
  +0x3C0 FALCON_RESET     +0x624 DMA_CTRL

Other:
  0x100CE0  VRAM size (lmag/lsca)
  0x625F04  VGA workspace (display, not read — simplified)
```


## For the Next Wit

### State
Complete GSP boot pipeline implemented. Boots in QEMU (headless + GUI
with teal screen). NVIDIA code paths untested — no NVIDIA GPU in QEMU.

### What's Next

1. **Test on real hardware** — `bash build/run img` → dd to USB → boot.
   Read debugcon for progress. Expected: SEC2ok → sRG → GSPok.
   Likely first failure: page fault (BAR0 unmapped) or WPR metadata
   values wrong (SEC2 returns error in MBOX0).

2. **Fix REG_MODIFY** — needed if sequencer sends one during cold boot.
   Options: add deref-AND to walker, or multi-step XOR/complement chain.

3. **Fix REG_POLL** — needs proper (read & mask) == val check.
   Same runtime-AND limitation as REG_MODIFY.

4. **Keyboard walk** — PS/2 poll → wave byte → input loop.
   Independent of GPU. Can do in parallel with GPU debugging.

5. **Loop allocation in genesis** — runtime loop headers at 0x400000+.
   Needed for keyboard input loop and display output loop.

### Source References
```
nouveau falcon PIO:     nvkm/falcon/gm200.c, v1.c
nouveau VRAM discovery: nvkm/subdev/fb/gp102.c (gp102_fb_vidmem_size)
nouveau GSP boot:       nvkm/subdev/gsp/r535.c (r535_gsp_wpr, r535_gsp_oneinit)
nouveau TU102 init:     nvkm/subdev/gsp/tu102.c
WPR metadata struct:    include/nvrm/.../gsp_fw_wpr_meta.h
RM_RISCV_UCODE_DESC:   include/nvrm/.../rmRiscvUcode.h
GSP init args:          include/nvrm/.../gsp_init_args.h
Sequencer opcodes:      include/nvrm/.../rmgspseq.h
BLD struct:             include/nvfw/flcn.h (flcn_bl_dmem_desc_v2)
HS firmware headers:    include/nvfw/hs.h, fw.h
RPC function IDs:       include/nvrm/.../rpc_global_enums.h
Message queue format:   r535.c (r535_gsp_msg, r535_gsp_msgq_wait, r535_gsp_cmdq_push)
```
