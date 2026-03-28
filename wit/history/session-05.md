# Session 5: Loop Primitive + Character Encoding Discovery

Coder session. Built loop read/write (build order item 1), then deep
exploration of character encoding using Compute First.

## What changed

- **walks/loop.asm**: Three macros for loop interaction.
  `loop_empty %1` — 4 bonds, pipeline = 1 (empty) or 0 (has data).
  `loop_read %1` — 10 bonds, reads one record, advances read cursor.
  `loop_write %1` — 9 bonds, writes pipeline to loop (drop mode), advances write cursor.
  All three read loop parameters (mask, recsz, bufaddr) from the header
  via indirect dereference flags (bit 7/6). Generic for any loop/record size.

- **genesis self-test**: After loop initialization, writes 'L' to the keyboard
  loop, checks non-empty, reads it back, echoes to debugcon. Output changed
  from `genesis\nnT` to `genesis\nLnT`. The 'L' proves round-trip works.

- **LP_* constants moved**: Loop header offsets moved from genesis.asm to
  walks/loop.asm. Genesis `%include`s loop.asm after walk.inc.

- **First atom in a walk**: atom TD (+2,+2,0,0) computes slot_index × record_size.
  Uses extended coordinates (2 extra bytes). The atom JIT path was untested
  before this session — now exercised and confirmed working.

- **Design doc updated**: Loops section in CURRENT STATE reflects the built
  primitive. Build order: item 1 marked done, items renumbered.

## What was decided

- Three separate macros (empty/read/write) rather than one mega-macro.
  Caller composes: `loop_empty` + `skip_nz` + `loop_read`. Clean separation.
- loop_write is drop mode (no fullness check). This is the default — rings wrap.
  Blocking writes (for persistence) add a fullness check before the macro.
- Uses stash_a (loop_empty) and stash_b (loop_read, loop_write). No conflicts
  as long as operations don't nest. Documented in header.
- KVM mode for testing. TCG has a debugcon noise bug (session 2). KVM uses
  real hardware virtualization — faithful and clean.

## Testing

- `bash build/run kvm`: `DVgenesis\nLnT` ✓ (DV = MBR boot noise)
- `bash build/run headless`: `genesis\nLnT` ✓
- Binary: 94084 bytes (was 94012 — 72 bytes added for self-test + macros)

## Build order position

DONE: Loop read/write primitive.
NEXT: Input pipeline (scancode → wave char → loop write). This converts the
kbd walk from debugcon output to loop write, adds scancode→wave byte conversion.

## Key technical notes for next wit

- The loop macros use indirect dereference flags extensively:
  `F(P,U32,P) | 0x40` = arg1 is a pointer, walker dereferences to get value.
  `F(U32,P,P) | 0x80` = arg0 is a pointer, walker dereferences.
  Session 2 handoff confirmed these work.
- "read(addr) already dereferences — don't use indirect on read's arg0."
  The read JIT does `mov rax, [rdi]`. The write JIT does `mov [rdi], rsi`.
  Both treat arg0 as a pointer already.
- The atom TD uses `mul` which clobbers rdx. For small values (slot < 64,
  recsz = 8) this doesn't matter. For large products, rdx overflow is silent.
- All addresses fit in U32 (kernel at 1MB, elf32 format). This stays fine
  until the binary exceeds 4GB.

## Character Encoding Exploration

Applied Compute First to the character encoding problem. Three approaches
explored, each dissolving more than the last.

### 1. Solver (char_solve.c)
Frequency-optimized assignment. Dimensions designed for classification
(is_letter = D≠0, case_flip = XOR 0x80). Best result: 63 chars at
shell 1, avg 1.057 bytes/char. Dissolves the text infrastructure stack
but the encoding itself is arbitrary.

### 2. Phonetic (char_phonetic.c)
Derived from articulatory phonetics — not designed, measured:
  T = case, D = place, M = openness, Q = voicing

**'a' = (+1,+1,+1,+1) = hylo.** The simplest vocalization maps to the
most fundamental bond pattern. All four dimensions positive = all four
forces active.

Voicing pairs (b/p, d/t, g/k, v/f, z/s) are Q flips — identical to
how case pairs are T flips. The lattice treats voicing and case as the
same operation on different dimensions.

16 unique shell-1 coordinates for 26 letters. Collisions reveal
phonetic identity: k=c=q=x (all /k/), b=m (both labial closed voiced),
l=r=z (all central partial voiced). Shell 2 disambiguates.

### 3. Legacy comparison
ASCII/UTF-8: 1.000 bytes/char but requires grep, regex, Unicode tables,
iconv, encoding detection, locale handling, font rasterizers.
Phonetic: ~1.4 bytes/char on x86 but dissolves the entire stack AND the
encoding itself. New sounds don't need committees — measure properties,
coordinate exists. On ternary hardware: zero overhead, fixed 4 trits.

### Key discoveries

- **Born-indexed network search**: query remote text without downloading.
  Send 3 trigram keys (~9 bytes), get posting list back (~20 bytes).
  The document never moves. 30 bytes vs 2MB.

- **SDF rendering is a walk**: glyph_sdf(x, y, T, D, M, Q) → distance.
  ~20-30 bonds. No font file, no texture atlas. The walk IS the font.
  walks/sdf.asm when we build it (build order item 5).

- **Ternary dissolves the byte overhead**: shell 2 extension bytes are
  x86 tax. On balanced ternary, every character is 4 trits. Fixed width.
  The "phonetic is 40% larger" problem vanishes on target hardware.

- **Classification for free**: is_vowel, is_voiced, is_front, voice_flip
  — all one gate bond. Legacy can't do any of these without tables.

### Decision
Use phonetic encoding. Derives from physics (articulatory properties),
not committee assignments. The 5.7%–40% x86 overhead is acceptable
given what dissolves: text infrastructure, encoding committees, font
rasterizers, search engines, the encoding itself.

### Files created
- wit/lattice/char_solve.c — frequency-optimized solver
- wit/lattice/char_phonetic.c — phonetic coordinate derivation
- wit/drafts/character-encoding.md — full comparison writeup
