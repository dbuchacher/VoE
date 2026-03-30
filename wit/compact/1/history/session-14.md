# Session 14: GPU Firmware Pipeline — From Virtio to NVIDIA

Coder session. Started with "we need to deal with GPU firmware NOW."
Ended with a 33MB bootable USB image carrying CPU microcode for 90%
of modern x86 PCs and NVIDIA TU102 GPU firmware, rendering teal
pixels via virtio-GPU, with the GSP boot sequence half-written.

Biggest session yet. 7 bugs found. 6 new substrate bonds. 8 walk
files. One lattice decomposition that predicted the entire design.


## What We Built

### Substrate (x86.asm) — 6 new bonds
- Dword port I/O: π₇δ̄/π̄₇δ (`in eax, dx` / `out dx, eax`) — PCI config
- Word read/write: π₅/π̄₅ — virtio registers need 16-bit MMIO
- MSR access: πδ̄₃/π̄δ₃ (`rdmsr`/`wrmsr`) — CPU microcode
- CPUID: πδ̄₅ — CPU identification
- Port bulk read: π∮δ̄ (`rep insd`) — ATA sector transfer

Width is on τ (|τ|=1→qword, 3→byte, 5→word, 7→dword).
Register file is on φ (|φ|=1→port, 3→MSR, 5→CPUID).
Both derived from the lattice — asked "what IS this?" not "what should we add?"

### hodos (walk assembler)
- @include directive (recursive, relative paths)
- New θ patterns: θ³ρρ, θ³²ρ, θ²¹ρ, θ¹¹¹, θ²²², etc.
- All new bond symbols for width/register variants

### Walk Files (8 new)
| File | Lines | What |
|------|-------|------|
| walks/pci.w | ~180 | PCI bus scan + virtio capability walking + BAR resolution |
| walks/virtio.w | ~170 | Virtio modern transport: reset → DRIVER_OK, virtqueue |
| walks/gpu.w | ~400 | 6 virtio-GPU commands: display_info through flush |
| walks/ata.w | ~100 | ATA PIO disk read (28-bit LBA, rep insd) |
| walks/ucode.w | ~220 | CPU microcode: CPUID → firmware index scan → wrmsr |
| walks/gpu_nvidia.w | ~150 | NVIDIA PCI detect + BAR map + ATA firmware load |
| walks/gsp_boot.w | ~230 | GSP boot skeleton: radix3 + falcon reset + mailboxes |

### Firmware Infrastructure
- firmware/cpu/ — 34 blobs: AMD Zen 1-5 + Intel 6th-15th gen (4.8MB packed)
- firmware/gpu/nvidia-tu102/ — 24 files: GSP, booter, bootloader, engine fw (28.8MB)
- firmware/pack.sh — CPU microcode packer with indexed binary format
- firmware/pack_gpu.sh — GPU firmware packer per vendor:device
- Build script: dual firmware regions on disk image, sector location defines

### The 33MB Bootable Image
```
sector 0:        MBR bootloader (512 bytes)
sectors 1-153:   kernel with all walks (87KB)
sectors 256-9650: CPU microcode (4.8MB, 34 blobs)
sectors 9651+:   NVIDIA TU102 GPU firmware (28.8MB)
```
`dd if=.build/nomos.img of=/dev/sdX bs=512` → boots on real hardware.


## The Lattice Decomposition That Predicted Everything

Before writing any code, we asked "what IS each task on the lattice?"

| Task | Bond(s) | What it IS |
|------|---------|------------|
| Firmware pre-load | π∮ (slurp) | Bulk copy — same as kernel load |
| PCI scan | π∮δ (scan) | Running accumulation through slots |
| PCI cap walk | ∮δ (take_while) + πδ (filter) | Scan linked list |
| Virtio init | linear π̄ + δ | Short polynomial, no loop |
| Virtqueue | loop_write + loop_read | IS the loop primitive |
| GPU commands | loop_write → π̄ → loop_read | Loop record submission |
| Firmware DMA | π∮ (slurp) + ∮δ (poll) | Bulk copy + wait |
| CPU microcode | π → wrmsr | Read + system register write |

This predicted: port I/O width goes on τ (read force magnitude).
Register file goes on φ (test/guard force magnitude). MSR and CPUID
are magnitude extensions of port I/O. No new forces needed — just
magnitude variants of existing bonds.

The virtqueue IS two loops (available + used ring). The GPU command
pattern IS loop_write → notify → loop_read. The message queue for
NVIDIA GSP IS the same loop primitive. Everything mapped.


## 7 Bugs Found

### 1. π̄₇/π̄₃ wave byte encoding (hodos)
hodos encoded write-extended as 0xC0 (τ=-1, shell 1) instead of
0x80 (τ=extended). The extension byte carried the sign, but the
wave byte said "not extended." Every dword memory write crashed.

### 2. loop_back off-by-5 (walker)
Walker did `sub r12, rbx; jmp .next` — skipping the +5 adjustment
that skip_z/skip_nz use via .skip_done. Every loop rewound 5 bytes
into the previous instruction's inline data. All walks with loops
were broken. Fix: `jmp .skip_done` instead of `jmp .next`.

### 3. VGA memory aperture (0xA0000-0xBFFFF)
Initial virtqueue + framebuffer at 0xA0000 hit the legacy VGA
window. `rep stosd` to VGA memory went through the VGA MMIO handler
instead of RAM — extremely slow. Moved everything to 0x400000+.

### 4. Port address truncation (walks)
ATA ports (0x1F0-0x1F7 = 496-503) don't fit in u8 (max 255).
Using θ¹ρρ (arg0=u8) truncated 0x1F7 to 0xF7. The ATA BSY poll
read from port 0xF7 (nothing there) instead of 0x1F7 (status reg).
Fix: use θ²ρρ (arg0=u32) for all port numbers > 0xFF.

### 5. Qword overlap at 0x9308/0x930C
ata.w wrote firmware size as dword at 0x930C. But firmware address
was a qword at 0x9308 (8 bytes: 0x9308-0x930F). The dword write
clobbered the upper 4 bytes, turning 0x0000000000600000 into
0x0000040000600000. The indirect read then page-faulted at 4GB+.
Fix: moved fw_size to 0x9310.

### 6. 32-bit ELF relocation in 64-bit code
Using a data section symbol (`fw_start_sector`) from 64-bit code
compiled as elf32 caused bad relocations. The `mov eax, [fw_start_sector]`
generated a 32-bit relocation that didn't work in long mode.
Fix: use NASM defines (compile-time constants) instead of symbols.

### 7. Build script DBGFLAG stripping
`img` mode sets DBGFLAG="" to strip debug output for USB images.
The MBR without -DDEBUG has no debugcon prints. Looked like a crash
(no output) but was actually running fine. Wasted 30 minutes.


## Lessons Learned

### Ask the lattice first
Every design question we answered by asking "what IS this on the
lattice?" worked. Width on τ, register file on φ, port bulk as π∮δ̄.
The lattice predicted the right design before we wrote a single line.
When we guessed instead of computing, we got it wrong.

### The single-pipeline gotcha
Walk code has one accumulator (pipeline). Every debug print, every
test, every port_write clears or overwrites it. Values that need to
survive must be explicitly saved to scratch memory. This caused most
of the debugging time. The pattern is: save before, restore after.

### Deref loads qwords
The · (deref) modifier does `mov rdi, [addr]` — always 64 bits.
If you wrote a dword to that address, the upper 4 bytes might be
garbage from a previous walk. Either write qwords (π̄ not π̄₇) to
scratch that gets deref'd, or ensure 8-byte separation.

### loop_back needs non-zero pipeline
`loop_back` fires when pipeline ≠ 0. After a `δ` (test), pipeline
is 0 or 1 — not the original value. If the test's 0 result means
"keep looping," you must reload a non-zero value before loop_back.

### Check the Linux source
For hardware register maps, the kernel source is right there. The
NVIDIA GSP research agent read nouveau's r535.c and extracted every
register offset, every data structure, every boot step in one pass.
Don't guess at hardware — read the driver that already works.

### Don't assume the MBR is broken
When debugcon shows nothing, check if -DDEBUG is defined. The MBR
strips debug output in img mode. The OS might be running fine.


## Key Decisions

### Width encoding: τ magnitude
|τ|=1→qword, 3→byte, 5→word, 7→dword. Consistent across memory
read/write and port I/O. The magnitude of the read force = scope
of observation. Port I/O and memory are orthogonal (φ vs no φ).

### Register file: φ magnitude
|φ|=1→port I/O, |φ|=3→MSR, |φ|=5→CPUID. Same bond pattern (πδ̄),
different register file selected by magnitude. No new force needed.

### Firmware on disk, not embedded
4.8MB CPU + 28.8MB GPU firmware = too big for the kernel image.
Firmware lives on disk sectors after the kernel. ATA PIO walk reads
it into RAM. Build script manages sector offsets via NASM defines.

### Fixed FWSTART at sector 256
Avoids chicken-and-egg: FWSTART depends on kernel size which isn't
known until after assembly. Fixed at 256 (128KB reserved for kernel).
Build script validates kernel fits.

### QEMU virtio-gpu needs a display backend
`-display none` prevents virtio-gpu from processing commands. GPU
walks hang at the first poll. Headless testing uses multiboot without
virtio-gpu (walks skip via 0x9400 check). GUI testing needs -vga std.

### NVIDIA GSP message queue = loop primitive
The CPU↔GSP communication uses a 256K command queue + 256K status
queue, each with write/read pointers and 4K entries. This IS the
loop header. The GPU's ring buffer protocol maps directly to our
loop infrastructure.


## Files Changed

```
NEW:
  walks/pci.w              PCI bus scan + cap walking
  walks/virtio.w           virtio modern transport init
  walks/gpu.w              6 virtio-GPU commands
  walks/ata.w              ATA PIO disk read
  walks/ucode.w            CPU microcode application
  walks/gpu_nvidia.w       NVIDIA GPU detect + fw load
  walks/gsp_boot.w         GSP boot skeleton
  firmware/pack.sh         CPU microcode packer
  firmware/pack_gpu.sh     GPU firmware packer
  firmware/cpu/             34 microcode blobs (AMD + Intel)
  firmware/gpu/nvidia-tu102/ 24 firmware files (GSP, booter, etc.)
  firmware/test_firmware.bin tiny test blob for debugging

MODIFIED:
  legacy/x86.asm           6 new bonds, CPUID, u16 r/w, loop_back fix,
                           π̄₇/π̄₃ encoding fix, port bulk read,
                           FWSTART/FWSECTORS/GPUSTART/GPUSECTORS defines
  build/hodos.c            @include, new θ patterns, new bond symbols,
                           π̄₇/π̄₃ encoding fix
  build/run                firmware packing, dual fw regions, sector
                           management, headless vs MBR path fixes
  genesis.w                includes all walks, display + firmware +
                           PCI + virtio + GPU + NVIDIA pipeline
```


## For the Next Wit

### State
Boots headless (multiboot) and MBR (USB). Teal screen via virtio-GPU
in GUI mode. CPU microcode scans index and applies (tested: AMD fam17h
entry found, wrmsr path exercised on matching hardware). NVIDIA GPU
detected + firmware loaded from disk. GSP boot skeleton in place.

### What's Next (Priority Order)

1. **GSP falcon PIO loader** — write booter_load code to SEC2 IMEM.
   Parse the booter binary header (RM_RISCV_UCODE_DESC). Write code
   words via PIO to BAR0+0x840180/184/188. Start SEC2 via CPUCTL.
   Register offsets are documented in gsp_boot.w comments.

2. **GSP message queue** — implement the ring buffer poll/send.
   The queue format is documented (msgqTxHeader). CPU reads writePtr,
   compares to readPtr, reads 4K message entries. This is loop_read.

3. **CPU sequencer** — during GSP boot, the GPU sends back
   RUN_CPU_SEQUENCER RPCs telling the CPU to do register writes.
   Must handle: CORE_RESET, CORE_START, CORE_WAIT_FOR_HALT,
   CORE_RESUME. Each is a few MMIO writes.

4. **Keyboard walk** — PS/2 poll (port 0x60) → wave byte → input loop.
   Needs loop infrastructure in genesis (runtime loop allocation).

5. **Loop allocation in genesis** — runtime loop headers at 0x400000+.
   Each loop: write cursor, read cursor, buffer, depth, mask, record_size.
   Genesis allocates N loops, stores pointers.

### Architecture Notes
```
Known addresses:
  0x9100     VESA FB info (from MBR)
  0x9200     loop_region base
  0x9300     CPU firmware LBA start
  0x9304     CPU firmware sector count
  0x9308     CPU firmware RAM address (set by ATA walk)
  0x9310     CPU firmware size (set by ATA walk)
  0x9318     GPU firmware LBA start
  0x931C     GPU firmware sector count
  0x9400+    PCI results (slot base, BAR0, virtio caps)
  0x9440-9458 virtio MMIO addresses (common, notify, device, ISR)
  0x9500-9530 virtqueue info (desc, avail, used, doorbell)
  0x9560     display width/height
  0x9600+    NVIDIA GPU info (slot, BAR0, BAR1, fw addr)
  0x9680+    NVIDIA scan scratch
  0x96C0+    GSP boot scratch

Memory layout:
  0x100000 (1MB)   kernel
  0x400000 (4MB)   virtqueue (desc + avail + used)
  0x500000 (5MB)   virtio-GPU framebuffer
  0x600000 (6MB)   CPU firmware (loaded by ATA)
  0x800000 (8MB)   GPU firmware (loaded by ATA)
  0x2800000 (40MB) GSP page tables + metadata + queues
```

### Critical Source References
- nouveau r535.c: GSP boot sequence, RPC protocol, message queues
- nouveau tu102.c: TU102-specific falcon config
- nouveau gm200.c: falcon fw_boot PIO protocol
- All at /usr/src/linux-6.12.58-gentoo/drivers/gpu/drm/nouveau/
