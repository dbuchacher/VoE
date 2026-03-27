# Session 1 Handoff — For the Next Wit

## What to build next

Wave byte walker. The design is in wit/drafts/wave-byte-walker.md.
The current walker (32-byte records) is in asm/equation.asm starting
at the `walker:` label. Add `walker_wave:` alongside it.

Read CURRENT STATE vs DESIGN in mind/coder/design before starting.

## The wave byte walker in brief

1. Read 1 byte: wave byte
2. If >= 0xF8: walker control (skip/loop), read u32 count in BYTES
3. Else: read 1 byte flags, read inline args per flags
4. Decode wave byte → 4 coords (2-bit lookup table)
5. Call get_atom (already cached) → JIT function ptr
6. Set registers: pipeline where flags=00, inline where not
7. Call function, result → pipeline, advance, repeat

## Don't fuck up

- The equation stays SEALED. You're adding a new walker entry point,
  not new bond types.
- Test with the existing genesis walk first — convert it to wave bytes
  and verify it still boots (blue screen + beep).
- The 32-byte walker stays. Both formats coexist until wave bytes
  are proven. Don't delete the old walker.
