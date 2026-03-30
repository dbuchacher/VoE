; cursor.w — mouse cursor: triangle from math, not bitmap
;
; width(row) = row + 1. Each row is a pixel-write loop.
; No bitmap, no per-pixel branching, no x86 helper.
;
; Scratch:
;   0x9B60  mouse_x         0x9B68  mouse_y
;   0x9B70  old_mouse_x     0x9B78  old_mouse_y
;   0x9BA0  row counter     0x9BA8  row FB address
;   0x9BB0  fill width      0x9BC0  pixel addr (inner loop)
;   0x9BC8  pixel counter

; skip if no framebuffer
π₇    θ²ρρ   0x9100
δ     θρ²ρ   0x00000000
skip_nz cursor_done

; ═════════════════════════════════════════════════
; ERASE old cursor (teal triangle)
; ═════════════════════════════════════════════════

; row addr = FB + old_y * 4096 + old_x * 4
π₇    θ²ρρ   0x9B78
δ₇    θρ¹ρ   12
π̄     θ²ρρ   0x9BA8
π₇    θ²ρρ   0x9B70
δ₇    θρ¹ρ   2
π̄δ̄   θρ²ρ·  0x9BA8
π̄     θ²ρρ   0x9BA8
π₇    θ²ρρ   0x9100
π̄δ̄   θρ²ρ·  0x9BA8
π̄     θ²ρρ   0x9BA8

π̄₇    θ²²ρ   0x9BA0  0
π̄₇    θ²²ρ   0x9BB0  1

: erase_row
π     θ²ρρ   0x9BA8
π̄     θ²ρρ   0x9BC0
π₇    θ²ρρ   0x9BB0
π̄₇    θ²ρρ   0x9BC8

: erase_px
π     θ²ρρ   0x9BC0
π̄₇   θρ²ρ   0x00008080
π     θ²ρρ   0x9BC0
π̄δ̄   θρ²ρ   4
π̄     θ²ρρ   0x9BC0
π₇    θ²ρρ   0x9BC8
π̄δ̄₃  θρ¹ρ   1
π̄₇    θ²ρρ   0x9BC8
skip_z erase_px_done
loop_back erase_px
: erase_px_done

; next row: addr += 4096, width++, row++
π     θ²ρρ   0x9BA8
π̄δ̄   θρ²ρ   0x1000
π̄     θ²ρρ   0x9BA8
π₇    θ²ρρ   0x9BB0
π̄δ̄   θρ²ρ   1
π̄₇    θ²ρρ   0x9BB0
π₇    θ²ρρ   0x9BA0
π̄δ̄   θρ²ρ   1
π̄₇    θ²ρρ   0x9BA0

; check row < 16: subtract 16, if negative (underflow) → continue
δ₁₁   θρ²ρ   0x00000010
skip_z erase_done
loop_back erase_row
: erase_done

; ═════════════════════════════════════════════════
; DRAW new cursor (white triangle)
; ═════════════════════════════════════════════════

; row addr = FB + mouse_y * 4096 + mouse_x * 4
π₇    θ²ρρ   0x9B68
δ₇    θρ¹ρ   12
π̄     θ²ρρ   0x9BA8
π₇    θ²ρρ   0x9B60
δ₇    θρ¹ρ   2
π̄δ̄   θρ²ρ·  0x9BA8
π̄     θ²ρρ   0x9BA8
π₇    θ²ρρ   0x9100
π̄δ̄   θρ²ρ·  0x9BA8
π̄     θ²ρρ   0x9BA8

π̄₇    θ²²ρ   0x9BA0  0
π̄₇    θ²²ρ   0x9BB0  1

: draw_row
π     θ²ρρ   0x9BA8
π̄     θ²ρρ   0x9BC0
π₇    θ²ρρ   0x9BB0
π̄₇    θ²ρρ   0x9BC8

: draw_px
π     θ²ρρ   0x9BC0
π̄₇   θρ²ρ   0x00FFFFFF
π     θ²ρρ   0x9BC0
π̄δ̄   θρ²ρ   4
π̄     θ²ρρ   0x9BC0
π₇    θ²ρρ   0x9BC8
π̄δ̄₃  θρ¹ρ   1
π̄₇    θ²ρρ   0x9BC8
skip_z draw_px_done
loop_back draw_px
: draw_px_done

π     θ²ρρ   0x9BA8
π̄δ̄   θρ²ρ   0x1000
π̄     θ²ρρ   0x9BA8
π₇    θ²ρρ   0x9BB0
π̄δ̄   θρ²ρ   1
π̄₇    θ²ρρ   0x9BB0
π₇    θ²ρρ   0x9BA0
π̄δ̄   θρ²ρ   1
π̄₇    θ²ρρ   0x9BA0

δ₁₁   θρ²ρ   0x00000010
skip_z draw_done
loop_back draw_row
: draw_done

; save current as old
π₇    θ²ρρ   0x9B60
π̄₇    θ²ρρ   0x9B70
π₇    θ²ρρ   0x9B68
π̄₇    θ²ρρ   0x9B78

: cursor_done
