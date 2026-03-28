; walk.inc — write walks as lattice coordinates
;
; Nobody has coded lattice walks before. We're all retarded here.
; Comments are written accordingly.
;
; THE PROBLEM:
;   The lattice is balanced ternary. Each dimension is a trit: -1, 0, or +1.
;   A bond (one instruction) is 4 trits — one per dimension (T, D, M, Q).
;   A 4-trit processor would execute walks directly. No encoding needed.
;   The Soviets built a balanced ternary computer in 1958 (Setun).
;   Then the world picked binary instead. So here we are, encoding
;   trits into bits on x86, 68 years later. Progress.
;
; WHAT THIS FILE DOES:
;   Lets you write walks as coordinates instead of hex bytes.
;   NASM macros expand at assembly time. Zero runtime cost.
;   The output binary is identical to hand-coded bytes.
;
; WHY WE NEED F() AND ARG SIZES:
;   x86 instructions have specific widths (byte, dword, qword).
;   The walker needs to know how many bytes each arg takes in the
;   walk byte stream. On a trit processor, the coordinate itself
;   would determine the width. On x86, we have to say it explicitly.
;   P = "no arg, use whatever's in the pipeline from the last bond."
;
; HOW A WALK RECORD LOOKS:
;   w -1, 0, 0, +1              ← the coordinate (here: port_write)
;   db F(U8,U8,P), 0xE9, 'g'   ← flags byte + inline arg data
;   |                |     |
;   |                |     +-- arg1: the character 'g'
;   |                +-------- arg0: port 0xE9 (debugcon)
;   +------------------------- flags: arg0 is u8, arg1 is u8, arg2 is pipeline
;
;   The w macro encodes (-1,0,0,+1) into wave byte 0xC1.
;   F(U8,U8,P) computes the flags byte 0x05.
;   The result in the binary: C1 05 E9 67. Same as writing db 0xC1, 0x05, 0xE9, 'g'.
;   But now you can see the coordinates.
;
; EXTENDED COORDINATES:
;   Shell-1 bonds use -1, 0, +1. They fit in 2 bits (one trit).
;   Higher magnitudes (like T=+7 for extended reads) use a marker bit
;   pattern (10) followed by a signed byte. This is x86 tax —
;   a trit processor would just have wider trits.
;
; REFERENCE:
;   w T, D, M, Q       emits wave byte + extension bytes
;   F(a0, a1, a2)      builds flags byte:  a0 | (a1<<2) | (a2<<4)
;   skip_z N            skip N bytes forward if pipeline == 0
;   skip_nz N           skip N bytes forward if pipeline != 0
;   loop_back N         rewind N bytes if pipeline != 0
;
;   P   = pipeline (no inline bytes)
;   U8  = 1 byte inline
;   U32 = 4 bytes inline
;   U64 = 8 bytes inline

; ── arg size constants ────────────────────────────────────────
P   equ 0
U8  equ 1
U32 equ 2
U64 equ 3

; ── flags byte builder ────────────────────────────────────────
; Packs 3 arg sizes into one byte. Each arg gets 2 bits.
; F(U8, U32, P) = 1 | (2<<2) | (0<<4) = 0x09
%define F(a0,a1,a2) ((a0)|((a1)<<2)|((a2)<<4))

; ── wave byte from coordinates ────────────────────────────────
; Takes 4 integers (T, D, M, Q). Encodes into a wave byte.
; Each coordinate becomes 2 bits:  00=0  01=+1  11=-1  10=extended
; Bit layout: [7:6]=T  [5:4]=D  [3:2]=M  [1:0]=Q
; If a coordinate is outside {-1, 0, +1}, it emits the 10 marker
; and appends a signed byte with the actual value.
%macro w 4
  %assign _wb 0
  %if %1 == 1
    %assign _wb _wb | (1 << 6)
  %elif %1 == -1
    %assign _wb _wb | (3 << 6)
  %elif %1 != 0
    %assign _wb _wb | (2 << 6)
  %endif
  %if %2 == 1
    %assign _wb _wb | (1 << 4)
  %elif %2 == -1
    %assign _wb _wb | (3 << 4)
  %elif %2 != 0
    %assign _wb _wb | (2 << 4)
  %endif
  %if %3 == 1
    %assign _wb _wb | (1 << 2)
  %elif %3 == -1
    %assign _wb _wb | (3 << 2)
  %elif %3 != 0
    %assign _wb _wb | (2 << 2)
  %endif
  %if %4 == 1
    %assign _wb _wb | 1
  %elif %4 == -1
    %assign _wb _wb | 3
  %elif %4 != 0
    %assign _wb _wb | 2
  %endif
  db _wb
  %if %1 != 0 && %1 != 1 && %1 != -1
    db (%1) & 0xFF
  %endif
  %if %2 != 0 && %2 != 1 && %2 != -1
    db (%2) & 0xFF
  %endif
  %if %3 != 0 && %3 != 1 && %3 != -1
    db (%3) & 0xFF
  %endif
  %if %4 != 0 && %4 != 1 && %4 != -1
    db (%4) & 0xFF
  %endif
%endmacro

; ── walker control ────────────────────────────────────────────
; These are not bonds. They're walker instructions (bytes >= 0xFC).
; They control flow within the walk byte stream.
%macro skip_z 1
  db 0xFE
  dd %1
%endmacro

%macro skip_nz 1
  db 0xFD
  dd %1
%endmacro

%macro loop_back 1
  db 0xFC
  dd %1
%endmacro
