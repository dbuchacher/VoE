; ═══════════════════════════════════════════════════════════════════
; [3] Handoff — the last x86 code before walks take over
;
; CPU entry [2] jumps here. We're in 64-bit mode, all cores awake.
; This is where x86 setup ends and the lattice begins.
;
; What this does:
;   1. Point the JIT code buffer to its start (equation [4] writes x86 here)
;   2. Seed the 16 bonds — call get_atom for each of the 15 force patterns
;      (bitmask 1-15: P, C, P+C, R, P+R, ... P+C+R+W). This pre-JITs the
;      fundamental operations so they're cached before any walk needs them.
;   3. Run genesis [6] — the founding walk that inits hardware and traces
;   4. Run kbd walk — keyboard poll loop (never returns, loops via loop_back)
;   5. If the walk somehow returns, spin forever (safety net)
;
; The bond_seeds table at the bottom lists all 15 positive-sign force
; combinations. Each is 4 bytes (T, D, M, Q). get_atom JITs each one.
;
; TODO: step 4 should be the bind drain (find fullest trace, drain it)
; instead of hardcoding the keyboard walk.
; ═══════════════════════════════════════════════════════════════════

section .text
bits 64

global lattice_start

extern get_atom
extern walker_wave
extern main_walk, main_len
extern kbd_walk, kbd_walk_len
extern code_base, code_cursor

lattice_start:
    ; init code generation buffer
    lea rax, [code_base]
    mov [code_cursor], rax

    ; ── seed the 16 bonds (the forces / ground state) ──
    ; 15 force patterns (bitmask 1-15) with positive signs
    lea rsi, [bond_seeds]
    mov ecx, 15
.seed:
    push rcx
    push rsi
    movsx edi, byte [rsi]
    movsx esi, byte [rsi+1]
    movsx edx, byte [rsi+2]
    movsx ecx, byte [rsi+3]
    call get_atom
    pop rsi
    pop rcx
    add rsi, 4
    dec ecx
    jnz .seed

    ; ── walk genesis (runs once, returns) ──
    lea rdi, [main_walk]
    mov esi, [main_len]
    call walker_wave

    ; ── genesis returned. enter the market. ──
    ; walk the keyboard poll (first available work)
    ; TODO: bind drain — find fullest trace instead of hardcoding
    lea rdi, [kbd_walk]
    mov esi, [kbd_walk_len]
    call walker_wave

    ; if walk returns (it shouldn't — loop_back), spin
.spin:
    pause
    jmp .spin


; ─── bond seed table (15 positive-sign patterns) ──────────────

section .rodata

bond_seeds:
    db  1,  0,  0,  0                     ; P:     read
    db  0,  1,  0,  0                     ; C:     apply
    db  1,  1,  0,  0                     ; P+C:   fold
    db  0,  0,  1,  0                     ; R:     drain
    db  1,  0,  1,  0                     ; P+R:   slurp
    db  0,  1,  1,  0                     ; C+R:   fix
    db  1,  1,  1,  0                     ; P+C+R: foldl
    db  0,  0,  0,  1                     ; W:     test
    db  1,  0,  0,  1                     ; P+W:   filter
    db  0,  1,  0,  1                     ; C+W:   maybe
    db  1,  1,  0,  1                     ; P+C+W: map
    db  0,  0,  1,  1                     ; R+W:   take_while
    db  1,  0,  1,  1                     ; P+R+W: scan
    db  0,  1,  1,  1                     ; C+R+W: bind
    db  1,  1,  1,  1                     ; P+C+R+W: hylo
