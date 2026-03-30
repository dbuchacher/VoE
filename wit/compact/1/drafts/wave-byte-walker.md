# Wave Byte Walker — Design

## Format

  byte 0: wave byte (the bond coordinate)
  byte 1: flags (arg widths, 2 bits per arg)
  bytes 2+: inline args (0-24 bytes)

## Flags Byte

  [reserved:2][arg2:2][arg1:2][arg0:2]
    00 = pipeline (no bytes)
    01 = u8 (1 byte)
    10 = u32 (4 bytes)
    11 = u64 (8 bytes)

## Walker Control (wave byte >= 0xF8)

  0xFE + u32 = skip_z (bytes forward if pipeline == 0)
  0xFD + u32 = skip_nz (bytes forward if pipeline != 0)
  0xFC + u32 = loop_back (bytes backward if pipeline != 0)

## Examples

  port_write(0xE9, 'A'):
    C1 06 E9000000 41              7 bytes (was 32)

  add(pipeline, pipeline):
    C5 00                          2 bytes (was 32)

  read(0x9100):
    40 02 00910000                 6 bytes (was 32)

  filld(0xFD000000, blue, 786432):
    C4 2A 000000FD 99663300 00C00000   14 bytes (was 32)

## Cache Optimization (LATER, not now)

  64-byte cache line = ~8-10 wave byte records vs 2 at 32-byte.
  Fewer cache misses per walk. Alignment, prefetch, packing
  could help further. Get it working first. Optimize later.
