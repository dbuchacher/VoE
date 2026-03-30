; test_loop.w — consumer walk: drain byte records from test loop
;
; Reads loop header + buffer addresses from genesis scratch:
;   [0x9B00] u64  loop header address
;   [0x9B08] u64  buffer address
;
; Loop header layout (64 bytes):
;   +0x00  write_cursor  (u64, number of records written)
;   +0x30  scratch_a     (u64, reader's cursor — how many we've read)
;
; Records are single bytes. depth=16, mask=15.
; Reads each byte and writes to debugcon (port 0xE9).
; Stops when read_cursor == write_cursor (empty).
;
; Scratch used: 0x9B10 (write_cursor), 0x9B18 (read_cursor)

: consume_loop

; ── load write_cursor from header+0x00 ──────────
π     θ²ρρ·  0x9B00                ; pipeline = *[header_addr] = write_cursor
π̄     θ²ρρ   0x9B10                ; save write_cursor to scratch

; ── load read_cursor from header+0x30 ───────────
π     θ²ρρ   0x9B00                ; pipeline = header addr
π̄δ̄   θρ²ρ   0x30                  ; pipeline += 0x30
π     θρρρ                          ; pipeline = [header+0x30] = read_cursor
π̄     θ²ρρ   0x9B18                ; save read_cursor

; ── empty check: read_cursor == write_cursor? ───
δ     θρ²ρ·  0x9B10                ; pipeline = (read == write) ? 1 : 0
skip_nz consume_done               ; equal → empty → done

; ── compute slot address: buf + (read_cursor & 15) ──
π₇    θ²ρρ   0x9B18                ; pipeline = read_cursor (as dword)
δ₃    θρ¹ρ   0x0F                  ; pipeline &= 15 (mask)
π̄δ̄   θρ²ρ·  0x9B08                ; pipeline += buf_addr → slot address

; ── read byte record ────────────────────────────
π₃    θρρρ                          ; pipeline = byte at [slot]

; ── print to debugcon ───────────────────────────
π̄δ    θ¹ρρ   0xE9                  ; port_write(0xE9, pipeline_byte)

; ── advance read_cursor ─────────────────────────
π₇    θ²ρρ   0x9B18                ; pipeline = old read_cursor
π̄δ̄   θρ²ρ   1                     ; pipeline += 1
π̄     θ²ρρ   0x9B18                ; save new read_cursor

; ── write cursor back to header+0x30 ────────────
π     θ²ρρ   0x9B00                ; pipeline = header addr
π̄δ̄   θρ²ρ   0x30                  ; pipeline = header + 0x30
π̄    θρ²ρ·  0x9B18                 ; write *[0x9B18] (new cursor) to [pipeline]

; ── loop: reload nonzero pipeline for loop_back ─
π₇    θ²ρρ   0x9B18                ; pipeline = read_cursor (always > 0 here)
loop_back consume_loop

: consume_done
; pipeline = 1 (from the equality test). walk ends.
