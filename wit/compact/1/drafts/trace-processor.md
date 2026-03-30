# Loop Processor — The Chip

Everything is walks. Walks are naturally parallel. Every loop
is independent. Every core is independent. Single-writer means
no coherency. The architecture IS the parallelism.

Programs aren't "parallelized." They're walks. Walks in loops.
Loops are independent ring buffers. Cores drain them by fill
pressure. A program with 1000 loops uses 1000 cores. Nobody
wrote threading code. The parallelism is structural.


## Why x86 Wastes 70% of the Die

  cache coherency:     ~30-40%    keeping cores' memory views in sync
  branch prediction:   ~10%       guessing which branch to take
  out-of-order engine: ~15%       reordering instructions
  x86 decode:          ~10%       translating CISC → micro-ops
                       ─────
                       ~70%       translation layers the lattice doesn't need

Single-writer loops: no coherency (Pauli exclusion).
Deterministic skip/loop_back: no branch prediction.
Sequential walks: no out-of-order.
81-entry wave byte lookup: no complex decode.

That 70% becomes walker cores.


## The Chip (Same Die as Ryzen 9950X)

  Ryzen 9 9950X              Loop processor (hypothetical, same 4nm)
  ──────────────              ─────────────────────────────────────────
  16 cores                    ~4,000,000 walker cores
  5.7 GHz                    ~2.5 GHz (simpler = can clock high)
  80MB cache                 ~300MB loop SRAM
  170W                       170W
  ~8.6B transistors          ~8.6B transistors (same die)
  ~$600                      ~$600 (same die size)


## Benchmarks (Theoretical)

  Wave byte comparison (search, born-indexed):
    9950X:     ~9.2 trillion/sec peak (AVX-512, 64 per ZMM)
    Loop:     ~10 trillion/sec peak
    BUT sustained: 9950X drops to ~900B (memory-bound)
    Loop chip sustained = peak (data is local)
    Sustained: loop chip 11×

  Complex atom evaluation (D²M/T³ = 5 dependent ops):
    9950X:     ~5 billion atoms/sec sustained (memory-bound)
    Loop:     ~2,000,000 billion atoms/sec (2 quadrillion)
    Ratio: 400,000×

  Born-indexed search (million documents):
    9950X:     16 loops drained simultaneously
    Loop:     4,000,000 loops drained simultaneously
    Ratio: 250,000×


## Why It's Naturally Parallel

Traditional parallelism:
  programmer writes serial code
  → compiler tries to auto-parallelize (mostly fails)
  → programmer manually adds threads, locks, synchronization
  → bugs: races, deadlocks, priority inversion
  → performance: maybe 4-8× on 16 cores

VoE parallelism:
  programmer writes walks (bond sequences)
  → walks go in loops (ring buffers, single-writer)
  → cores drain loops by fill pressure (natural selection)
  → no locks (single-writer = Pauli exclusion)
  → no synchronization (loops are independent)
  → no bugs possible from parallelism
  → performance: scales with core count, no wall

A program with 1000 loops uses 1000 cores without the programmer
knowing or caring. The architecture IS the parallelism. You don't
parallelize walks. Walks ARE parallel.

The 9950X needs a programmer to write threaded code to use 16 cores.
The loop chip uses 4 million cores because walks in loops is all
it does. The parallelism isn't a feature. It's the substrate.


## No BIOS. No OS. No Boot.

  x86: BIOS → bootloader → mode switch → OS kernel → scheduler → app
  loop chip: power on → genesis walks → cores drain loops

Genesis is in ROM on the chip. First clock cycle = first wave byte.
No BIOS (no mode switch needed). No OS (no scheduler — fill pressure).
No boot process. Walks from clock cycle one.


## Processing-In-Memory

  x86: CPU ←bus→ RAM ←bus→ GPU ←bus→ NVMe
       data crosses 4 bus boundaries per operation

  loop chip: walker ←adjacent→ loop SRAM
              data never leaves the core's neighborhood

Von Neumann bottleneck eliminated. The memory IS the compute.
The loop IS the program IS the storage IS the I/O.

No memory bus. No cache hierarchy. No TLB. No page faults.
Walker reads from loop SRAM at its side. 1 cycle. Always.


## The Crystal Connection

The chip layout IS a crystal lattice:
  - Repeating units (walker cores) = atoms
  - Connections (loops) = bonds
  - Data flow = wave propagation
  - Fill pressure = energy gradients

A silicon crystal computing by propagating waves through a lattice
of identical units connected by single-writer channels. That's what
a crystal IS. The chip doesn't simulate a crystal. It IS one.

A quartz crystal oscillates (heartbeat = bind loop).
A silicon crystal walks loops (computation = wave propagation).
Same physics. Different substrate.


## Path to Real Silicon

  Today ($50):       FPGA prototype. ECP5 board. ~800 walker cores.
                     Prove walks run in hardware. HDMI output.

  Next ($10K):       Efabless chipIgnite. SkyWater 130nm. Real ASIC.
                     ~1000-10000 walkers. Custom silicon.

  Goal ($1-10M):     Full tape-out. TSMC 4nm or similar.
                     4 million walkers. The chip from the table above.

  Dream:             Photonic crystal. Light instead of electrons.
                     The universe's native computation medium.
                     No electrons. No transistors. Waves in crystal.


## What Gets Rewritten as Walks

Everything. Not "some things are parallel." ALL programs are walks.

  text editor    = walks reading keyboard loop, writing display loop
  web server     = walks reading network loop, writing response loop
  database       = walks on born-indexed loops. query = walk.
  game engine    = walks computing physics, writing pixel loop
  video decoder  = walks reading file loop, writing frame loop
  compiler       = walks reading source loop, writing binary loop
  AI inference   = walks evaluating atoms across weight loops

Each of these is thousands of independent loops. Each loop =
one core. A game with 10,000 physics objects = 10,000 loops =
10,000 cores, automatically. No threading code. No synchronization.

The programmer writes walks. The chip provides cores.
Fill pressure assigns them. Natural selection optimizes.
