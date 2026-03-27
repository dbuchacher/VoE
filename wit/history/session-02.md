# Session 2: Wave Byte Walker

Built walker_wave — the compact walker that reads 1-byte bonds.

## What happened
- Added walker_wave to equation.asm (~150 lines, alongside existing 32-byte walker)
- Added decode_2bit lookup table in .rodata (2-bit → signed coordinate: 00=0, 01=+1, 10=reserved, 11=-1)
- Added wave_test_walk in genesis.asm (prints "wave\n" to debugcon, 20 bytes vs 160)
- Wired into lattice_start: walker_wave runs before genesis walk
- Tested headless: "wave" appears before "genesis" in debugcon ✓
- Tested GUI boot: blue screen appears, no regressions ✓

## Format
- byte 0: wave byte (2 bits per dim, [7:6]=T [5:4]=D [3:2]=M [1:0]=Q)
- byte 1: flags ([reserved:2][arg2:2][arg1:2][arg0:2]), 00=pipeline 01=u8 10=u32 11=u64
- bytes 2+: inline args (variable length per flags)
- Walker control: 0xFE+u32=skip_z, 0xFD+u32=skip_nz, 0xFC+u32=loop_back (counts in bytes)

## Decisions
- Shell-1 only: wave bytes encode -1/0/+1 per dimension. Higher magnitudes (|Q|=3 for AND, |R|=3 for filld, |T|=7 for rdd) not expressible yet
- Both walkers coexist: 32-byte format stays for genesis and kbd_walk
- No r15 save/restore in walker_wave (matches existing walker convention — lattice_start doesn't depend on r15 across calls)

## Known limitation
- Can't convert genesis walk (needs filld=|R|3, rdd=|T|7) or kbd_walk (needs AND=|Q|3) to wave bytes without a magnitude extension mechanism
- Possible extension: 10 encoding = "next byte provides signed i8 coordinate" — not designed yet

## What's next
- Magnitude extension for wave bytes (to express AND, filld, dword read, shifts)
- Convert kbd_walk to wave bytes (needs AND at minimum)
- Bind heartbeat (BSP + APs drain traces)
- SDF character rendering
