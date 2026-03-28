; pci.asm — PCI config space probe macro
;
; Discovers NVMe drives by reading PCI config registers.
; PCI config space is accessed via two x86 I/O ports:
;   Port 0xCF8 = address register (write: which device/register to read)
;   Port 0xCFC = data register (read: the value at that address)
;
; PCI address format (32-bit dword written to 0xCF8):
;   bit 31 = enable (must be 1)
;   bits 15:11 = device number (0-31 on bus 0)
;   bits 7:2 = register offset (which 4-byte register)
;
; Register 0x08 = class code (identifies device type).
;   NVMe class = 0x010802 (mass storage, NVM, NVMe interface).
; Register 0x10 = BAR0 (base address register — where the device's memory lives).
;   Low 4 bits are type flags, mask them off with AND 0xFFFFFFF0.
;
; This macro probes ONE device slot. Genesis calls it 4 times (devices 3-6).
; If any slot is NVMe, it reads BAR0 and stores it in nvme_bar.
; Not a bus scan — targeted reads of known slots. 2 bonds per slot.

%macro pci_probe_dev 1                     ; %1 = device number (0-31)

    ; --- write PCI address for class register (0x08) ---
    w -5,0,0,+1                            ; port_write dword (|T|=5 = 32-bit I/O)
    db F(U32,U32,P)
    dd 0xCF8                               ;   address port
    dd 0x80000008 | (%1 << 11)             ;   enable + device %1 + register 0x08

    ; --- read class dword back ---
    w +5,0,0,-1                            ; port_read dword (|T|=5 = 32-bit I/O)
    db F(U32,P,P)
    dd 0xCFC                               ;   data port → pipeline = class dword

    ; --- shift right 8 to drop the revision byte ---
    ; class dword = [class:subclass:progif:revision], we want top 3 bytes
    w 0,0,0,-7                             ; SHR (-W at magnitude 7)
    db F(P,U8,P), 8                        ;   shift pipeline right by 8 bits

    ; --- compare against NVMe class code ---
    w 0,0,0,+1                             ; test: pipeline == 0x010802?
    db F(P,U32,P)                          ;   pipeline = 1 if match, 0 if not
    dd 0x010802

    ; --- if not NVMe, skip the BAR read ---
    skip_z (%%next - %%bar)
%%bar:

    ; --- found NVMe! read BAR0 (register 0x10) ---
    w -5,0,0,+1                            ; port_write dword: set address to BAR0
    db F(U32,U32,P)
    dd 0xCF8
    dd 0x80000010 | (%1 << 11)             ;   enable + device %1 + register 0x10

    w +5,0,0,-1                            ; port_read dword: read BAR0 → pipeline
    db F(U32,P,P)
    dd 0xCFC

    ; --- mask off BAR type bits (low 4 bits are flags, not address) ---
    w 0,0,0,+3                             ; AND (+W at magnitude 3)
    db F(P,U32,P)
    dd 0xFFFFFFF0                           ;   pipeline &= 0xFFFFFFF0

    ; --- store the clean BAR address ---
    w -1,0,0,0                             ; write: nvme_bar = pipeline
    db F(U32,P,P)
    dd nvme_bar
%%next:
%endmacro
