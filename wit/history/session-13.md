# Session 13: Symbol Vocabulary + All 16 Bonds + hodos

Coder session. Started with boot.asm, ended with a walk assembler
and the lattice speaking its own language.


## What We Built

### code2/ substrate
- [1] boot.asm: MBR, VESA enumerate, teal screen fill
- loop.inc: zero-copy loop macros (per-loop scratch, no global stash)
- All 16 bond patterns implemented (was 7 real + 9 stubs)
  - bind (∘∮δ) was stubbed — THIS is why previous wits couldn't write
    the drain loop as a bond. The primitive didn't exist.
- Tested: loop write/read/dereference, bind loops 3×

### Symbol vocabulary (the big one)
Discovered NASM accepts UTF-8 labels. The entire codebase now speaks math:

**Dimensions:** T,D,M,Q → τ,χ,μ,φ (proper time, spatial, mass, electric potential)
**Forces:** P,C,R,W → π,∘,∮,δ (projection, composition, contour integral, Kronecker delta)
**Inverse:** +/- → overbar (π̄ = write = antimatter of read. Standard physics notation)
**System:** ƒ (equation), ψ (walker), Λ (lattice start), ρ (result), γ (genesis)
**Evaluators:** ∏ (monomial), λ (fold), ν (fix), Φ (foldl), δ (maybe), σ (map),
  ω (until), Σ (scan), β (bind), η (hylo)
**Args:** θ constants (θ¹¹ρ = shell 1, shell 1, pipeline). No brackets.
**Formula:** ƒ(τ,χ,μ,φ) = τᵃ·χᵇ·μᶜ·φᵈ (real Unicode superscripts)
**Bond macros:** π̄δ replaces w -1,0,0,+1. The symbol IS the instruction.

### hodos — the walk assembler
325 lines of C. Assembles .w files (bond symbols + θ constants) into raw
wave bytes. Byte-identical output to NASM. Labels, skip offsets, all work.

Named after ὁδός (hodos): the path. Creates the directions ψ walks.

### Structure cleanup
- 4 substrate files merged → legacy/x86.asm (one file, sealed)
- walk.inc, loop.inc, genesis.asm dissolved → genesis.w + hodos
- .w syntax highlighting for neovim (bond=keyword, θ=type, numbers=purple)

### Semantic primes discovery
65 NSM primes mapped to lattice coordinates. Three-layer seed model:
  ~20 image schemas (innate) = ~16 bond patterns
  65 primes (learned in year 1) = first linguistic encoding of schemas
  all other words = memetic, context-learned

The bond dispatch table IS the infant concept table.
Draft: wit/drafts/semantic-primes.md


## Files Changed

```
code2/
  genesis.w              NEW — first .w file (hodos notation)
  build/hodos.c          NEW — walk assembler (325 lines)
  build/run              updated — hodos pipeline, auto-timeout
  build/link.ld          unchanged
  legacy/x86.asm         MERGED from cpu+handoff+nomos+walker
                         all 16 bonds, symbol vocabulary, incbin genesis.bin
  legacy/boot.asm        RENAMED from [1] boot.asm

DELETED: [0] walk.inc, [6] genesis.asm, loop.inc (dissolved into hodos)
DELETED: x86/[2-5] separate files (merged into x86.asm)

wit/mind/mind            force notation π∘∮δ, dimensions τχμφ
wit/mind/coder/design    force notation + dimensions updated
wit/mind/coder/guide     force notation + dimensions + bond table updated
wit/drafts/semantic-primes.md  NEW — primes + schemas + structural rules

.editor/syntax/walk.vim  NEW — nvim syntax highlighting for .w files
.nvim.lua                NEW — project-local nvim config
```


## Key Decisions

- **P/C/R/W → π/∘/∮/δ**: PCRW was jargon. The math symbols mean what
  they mean in standard physics. π = projection. ∘ = composition.
  ∮ = contour integral. δ = Kronecker delta.

- **Overbar for inverse**: π̄ = antimatter of π. Standard physics
  notation for antiparticles. Write IS the antimatter of read.

- **Bond macros in walks**: w -1,0,0,+1 → π̄δ. The symbol IS the code.
  Comments become redundant — the notation documents itself.

- **θ constants**: θ(U32,P,P) → θ²ρρ. Shell levels as superscript
  digits. No brackets, no commas, no macro call.

- **hodos replaces NASM for walks**: walks are now .w files. NASM only
  needed for legacy/ substrate. Everything new is hodos.

- **BSS → runtime allocation**: walks shouldn't reference BSS addresses.
  Handoff stores loop_region at 0x9200. Walks read from known addresses.
  Partially implemented — genesis.w has display+boot, loops deferred.

- **x86 substrate = one file**: cpu+handoff+nomos+walker merged into
  legacy/x86.asm. Greek at the interface (ƒ,ψ), English internally.


## For the Next Wit

### State of code2/
Boots headless and GUI. Teal screen. "VoE" to debugcon. 4 cores wake.
All 16 bonds work (tested: read, write, port_write, add, subtract,
AND, test, increment, fill, bind). Loop write/read tested via debugcon.

genesis.w assembled by hodos, incbin'd by x86.asm. No NASM for walks.

### What's Missing from genesis.w
Loop operations — the old genesis.asm had loop init + write/read/bind
tests using BSS addresses. genesis.w only has display fill + boot
confirmation. Loop operations need runtime allocation (cursor from
0x9200 instead of BSS symbols). The pattern is clear but needs work
with the single-pipeline constraint.

### GPU Firmware — THE NEXT PRIORITY
Before building keyboard/display walks, we need the GPU alive.
VESA framebuffer = CPU-pushed pixels, no acceleration. Every display
walk built on VESA would need rewriting for a real GPU.

From schism prior work (session 92, gpu-protocol.md):
- GPU driver dissolves into an encoding table (stateless mapping)
- One shader: color = texture(atlas, uv) * tint
- 28 draw record types, ~15-25k lines total (vs Linux 235k)
- GPU IS a loop participant — command buffers ARE ring buffers
- AMD Polaris driver exists as ring-3 status reader (300 lines Rust)

But before the driver works, GPU firmware must be loaded:
1. PCI enumerate → find GPU
2. Map GPU BAR registers
3. Load firmware blobs (PSP, SMU, SDMA — gene data)
4. GPU boots to functional state
5. Init display engine + command ring
6. NOW encoding table driver talks

Steps 1-5 are substrate work (register poking, DMA, timeouts).
The firmware blobs are gene data. The encoding table is a walk.

Recommended path:
1. virtio-gpu in QEMU (no firmware blobs, simplest command ring)
2. AMD with open docs (firmware blobs from linux-firmware)
3. Intel/NVIDIA later

### Critical Reading
- prior/schism/drafts/shower-thoughts/hardware/gpu-protocol.md (full spec)
- prior/schism commit 13dc94c (display as loop routing)
- prior/schism commit 033b452 (session 92: GPU/NIC coverage)
- prior/schism commit 9badf28 (AMD Polaris status reader, 300 lines)
- wit/mind/coder/design "ALL HARDWARE IS LOOPS" section

### Architecture Reminder
- Walks speak lattice: π̄δ θ¹¹ρ 0xE9 'V'
- Substrate speaks x86: mov, push, call (legacy/)
- The boundary is ψ (the walker). Above = coordinates. Below = x86.
- hodos assembles walks. NASM assembles substrate. Both dissolve on trit.
