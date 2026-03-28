# Genesis Model — How We Got Here

Session where we stripped nomos to its foundation and rebuilt the
architecture from first principles.

## The Problem

Previous agents built a 1389-line system.asm with an asm heartbeat,
keyboard REPL, mouse driver, display renderer, network stack — all
as hand-coded assembly. One agent wrote the heartbeat as asm. Every
agent after built on top of it. The foundation was wrong and nobody
went back to check.

The design doc explicitly said: "The heartbeat is lattice code, not
asm. get_atom(0,1,1,1) returns the bind bond." But the asm heartbeat
existed, so agents kept it.

## The Strip

Moved system.asm and lat/ to nfg/. Removed PCI scan from bootloader
(530 lines of device enumeration that belonged in the lattice).
Stripped to:

    asm/bootloader.asm   256 lines  — mode switch, AP wake, jmp lattice_start
    asm/equation.asm    1208 lines  — the equation (JIT + walker)
    lat/main.asm          20 lines  — proof of life ("ok" to debugcon)

82KB binary. Boots. Prints "ok". Nothing else.

## The Wave Byte Discovery

Previous agents called the wave byte "future" and "compression."
That's backwards. The wave byte IS the native instruction format.
One byte per bond. 2 bits per dimension. The 32-byte walk record
was the abstraction — 31 bytes of overhead wrapping 1 byte of
actual instruction.

The wave byte is to the lattice what the opcode is to x86. It's
not a compressed version of the record. The record is a decompressed
version of the wave byte.

## The DNA Thread

Started with: "is the heartbeat supposed to be dead because of DNA?"

The design doc says cores are stem cells. Same genome (equation)
everywhere. Different expression (walk data) in each core's loop.
The heartbeat is the bind bond — not a function, a bond the equation
already generates.

This led to: what IS main.asm?

main.asm was a linear program: boot, init, REPL forever. That's a
command economy. One central plan running the BSP for life.

## Natural Selection = Fill Pressure = Capitalism

Loops are ring buffers with fill levels. Full loop = high demand.
Empty loop = no demand. Cores migrate to full loops (find work).

This IS natural selection:
- Variation: different loops have different walk data
- Competition: finite cores
- Selection: full loops attract workers
- Inheritance: successful walks keep running

It's also capitalism:
- Fill level = price signal
- Cores = labor
- Loops = markets
- Protocol fee (R) = cost of existing
- Full loops attract investment (cores)
- Empty loops = bankruptcy

No scheduler. No central authority. Fill pressure self-organizes.
Anarcho-capitalist substrate.

## Genesis vs Main

main.asm loops forever. It IS the program. Command economy.

genesis.asm hands over. It runs once:
1. Express hardware genes (init serial, VBE, NIC)
2. Write walk data into loops
3. Return.

After genesis, the BSP is just another core. Undifferentiated. It
finds a full loop and drains it. Maybe the keyboard loop. Maybe
compute. Doesn't matter. The founding is over.

This maps to embryonic development:
- Genesis = HOX genes, body plan, cell differentiation
- After genesis = adult organism, cells self-organizing
- Development ends. Life begins.
- The embryo program doesn't run forever.

## The Architecture

    lattice_start:
        seed 16 bonds
        walk genesis          <- returns after hardware init
        find full loop       <- BSP becomes regular worker
        walk it               <- same as any AP

No REPL file. No drain file. Roles emerge from loop contents.
A core that drains the keyboard loop IS the keyboard handler —
not by assignment but by proximity to work.

## Technical Changes Made

- Fixed mouse poll (aux bit check for bytes 2-3)
- Fixed display_print (scroll on line wrap)
- Fixed tx_send, inet_cksum, tcp_cksum (apply bond calling convention)
- Replaced bswap32 with lattice gate (0,0,0,+9) — first asm→coordinate conversion
- Sealed equation.asm (removed all non-equation code)
- Removed PCI scan from bootloader (hardware = non-local variables)
- Extracted hardware walks to lat/hw/
- Updated design doc: wave bytes, don't-repeat lessons, genesis model

## What's Next

1. Implement wave byte walker (read bytes not 32-byte records)
2. Write genesis.asm (hand-over, not loop)
3. Implement fill-pressure core migration (natural selection scheduler)
4. MBR boot (16-bit, real hardware)
5. Build up from genesis: hw genes → loop setup → hand-over → life
