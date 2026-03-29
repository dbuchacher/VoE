; walk.inc — encoding: coordinates → wave bytes
;
;   physics:    encodes quantum numbers into wave packets
;   math:       packs 4D coordinates into compact byte representation
;   programmer: NASM macros that emit walk bytecode at zero runtime cost
;   english:    turns 4 numbers into the bytes the machine reads
;
; On a trit processor, this file dissolves — hardware reads coordinates natively.
;
; ── usage ─────────────────────────────────────────────────────
;
;   w τ, χ, μ, φ            emit wave byte (+ extension bytes if |coord| > 1)
;   F(a0, a1, a2)           build flags byte for 3 args
;   skip_z  N               skip N bytes if ρ = 0   (pipeline empty)
;   skip_nz N               skip N bytes if ρ ≠ 0   (pipeline has value)
;   loop_back N              rewind N bytes if ρ ≠ 0 (keep looping)
;
; ── arg sizes ─────────────────────────────────────────────────
;
;   ρ   = pipeline     (no inline bytes — use previous result)
;   U8  = 1 byte       (inline constant)
;   U32 = 4 bytes      (inline constant)
;   U64 = 8 bytes      (inline constant)

ρ   equ 0
U8  equ 1
U32 equ 2
U64 equ 3

; ── F(a0, a1, a2) — flags byte ───────────────────────────────
;
;   packs 3 arg sizes into one byte (2 bits each)
;   bit 7: dereference arg0 (follow pointer)
;   bit 6: dereference arg1 (follow pointer)

%define F(a0,a1,a2) ((a0)|((a1)<<2)|((a2)<<4))

; ── w τ, χ, μ, φ — wave byte from coordinates ────────────────
;
;   2 bits per dimension:  00 = 0    01 = +1    11 = -1    10 = extended
;   bit layout: [7:6]=τ  [5:4]=χ  [3:2]=μ  [1:0]=φ
;
;   shell 1 (±1): fits in 1 byte. higher magnitudes add 1 extension byte each.

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

; ── ψ control — walker instructions (not bonds) ──────────────
;
;   bytes ≥ 0xFC are walker control, not coordinates.
;   these are the only non-lattice bytes in a walk.

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
