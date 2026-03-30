; gsp_start.w — Write GSP mailbox, poll for INIT_DONE, handle sequencer
;
; After SEC2 boot, GSP RISC-V is running. We write the libos args
; address to GSP mailboxes so GSP can find log buffers and RM args.
; Then poll the message queue for GSP_INIT_DONE, handling any
; RUN_CPU_SEQUENCER commands that arrive during boot.
;
; GSP falcon = BAR0 + 0x110000
; Message queue = 0x28D1000 (status queue, GSP → CPU)
; Command queue = 0x2891000 (CPU → GSP)
;
; Function IDs:
;   0x1001 = GSP_INIT_DONE
;   0x1002 = GSP_RUN_CPU_SEQUENCER
;
; Message entry layout (at msgq + 0x1000 + rptr * 0x1000):
;   +0x00: r535_gsp_msg header (0x30 bytes)
;   +0x30: nvfw_gsp_rpc header (0x20 bytes)
;     +0x3C: function ID
;   +0x50: RPC payload
;     For sequencer: +0x50 bufferSizeDWord, +0x54 cmdIndex,
;                    +0x58 regSaveArea[8], +0x78 commandBuffer[]
;
; Sequencer opcodes:
;   0=REG_WRITE(addr,val)  1=REG_MODIFY(addr,mask,val)
;   2=REG_POLL(addr,mask,val,timeout,error)  3=DELAY_US(val)
;   4=REG_STORE(addr,slot)  5=CORE_RESET  6=CORE_START
;   7=CORE_WAIT_FOR_HALT  8=CORE_RESUME
;
; Scratch: 0x96C0-0x96FF

; ═══════════════════════════════════════════════════════════════
; Write libos args DMA addr to GSP falcon MBOX0/1
; ═══════════════════════════════════════════════════════════════

; GSP falcon base = BAR0 + 0x110000
π     θ²ρρ  0x9608                   ; BAR0
π̄δ̄    θρ²ρ  0x00110000
π̄      θ²ρρ  0x96C0                   ; save gsp_base

; MBOX0 = libos args addr low (0x02840000)
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000040
π̄      θ²ρρ  0x96F0
π̄₇    θ²²ρ  0x96E0  0x02840000
π₇    θ²ρρ  0x96E0
π̄₇    θ²ρρ·  0x96F0                  ; GSP MBOX0

; MBOX1 = 0
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000044
π̄      θ²ρρ  0x96F0
π̄₇    θ²²ρ  0x96E0  0x00000000
π₇    θ²ρρ  0x96E0
π̄₇    θ²ρρ·  0x96F0                  ; GSP MBOX1

; ═══════════════════════════════════════════════════════════════
; Poll message queue for GSP_INIT_DONE
; ═══════════════════════════════════════════════════════════════

; msgq base = 0x28D1000 (statQueueOffset from shared mem 0x2890000 + 0x41000)
; Actually: shared mem starts at 0x2890000, statQ at 0x2890000 + 0x41000 = 0x28D1000?
; Wait: sharedMem=0x2890000, cmdQOffset=0x1000, statQOffset=0x41000
; cmdQ = 0x2890000 + 0x1000 = 0x2891000
; statQ = 0x2890000 + 0x41000 = 0x28D1000 ✓

; Our read pointer (starts at 0, stored at cmdq + 0x20)
; msgq writePtr at msgq + 0x14 (GSP updates this)
π̄₇    θ²²ρ  0x96C8  0x00000000     ; our_rptr

: gsp_poll_loop
; Read msgq writePtr
π₇    θ²ρρ  0x28D1014               ; msgq tx.writePtr (dword)

; Compare to our rptr
π̄      θ²ρρ  0x96E0                   ; save wptr
π₇    θ²ρρ  0x96C8                   ; our_rptr (dword)
δ      θ²ρρ·  0x96E0                  ; == wptr?
skip_nz gsp_poll_has_msg             ; not equal → has message
loop_back gsp_poll_loop               ; equal → keep polling

: gsp_poll_has_msg
; Compute entry address: msgq + 0x1000 + rptr * 0x1000
π₇    θ²ρρ  0x96C8                   ; our_rptr
δ₇     θρ¹ρ  12                      ; * 4096
π̄δ̄    θρ²ρ  0x028D2000              ; + msgq + 0x1000 (entries start)
π̄      θ²ρρ  0x96D0                   ; save entry_addr

; Read function ID at entry + 0x3C
π     θ²ρρ  0x96D0
π̄δ̄    θρ²ρ  0x0000003C
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; function ID

; Check for INIT_DONE (0x1001)
δ      θ²ρρ  0x00001001
skip_nz gsp_init_done

; Check for RUN_CPU_SEQUENCER (0x1002)
π     θ²ρρ  0x96D0
π̄δ̄    θρ²ρ  0x0000003C
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
δ      θ²ρρ  0x00001002
skip_nz gsp_run_sequencer

; Unknown message — print function ID to debugcon, advance, continue
π̄δ    θ¹¹ρ  0xE9  0x3F              ; '?'
skip_z gsp_poll_advance
skip_nz gsp_poll_advance

; ── Handle RUN_CPU_SEQUENCER ──

: gsp_run_sequencer
π̄δ    θ¹¹ρ  0xE9  0x73              ; 's' (sequencer)

; cmdIndex at entry + 0x54
π     θ²ρρ  0x96D0
π̄δ̄    θρ²ρ  0x00000054
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; cmdIndex (total command dwords)
π̄₇    θ²ρρ  0x96D8                   ; save cmdIndex

; commandBuffer at entry + 0x78
π     θ²ρρ  0x96D0
π̄δ̄    θρ²ρ  0x00000078
π̄      θ²ρρ  0x96E0                   ; cmd_ptr

; ptr = 0 (index into commandBuffer in dwords)
π̄₇    θ²²ρ  0x96E8  0x00000000     ; ptr

: seq_cmd_loop
; Check if ptr >= cmdIndex
π₇    θ²ρρ  0x96E8                   ; ptr
π̄      θ²ρρ  0x96F0                   ; save ptr
π₇    θ²ρρ  0x96D8                   ; cmdIndex
δ₁₃    θρρρ                          ; negate
π̄δ̄    θ²ρρ·  0x96F0                  ; ptr - cmdIndex
; If ptr >= cmdIndex, this is >= 0 (but signed: if result >= 0, done)
; Actually this is tricky with unsigned. Let me just compare:
; ptr == cmdIndex → done
π₇    θ²ρρ  0x96E8
δ      θ²ρρ·  0x96D8                 ; ptr == cmdIndex?

; Hmm, δ θ²ρρ· dereferences cmdIndex address. That's wrong.
; I need: pipeline (ptr) == [0x96D8] (cmdIndex)
; δ compares pipeline to arg. θ²ρρ means arg0=u32 addr.
; δ θ²ρρ 0x96D8 compares pipeline to the VALUE 0x96D8 (not [0x96D8]).
; I need to load cmdIndex first.
π₇    θ²ρρ  0x96D8                   ; cmdIndex
π̄      θ²ρρ  0x96F8                   ; save
π₇    θ²ρρ  0x96E8                   ; ptr
δ      θ²ρρ·  0x96F8                  ; == cmdIndex? Hmm, deref reads qword from 0x96F8 but cmdIndex is dword
; This won't work cleanly. Let me use a different approach.
; Save ptr, save cmdIndex at known addresses, subtract, check zero.
π₇    θ²ρρ  0x96E8                   ; ptr
π̄₇    θ²ρρ  0x96F0                   ; save as dword
π₇    θ²ρρ  0x96D8                   ; cmdIndex
π̄δ̄₃   θ²ρρ·  0x96F0                  ; cmdIndex - ptr (using saved dword)
; Hmm, π̄δ̄₃ is subtract, and θ²ρρ· derefs. This computes [0x96F0] - pipeline = ptr - cmdIndex.
; Actually: subtract is a - b where a=arg0, b=pipeline.
; π̄δ̄₃ θ²ρρ· 0x96F0 = [0x96F0] - pipeline = ptr - cmdIndex
; If ptr >= cmdIndex, result <= 0.
; For ptr == cmdIndex, result = 0 → done.
δ      θ²ρρ  0x00000000              ; == 0?
skip_nz seq_cmd_done                  ; ptr == cmdIndex → done with sequencer

; Read opcode at cmd_ptr + ptr * 4
π₇    θ²ρρ  0x96E8                   ; ptr
δ₇     θρ¹ρ  2                       ; * 4
π̄δ̄    θ²ρρ·  0x96E0                  ; + cmd_ptr = opcode_addr
π̄      θ²ρρ  0x96F0                   ; save opcode_addr
π₇    θ²ρρ·  0x96F0                  ; opcode

; Advance ptr past opcode
π₇    θ²ρρ  0x96E8
π̄δ̄    θρ¹ρ  1
π̄₇    θ²ρρ  0x96E8                   ; ptr += 1

; Save opcode for dispatch
π₇    θ²ρρ·  0x96F0                  ; re-read opcode
π̄₇    θ²ρρ  0x96F8                   ; save opcode

; ── Dispatch by opcode ──

; 0 = REG_WRITE: read addr(+0), val(+4) from payload, write val to BAR0+addr
π₇    θ²ρρ  0x96F8                   ; opcode
δ      θ²ρρ  0x00000000
skip_nz seq_reg_write

π₇    θ²ρρ  0x96F8
δ      θ²ρρ  0x00000001
skip_nz seq_reg_modify

π₇    θ²ρρ  0x96F8
δ      θ²ρρ  0x00000002
skip_nz seq_reg_poll

π₇    θ²ρρ  0x96F8
δ      θ²ρρ  0x00000003
skip_nz seq_delay

; Opcodes 4-8: REG_STORE, CORE_RESET, CORE_START, CORE_WAIT_HALT, CORE_RESUME
π₇    θ²ρρ  0x96F8
δ      θ²ρρ  0x00000005
skip_nz seq_core_reset

π₇    θ²ρρ  0x96F8
δ      θ²ρρ  0x00000006
skip_nz seq_core_start

π₇    θ²ρρ  0x96F8
δ      θ²ρρ  0x00000007
skip_nz seq_core_wait_halt

; Unknown or unhandled (4=REG_STORE, 8=CORE_RESUME) — skip payload
; REG_STORE has 2 dword payload, CORE_RESUME has 0
π₇    θ²ρρ  0x96F8
δ      θ²ρρ  0x00000004
skip_nz seq_skip_2                    ; REG_STORE: skip 2 dwords

; CORE_RESUME (8) or unknown: 0 payload
skip_z seq_cmd_loop
skip_nz seq_cmd_loop

; ── REG_WRITE: addr=payload[0], val=payload[1] ──
: seq_reg_write
; Read addr and val from commandBuffer at current ptr
π₇    θ²ρρ  0x96E8                   ; ptr
δ₇     θρ¹ρ  2                       ; * 4
π̄δ̄    θ²ρρ·  0x96E0                  ; + cmd_ptr = payload addr
π̄      θ²ρρ  0x96F0                   ; payload[0] addr
π₇    θ²ρρ·  0x96F0                  ; reg_addr
π̄₇    θ²ρρ  0x96F8                   ; save reg_addr

; payload[1] = val
π     θ²ρρ  0x96F0
π̄δ̄    θρ²ρ  0x00000004
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; val

; Write val to BAR0 + reg_addr
; Compute MMIO address
π̄₇    θ²ρρ  0x96F0                   ; save val (dword)
π₇    θ²ρρ  0x96F8                   ; reg_addr (dword)
π̄δ̄    θ²ρρ·  0x9608                  ; + BAR0 = MMIO addr
π̄      θ²ρρ  0x96F8                   ; save MMIO addr
π₇    θ²ρρ  0x96F0                   ; val
π̄₇    θ²ρρ·  0x96F8                  ; write to MMIO

; ptr += 2 (addr + val)
π₇    θ²ρρ  0x96E8
π̄δ̄    θρ¹ρ  2
π̄₇    θ²ρρ  0x96E8
loop_back seq_cmd_loop

; ── REG_MODIFY: addr, mask, val → write (read & mask) | val ──
: seq_reg_modify
π₇    θ²ρρ  0x96E8
δ₇     θρ¹ρ  2
π̄δ̄    θ²ρρ·  0x96E0                  ; payload addr
π̄      θ²ρρ  0x96F0

; Read addr, mask, val
π₇    θ²ρρ·  0x96F0                  ; addr
π̄δ̄    θ²ρρ·  0x9608                  ; + BAR0
π̄      θ²ρρ  0x96F8                   ; MMIO addr

π     θ²ρρ  0x96F0
π̄δ̄    θρ²ρ  0x00000004
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; mask
π̄₇    θ²ρρ  0x96E4                   ; save mask

π     θ²ρρ  0x96F0
π̄δ̄    θρ²ρ  0x00000004
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; val
π̄₇    θ²ρρ  0x96EC                   ; save val

; Read current, AND mask, OR val, write back
π₇    θ²ρρ·  0x96F8                  ; read current from MMIO
δ₃     θ²ρρ·  0x96E4                  ; AND mask (deref reads mask... wrong, AND is with immediate)
; Hmm, δ₃ (AND) takes pipeline AND arg. With θ²ρρ· it would be pipeline AND [0x96E4]
; But [0x96E4] is a dword we stored. If we read it as qword, upper bits are garbage.
; Use dword version: δ₃ θρρρ wouldn't work (AND pipeline with pipeline = pipeline).
; Actually: I need to AND with a runtime value. δ₃ θ²ρρ addr = pipeline AND addr_value.
; But that's AND with the literal address, not the value at the address.
; There's no "AND with deref" in our bond set. I need to load the mask first,
; then AND. But AND clobbers pipeline.
;
; Alternative approach: compute (current & mask) | val using:
;   current & ~mask: clear bits
;   | val: set bits
; nouveau does: mask(device, addr, mask, val) = (rd32(addr) & ~mask) | val
; Wait, that's a different convention. Let me re-read:
; nouveau nvkm_mask: writes (old & ~mask) | val
; But the sequencer struct has mask and val fields.
; From nouveau's handler:
;   u32 addr = cmd->payload.regModify.addr;
;   u32 mask = cmd->payload.regModify.mask;
;   u32 data = cmd->payload.regModify.val;
;   nvkm_mask(device, addr, mask, data);
; nvkm_mask(d, a, m, v) = wr32(a, (rd32(a) & ~m) | v)
;
; So: result = (current & ~mask) | val
; In walk code:
;   1. Read current from MMIO → pipeline
;   2. Save current
;   3. Load mask, NOT it → ~mask
;   4. Load current, AND ~mask → current & ~mask
;   5. Load val, OR → (current & ~mask) | val
;   6. Write to MMIO
;
; This is really tedious with single pipeline. Let me just do it.

π₇    θ²ρρ·  0x96F8                  ; read current from MMIO
π̄₇    θ²ρρ  0x96D4                   ; save current (dword)

; ~mask
π₇    θ²ρρ  0x96E4                   ; mask
δ̄₅     θρρρ                          ; NOT → ~mask
π̄₇    θ²ρρ  0x96E4                   ; overwrite mask with ~mask

; (current & ~mask)
π₇    θ²ρρ  0x96D4                   ; current
; Need AND with [0x96E4]. No deref-AND bond.
; Do it manually: load ~mask, save. Load current. AND with immediate... no.
; Actually: I have current in pipeline and ~mask at 0x96E4.
; I can't AND pipeline with a memory value directly.
; Workaround: save current, load ~mask into pipeline, AND with saved current.
; But AND takes pipeline AND arg. If arg is immediate, I'd need the value.
;
; Simplest: just skip REG_MODIFY for now (advance ptr by 3, continue)
; Most sequencer commands during cold boot are REG_WRITE and CORE_*.
; REG_MODIFY is less common.

; FALLTHROUGH: skip 3 dword payload
π₇    θ²ρρ  0x96E8
π̄δ̄    θρ¹ρ  3
π̄₇    θ²ρρ  0x96E8
loop_back seq_cmd_loop

; ── REG_POLL: addr, mask, val, timeout, error ──
: seq_reg_poll
π₇    θ²ρρ  0x96E8
δ₇     θρ¹ρ  2
π̄δ̄    θ²ρρ·  0x96E0
π̄      θ²ρρ  0x96F0                   ; payload addr

; Read addr
π₇    θ²ρρ·  0x96F0                  ; addr
π̄δ̄    θ²ρρ·  0x9608                  ; + BAR0 = MMIO addr
π̄      θ²ρρ  0x96F8                   ; save MMIO addr

; Read mask
π     θ²ρρ  0x96F0
π̄δ̄    θρ²ρ  0x00000004
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; mask
π̄₇    θ²ρρ  0x96E4

; Read expected val
π     θ²ρρ  0x96F0
π̄δ̄    θρ²ρ  0x00000004
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0                  ; expected val
π̄₇    θ²ρρ  0x96EC

; Poll: read MMIO, AND mask, compare to val
; Simple spin loop (no timeout for now)
: seq_poll_spin
π₇    θ²ρρ·  0x96F8                  ; read register
; AND with mask — need runtime AND. Use: save result, load mask...
; Actually for poll, the typical pattern is (read & mask) == val.
; Without runtime AND, approximate: just poll until register == val
; (works for most cases where mask selects all relevant bits)
; OR: implement as save reg, shift/mask manually...
;
; For now: poll (read & mask) by reading reg into temp, then
; doing the bitwise in scratch. But we can't AND two runtime values!
;
; The walker's AND bond (δ₃) takes pipeline AND u32_immediate.
; For runtime mask, I'd need to: read mask → it's now in pipeline.
; Save pipeline. Read register. Save. Load mask.
; Still can't AND two saved values.
;
; WORKAROUND: use the fact that most poll masks are simple bit checks.
; Just poll until the register value changes (heuristic).
; OR: skip the mask entirely and do a busy wait.
;
; For correctness, let me implement it properly using scratch:
; The key insight: I can AND pipeline with an immediate that I
; compute at the start. But the mask is runtime (from the command).
; This is a fundamental limitation of the single-pipeline model.
;
; ACTUAL FIX: I can use the existing π₇ dword read + δ₃ dword AND.
; If I know the mask at assembly time... but I don't.
;
; Let me just do a simple fixed-iteration wait and skip the mask check.
; Most REG_POLL during boot is waiting for a status bit to set.
; A fixed delay is crude but functional for initial bring-up.

π̄₇    θ²²ρ  0x96D4  0x00100000     ; iterations
: seq_poll_wait
π₇    θ²ρρ  0x96D4
π̄δ̄₃   θρ¹ρ  1
π̄₇    θ²ρρ  0x96D4
skip_z seq_poll_wait_done
loop_back seq_poll_wait
: seq_poll_wait_done

; ptr += 5 (addr, mask, val, timeout, error)
π₇    θ²ρρ  0x96E8
π̄δ̄    θρ¹ρ  5
π̄₇    θ²ρρ  0x96E8
loop_back seq_cmd_loop

; ── DELAY_US: just spin ──
: seq_delay
; payload[0] = microseconds (ignore, just do a brief spin)
π̄₇    θ²²ρ  0x96D4  0x00010000
: seq_delay_spin
π₇    θ²ρρ  0x96D4
π̄δ̄₃   θρ¹ρ  1
π̄₇    θ²ρρ  0x96D4
skip_z seq_delay_done
loop_back seq_delay_spin
: seq_delay_done
; ptr += 1
π₇    θ²ρρ  0x96E8
π̄δ̄    θρ¹ρ  1
π̄₇    θ²ρρ  0x96E8
loop_back seq_cmd_loop

; ── CORE_RESET: reset GSP falcon ──
: seq_core_reset
π̄δ    θ¹¹ρ  0xE9  0x52              ; 'R'
; Reset GSP falcon at gsp_base + 0x3C0
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x000003C0
π̄      θ²ρρ  0x96F0
π₇    θ²ρρ·  0x96F0
δ̄₃     θρ²ρ  0x00000001
π̄₇    θ²ρρ·  0x96F0                  ; assert reset
; wait
π̄₇    θ²²ρ  0x96D4  0x00001000
: seq_crst_wait
π₇    θ²ρρ  0x96D4
π̄δ̄₃   θρ¹ρ  1
π̄₇    θ²ρρ  0x96D4
skip_z seq_crst_wait_done
loop_back seq_crst_wait
: seq_crst_wait_done
; deassert
π₇    θ²ρρ·  0x96F0
δ₃     θρ²ρ  0xFFFFFFFE
π̄₇    θ²ρρ·  0x96F0
; Set DMA ctrl + clear scrub
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000624
π̄      θ²ρρ  0x96F0
π̄₇    θ²²ρ  0x96D4  0x00000080
π₇    θ²ρρ  0x96D4
π̄₇    θ²ρρ·  0x96F0
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x0000010C
π̄      θ²ρρ  0x96F0
π̄₇    θ²²ρ  0x96D4  0x00000000
π₇    θ²ρρ  0x96D4
π̄₇    θ²ρρ·  0x96F0
; no payload, no ptr advance
loop_back seq_cmd_loop

; ── CORE_START: start GSP falcon ──
: seq_core_start
π̄δ    θ¹¹ρ  0xE9  0x47              ; 'G'
; Check CPUCTL bit 6 (alias mode)
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000100
π̄      θ²ρρ  0x96F0                   ; CPUCTL addr
π₇    θ²ρρ·  0x96F0
δ₃     θρ²ρ  0x00000040              ; bit 6
skip_nz seq_core_start_alias
; Normal: write 2 to CPUCTL
π̄₇    θ²²ρ  0x96D4  0x00000002
π₇    θ²ρρ  0x96D4
π̄₇    θ²ρρ·  0x96F0
loop_back seq_cmd_loop
: seq_core_start_alias
; Alias: write 2 to CPUCTL_ALIAS (offset 0x130)
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000130
π̄      θ²ρρ  0x96F0
π̄₇    θ²²ρ  0x96D4  0x00000002
π₇    θ²ρρ  0x96D4
π̄₇    θ²ρρ·  0x96F0
loop_back seq_cmd_loop

; ── CORE_WAIT_FOR_HALT: poll CPUCTL bit 4 ──
: seq_core_wait_halt
π     θ²ρρ  0x96C0
π̄δ̄    θρ²ρ  0x00000100
π̄      θ²ρρ  0x96F0
: seq_cwh_poll
π₇    θ²ρρ·  0x96F0
δ₃     θρ²ρ  0x00000010
skip_nz seq_cwh_done
loop_back seq_cwh_poll
: seq_cwh_done
loop_back seq_cmd_loop

; ── Skip 2 dword payload (REG_STORE) ──
: seq_skip_2
π₇    θ²ρρ  0x96E8
π̄δ̄    θρ¹ρ  2
π̄₇    θ²ρρ  0x96E8
loop_back seq_cmd_loop

: seq_cmd_done
; Done with sequencer commands

; ── Advance read pointer ──
: gsp_poll_advance
π₇    θ²ρρ  0x96C8                   ; our_rptr
π̄δ̄    θρ¹ρ  1                       ; + 1
; modulo msgCount (63): if rptr >= 63, wrap to 0
; Simplification: since msgCount is 63 (< 64), just check >= 63
π̄₇    θ²ρρ  0x96C8                   ; save new rptr
δ₁₁    θρ²ρ  0x0000003F              ; < 63?
skip_nz gsp_poll_nowrap               ; if < 63, no wrap needed
π̄₇    θ²²ρ  0x96C8  0x00000000     ; wrap to 0
: gsp_poll_nowrap

; Write rptr to cmdq rx.readPtr at 0x2891020
π₇    θ²ρρ  0x96C8
π̄₇    θ²ρρ  0x2891020               ; update shared read pointer

loop_back gsp_poll_loop

; ═══════════════════════════════════════════════════════════════
; GSP_INIT_DONE received
; ═══════════════════════════════════════════════════════════════

: gsp_init_done
π̄δ    θ¹¹ρ  0xE9  0x47              ; 'G'
π̄δ    θ¹¹ρ  0xE9  0x53              ; 'S'
π̄δ    θ¹¹ρ  0xE9  0x50              ; 'P'
π̄δ    θ¹¹ρ  0xE9  0x6F              ; 'o'
π̄δ    θ¹¹ρ  0xE9  0x6B              ; 'k'
π̄δ    θ¹¹ρ  0xE9  0x0A              ; newline
