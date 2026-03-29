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
;   θ(a0, a1, a2)           arg encoding (shell levels + deref bits)
;   skip_z  N               skip N bytes if ρ = 0
;   skip_nz N               skip N bytes if ρ ≠ 0
;   loop_back N              rewind N bytes if ρ ≠ 0
;
; ── arg shells ────────────────────────────────────────────────
;
;   shell 0: ρ   (0 bytes — use previous result)
;   shell 1: ¹   (1 byte inline)
;   shell 2: ²   (4 bytes inline)
;   shell 3: ³   (8 bytes inline)

ρ   equ 0
¹   equ 1
²   equ 2
³   equ 3

; ── θ — arg encoding constants ────────────────────────────────
;
;   each θ packs 3 arg shells into one byte (2 bits each)
;   suffix · = dereference (bit 7 for arg0, bit 6 for arg1)
;
;   read as: θ{arg0}{arg1}{arg2}
;   ρ = pipeline  ¹ = 1 byte  ² = 4 bytes  ³ = 8 bytes

θρρρ    equ 0x00
θ¹ρρ    equ 0x01
θ¹¹ρ    equ 0x05
θ²ρρ    equ 0x02
θ²²ρ    equ 0x0A
θ²³ρ    equ 0x0E
θρ¹ρ    equ 0x04
θρ²ρ    equ 0x08
θρ²²    equ 0x28
θ²ρρ·   equ 0x82                ; deref arg0
θρ²ρ·   equ 0x48                ; deref arg1

; θ macro for unusual combinations (brackets + commas = NASM syntax)
%define θ(a0,a1,a2) ((a0)|((a1)<<2)|((a2)<<4))

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

; ── bonds — named macros for common coordinates ──────────────
;
;   the symbol IS the coordinate. no tuple needed.
;   w τ,χ,μ,φ still works for unusual combinations.

; pattern 1: π (read/write)
%macro π 0
  w +1,0,0,0
%endmacro
%macro π̄ 0
  w -1,0,0,0
%endmacro
%macro π₃ 0                     ; byte read
  w +3,0,0,0
%endmacro
%macro π̄₃ 0                     ; byte write
  w -3,0,0,0
%endmacro
%macro π₇ 0                     ; dword read
  w +7,0,0,0
%endmacro
%macro π̄₇ 0                     ; dword write
  w -7,0,0,0
%endmacro

; pattern 2: ∘ (call/return)
%macro ∘ 0
  w 0,+1,0,0
%endmacro
%macro ∘̄ 0
  w 0,-1,0,0
%endmacro

; pattern 4: ∮ (loop/increment)
%macro ∮̄ 0                      ; increment (+1)
  w 0,0,-1,0
%endmacro

; pattern 5: π∮ (copy/fill)
%macro π̄∮̄₃ 0                    ; dword fill
  w -1,0,-3,0
%endmacro

; pattern 8: δ (test/guard) + magnitudes
%macro δ 0                      ; test (==)
  w 0,0,0,+1
%endmacro
%macro δ̄ 0                      ; guard (conditional)
  w 0,0,0,-1
%endmacro
%macro δ₃ 0                     ; AND
  w 0,0,0,+3
%endmacro
%macro δ̄₃ 0                     ; OR
  w 0,0,0,-3
%endmacro
%macro δ₅ 0                     ; XOR
  w 0,0,0,+5
%endmacro
%macro δ̄₅ 0                     ; NOT
  w 0,0,0,-5
%endmacro

; pattern 9: πδ (filter/port/add)
%macro πδ̄ 0                     ; port_read
  w +1,0,0,-1
%endmacro
%macro π̄δ 0                     ; port_write
  w -1,0,0,+1
%endmacro
%macro π̄δ̄ 0                     ; add
  w -1,0,0,-1
%endmacro
%macro π̄δ̄₃ 0                    ; subtract
  w -1,0,0,-3
%endmacro

; pattern 14: ∘∮δ (bind — the event loop)
%macro ∘∮δ 0
  w 0,+1,+1,+1
%endmacro

; pattern 15: π∘∮δ (hylo — full pipeline)
%macro π∘∮δ 0
  w +1,+1,+1,+1
%endmacro

; ── atoms — common formulas ──────────────────────────────────

%macro τχ 0                     ; τ·χ = multiply (arg0 × arg1)
  w +2,+2,0,0
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
