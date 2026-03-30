; ucode.w — CPU microcode update
;
; Reads firmware from RAM (loaded by ata.w), finds matching
; microcode for this CPU, applies via wrmsr.
;
; Reads:
;   [0x9308] u64  firmware RAM address (0 = no firmware)
;   [0x9310] u32  firmware size
;
; Firmware format (from pack.sh):
;   [0:3]   u32  magic 0x46574D43 ("FWMC")
;   [4:7]   u32  entry count
;   entries[]:
;     [0]   u8   vendor (0=AMD, 1=Intel)
;     [1]   u8   family
;     [2]   u8   model (0xFF = any)
;     [3]   u8   stepping (0xFF = any)
;     [4:7] u32  offset (from file start)
;     [8:11] u32 size
;
; AMD microcode: write blob physical address to MSR 0xC0010020
; Intel microcode: write (blob_addr + 48) to MSR 0x79
;   (Intel blob has 48-byte header, data starts at +48)
;
; CPUID leaf 1 EAX:
;   [3:0]   stepping
;   [7:4]   model
;   [11:8]  family (base)
;   [19:16] extended model
;   [27:20] extended family
;
;   AMD: effective_family = base + ext (base is always 0x0F for Zen)
;   Intel: family = 0x06, effective_model = (ext_model << 4) | model
;
; Scratch:
;   [0x95A0] u32  CPUID signature (leaf 1 EAX)
;   [0x95A4] u32  effective family
;   [0x95A8] u32  effective model
;   [0x95AC] u32  stepping
;   [0x95B0] u64  firmware base address
;   [0x95B8] u32  entry count
;   [0x95C0] u64  current entry pointer
;   [0x95C8] u32  entries remaining


; skip if no firmware in RAM
π     θ²ρρ  0x9308                   ; fw RAM address (qword)
δ      θ²ρρ  0x00000000
skip_nz ucode_end                    ; address == 0 → skip

; verify magic
π     θ²ρρ  0x9308                   ; fw base addr
π₇    θρρρ                           ; read dword at [fw_base]
δ      θ²ρρ  0x46574D43              ; == "FWMC"?
skip_z ucode_end                     ; bad magic → skip

; read entry count
π     θ²ρρ  0x9308                   ; fw base
π̄δ̄    θρ²ρ  0x00000004              ; +4
π₇    θρρρ                           ; entry count
π̄      θ²ρρ  0x95B8                   ; save

; save fw base
π     θ²ρρ  0x9308
π̄      θ²ρρ  0x95B0

; compute entry table pointer = fw_base + 8
π     θ²ρρ  0x9308
π̄δ̄    θρ²ρ  0x00000008
π̄      θ²ρρ  0x95C0                   ; current entry ptr

; entry count → remaining
π₇    θ²ρρ  0x95B8
π̄      θ²ρρ  0x95C8

; ── get CPUID ────────────────────────────────────────────────

πδ̄₅   θ²ρρ  0x00000001              ; CPUID leaf 1 → EAX
π̄      θ²ρρ  0x95A0                   ; save signature

; extract family (bits 11:8)
δ̄₇     θρ¹ρ  8
δ₃     θρ²ρ  0x0000000F              ; base family
π̄      θ²ρρ  0x95A4                   ; save (will be overwritten if extended)

; check if extended family needed (base family >= 0x0F)
π₇    θ²ρρ  0x95A4
δ      θ¹ρρ  0x0F
skip_nz ucode_ext_fam
skip_z ucode_model                   ; base family < 0x0F → use as-is
skip_nz ucode_model

: ucode_ext_fam
; effective family = base(0x0F) + extended_family(bits 27:20)
π₇    θ²ρρ  0x95A0
δ̄₇     θρ¹ρ  20                      ; >> 20
δ₃     θρ²ρ  0x000000FF              ; ext family
π̄δ̄    θρ¹ρ  0x0F                    ; + 0x0F = effective family
π̄      θ²ρρ  0x95A4

: ucode_model
; extract model: bits 7:4 (base) + bits 19:16 (extended, if family >= 6)
π₇    θ²ρρ  0x95A0
δ̄₇     θρ¹ρ  4
δ₃     θρ²ρ  0x0000000F              ; base model
π̄      θ²ρρ  0x95A8

; extended model (bits 19:16) — add for Intel (family 6) and AMD (family >= 0x0F)
π₇    θ²ρρ  0x95A0
δ̄₇     θρ¹ρ  12                      ; >> 12 (bits 19:16 → bits 7:4)
δ₃     θρ²ρ  0x000000F0              ; ext model << 4
π̄δ̄    θ²ρρ·  0x95A8                  ; + base model (deref)
π̄      θ²ρρ  0x95A8                   ; save effective model

; extract stepping: bits 3:0
π₇    θ²ρρ  0x95A0
δ₃     θρ²ρ  0x0000000F
π̄      θ²ρρ  0x95AC

; ── scan firmware index ──────────────────────────────────────

: ucode_scan
π₇    θ²ρρ  0x95C8                   ; remaining
δ      θ²ρρ  0x00000000
skip_nz ucode_end                    ; no more entries

; read entry: vendor(u8), family(u8), model(u8), stepping(u8)
; packed as one dword at entry_ptr
π     θ²ρρ  0x95C0                   ; entry ptr
π₇    θρρρ                           ; read dword → pipeline
π̄      θ²ρρ  0x95D0                   ; save entry dword

; extract vendor (byte 0)
δ₃     θρ²ρ  0x000000FF
π̄      θ²ρρ  0x95D4                   ; save vendor

; extract entry family (byte 1)
π₇    θ²ρρ  0x95D0
δ̄₇     θρ¹ρ  8
δ₃     θρ²ρ  0x000000FF
π̄      θ²ρρ  0x95D8                   ; save entry family

; match family
π₇    θ²ρρ  0x95A4                   ; our effective family
δ      θ²ρρ·  0x95D8                  ; == entry family?
skip_nz ucode_match_model
skip_z ucode_next
skip_nz ucode_next

: ucode_match_model
; extract entry model (byte 2)
π₇    θ²ρρ  0x95D0
δ̄₇     θρ¹ρ  16
δ₃     θρ²ρ  0x000000FF

; check if model == 0xFF (wildcard)
δ      θ²ρρ  0x000000FF
skip_nz ucode_found                  ; wildcard → match (AMD blobs cover all models)

; exact model match
π₇    θ²ρρ  0x95D0
δ̄₇     θρ¹ρ  16
δ₃     θρ²ρ  0x000000FF
π̄      θ²ρρ  0x95DC

π₇    θ²ρρ  0x95A8                   ; our model
δ      θ²ρρ·  0x95DC                  ; == entry model?
skip_nz ucode_check_step
skip_z ucode_next
skip_nz ucode_next

: ucode_check_step
; extract entry stepping (byte 3)
π₇    θ²ρρ  0x95D0
δ̄₇     θρ¹ρ  24
δ₃     θρ²ρ  0x000000FF

; wildcard?
δ      θ²ρρ  0x000000FF
skip_nz ucode_found

; exact stepping match
π₇    θ²ρρ  0x95D0
δ̄₇     θρ¹ρ  24
δ₃     θρ²ρ  0x000000FF
π̄      θ²ρρ  0x95E0

π₇    θ²ρρ  0x95AC                   ; our stepping
δ      θ²ρρ·  0x95E0
skip_nz ucode_found
skip_z ucode_next
skip_nz ucode_next

: ucode_next
; advance to next entry (+12 bytes per entry)
π     θ²ρρ  0x95C0
π̄δ̄    θρ²ρ  0x0000000C
π̄      θ²ρρ  0x95C0

; decrement remaining
π₇    θ²ρρ  0x95C8
π̄δ̄₃   θρ¹ρ  1
π̄      θ²ρρ  0x95C8
skip_z ucode_end                     ; exhausted → no match
π₇    θ²ρρ  0x95C8                   ; reload non-zero for loop_back
loop_back ucode_scan

: ucode_found
; read blob offset and size from entry
π     θ²ρρ  0x95C0                   ; entry ptr
π̄δ̄    θρ²ρ  0x00000004              ; +4 = offset field
π₇    θρρρ                           ; blob offset
π̄δ̄    θ²ρρ·  0x95B0                  ; + fw_base = blob physical address
π̄      θ²ρρ  0x95E8                   ; save blob addr

; read blob size
π     θ²ρρ  0x95C0
π̄δ̄    θρ²ρ  0x00000008
π₇    θρρρ
π̄      θ²ρρ  0x95EC                   ; save blob size

; determine vendor and apply
π₇    θ²ρρ  0x95D4                   ; vendor
δ      θ²ρρ  0x00000000              ; == 0 (AMD)?
skip_nz ucode_amd
; not AMD → Intel
skip_z ucode_intel
skip_nz ucode_intel

: ucode_amd
; AMD: write blob physical address to MSR 0xC0010020
π     θ²ρρ  0x95E8                   ; blob addr
π̄δ₃   θ²ρρ  0xC0010020              ; wrmsr(0xC0010020, blob_addr)
skip_z ucode_done
skip_nz ucode_done

: ucode_intel
; Intel: write (blob_addr + 48) to MSR 0x79
; Intel microcode blob has 48-byte header, update data starts at +48
π     θ²ρρ  0x95E8
π̄δ̄    θρ²ρ  0x00000030              ; + 48
π̄δ₃   θ²ρρ  0x00000079              ; wrmsr(0x79, data_addr)

: ucode_done
; microcode applied

: ucode_end
