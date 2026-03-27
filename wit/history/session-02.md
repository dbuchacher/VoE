# Session 2: Wave Bytes Complete

Built walker_wave, added magnitude extension, converted all walks, deleted 32-byte walker.

## What happened

Phase 1 — wave byte walker:
- Added walker_wave to equation.asm (~150 lines)
- Added decode_2bit lookup table (00=0, 01=+1, 10=extended, 11=-1)
- Added wave_test_walk (prints "wave\n" to debugcon, 20 bytes vs 160)
- Tested headless + GUI: both work ✓

Phase 2 — magnitude extension:
- 10 in 2-bit field = read next byte as signed i8 coordinate
- Extension bytes between wave byte and flags byte, T/D/M/Q order
- Exists because x86 has width-specific opcodes — lattice-native chip wouldn't need it

Phase 3 — full conversion:
- Converted main_walk (genesis) to wave bytes: 576 → ~99 bytes
- Converted kbd_walk to wave bytes: 224 → 27 bytes
- Converted speaker beep to wave bytes: 128 → 16 bytes
- Changed walk_sub to call walker_wave
- Changed lattice_start to use walker_wave for all walks
- Deleted 32-byte walker (100+ lines removed)
- Binary: 92KB → 88KB

Phase 4 — cleanup:
- Added graceful headless fallback (skip_z before filld when FB=0)
- Headless no longer crashes — prints "genesis\nT" and enters kbd loop
- Renamed font.ha → glyph.ha, font_8x16 → glyph_sdf
- Fixed skip_z bug in kbd_walk (was skip_z(3)→loop_back, now skip_z(6)→add)

Phase 5 — design gaps documented:
- Identified 5 undocumented gaps in build order
- Designed solutions for all 5 (trace R/W, registry, input pipeline, display, NVMe)
- Updated design doc: new sections, stale entries fixed, build order updated
- Documented equation-dissolves insight (exists because of x86 manufacturing)

## Format (final)
- byte 0: wave byte (2 bits per dim, 10=extended)
- [ext]: signed i8 per extended dimension (T,D,M,Q order)
- byte N: flags ([reserved:2][arg2:2][arg1:2][arg0:2])
- bytes: inline args (00=pipeline, 01=u8, 10=u32, 11=u64)
- control: 0xFE/0xFD/0xFC + u32 byte count

## Key decisions
- Flags byte 00 = pipeline (no sentinel values, unlike old PIPE=-1)
- Walker control counts in bytes, not records
- walk_sub stays as asm scaffolding (bridges apply to walker_wave)
- stash_a for trace read/write (single-core; per-trace scratch for multi-core later)

## What's next (full build order)
1. Trace read/write primitive (THE foundational primitive)
2. Input pipeline (scancode → wave char → trace write)
3. Trace registry (fixed array, genesis fills)
4. Bind drain (cores find fullest trace, bind loops it)
5. Serial text output (prove pipeline before SDF)
6. SDF character rendering
7. Born-indexed writes
8. NVMe gene
9. Network traces
