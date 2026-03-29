; walk.inc — encoding tool: lattice coordinates to wave bytes
;
; NASM macros. Zero runtime cost. The output binary is identical
; to hand-coded bytes. This file IS the encoding layer (Q/M) —
; it translates coordinates into the byte format x86 needs.
; On a trit processor, this file dissolves.
;
;   w T, D, M, Q       emit wave byte + extension bytes
;   F(a0, a1, a2)      build flags byte
;   skip_z N            skip N bytes forward if pipeline == 0
;   skip_nz N           skip N bytes forward if pipeline != 0
;   loop_back N         rewind N bytes if pipeline != 0
;
;   P   = pipeline (no inline bytes, use previous result)
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
; bit 7: deref arg0. bit 6: deref arg1.
%define F(a0,a1,a2) ((a0)|((a1)<<2)|((a2)<<4))

; ── wave byte from coordinates ────────────────────────────────
; 4 integers (T, D, M, Q) → wave byte + extension bytes.
; 2 bits per dimension: 00=0  01=+1  11=-1  10=extended
; Bit layout: [7:6]=T  [5:4]=D  [3:2]=M  [1:0]=Q
; Extended: 10 marker + signed i8 for values outside {-1, 0, +1}
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
; Not bonds. Walker instructions (bytes >= 0xFC).
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
