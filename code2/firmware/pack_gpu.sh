#!/bin/bash
# Pack GPU firmware blobs into a single indexed image
#
# Format:
#   Global header:
#     u32 magic = 0x46574750 ("FWGP")
#     u32 device_count
#   Device entries (16 bytes each):
#     u16 vendor_id
#     u16 device_id
#     u32 file_count
#     u32 dir_offset  (absolute — points to file directory)
#     u32 total_size  (total bytes for this device)
#   File directory (at dir_offset, 12 bytes per entry):
#     u32 tag         (4 ASCII chars identifying the file)
#     u32 data_offset (absolute from image start)
#     u32 data_size
#   Padding to 512-byte boundary
#   File data (concatenated)
#
# Tags are assigned by filename. The walk searches by tag.

set -e
cd "$(dirname "$0")"

OUT=gpu_firmware.bin

# write 4-byte tag for filename directly to file (avoids null byte issues in bash)
write_tag() {
    local name="${1%.bin}"
    case "$name" in
        bl)               printf 'bl__' ;;
        booter_load)      printf 'bolo' ;;
        booter_unload)    printf 'boul' ;;
        bootloader)       printf 'boot' ;;
        gsp)              printf 'gsp_' ;;
        desc)             printf 'desc' ;;
        image)            printf 'imgs' ;;
        sig)              printf 'sig_' ;;
        fecs_bl)          printf 'febl' ;;
        fecs_data)        printf 'feda' ;;
        fecs_inst)        printf 'fein' ;;
        fecs_sig)         printf 'fesi' ;;
        gpccs_bl)         printf 'gpbl' ;;
        gpccs_data)       printf 'gpda' ;;
        gpccs_inst)       printf 'gpin' ;;
        gpccs_sig)        printf 'gpsi' ;;
        sw_bundle_init)   printf 'swbi' ;;
        sw_ctx)           printf 'swcx' ;;
        sw_method_init)   printf 'swmi' ;;
        sw_nonctx)        printf 'swnc' ;;
        sw_veid_bundle_init) printf 'swvb' ;;
        ucode_ahesasc)    printf 'ucah' ;;
        ucode_asb)        printf 'ucas' ;;
        ucode_unload)     printf 'ucul' ;;
        unload_bl)        printf 'ulbl' ;;
        *)                printf '%.4s' "$name" ;;
    esac
}

# write u16 little-endian
w16() { printf "$(printf '\\x%02x\\x%02x' $(($1 & 0xFF)) $((($1 >> 8) & 0xFF)))"; }
# write u32 little-endian
w32() { printf "$(printf '\\x%02x\\x%02x\\x%02x\\x%02x' \
    $(($1 & 0xFF)) $((($1 >> 8) & 0xFF)) $((($1 >> 16) & 0xFF)) $((($1 >> 24) & 0xFF)))"; }

# Count GPU directories
NDIRS=0
for d in gpu/*/; do [ -d "$d" ] && NDIRS=$((NDIRS + 1)); done
if [ $NDIRS -eq 0 ]; then echo "no GPU firmware found"; exit 0; fi

# ── compute layout ──────────────────────────────────────────

# Header: magic(4) + count(4) + devices(16 each)
HEADER_FIXED=$((8 + 16 * NDIRS))

# Collect file info per device
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

TOTAL_DIR_SIZE=0
DEVI=0
for d in gpu/*/; do
    [ -d "$d" ] || continue
    DIRNAME=$(basename "$d")

    # Count files
    NFILES=0
    for f in "$d"*.bin; do [ -f "$f" ] && NFILES=$((NFILES + 1)); done

    echo "$NFILES" > "$TMPDIR/dev${DEVI}_nfiles"
    echo "$DIRNAME" > "$TMPDIR/dev${DEVI}_name"

    # Directory for this device: 12 bytes per file
    TOTAL_DIR_SIZE=$((TOTAL_DIR_SIZE + 12 * NFILES))
    DEVI=$((DEVI + 1))
done

# Total header area (before padding)
HEADER_TOTAL=$((HEADER_FIXED + TOTAL_DIR_SIZE))
HEADER_PAD=$(( ((HEADER_TOTAL + 511) / 512) * 512 ))

# ── write output ────────────────────────────────────────────

OUTFILE=$(mktemp)

# Global header
{ w32 0x46574750; w32 $NDIRS; } > "$OUTFILE"

# First pass: compute dir_offsets and data offsets
DIR_OFF=$HEADER_FIXED  # directories start right after device entries
DATA_OFF=$HEADER_PAD   # file data starts after padded header

DEVI=0
for d in gpu/*/; do
    [ -d "$d" ] || continue
    DIRNAME=$(basename "$d")
    NFILES=$(cat "$TMPDIR/dev${DEVI}_nfiles")

    case "$DIRNAME" in
        nvidia-tu102) VENDOR=0x10DE; DEVICE=0x1E04 ;;
        nvidia-ga102) VENDOR=0x10DE; DEVICE=0x2204 ;;
        nvidia-ad102) VENDOR=0x10DE; DEVICE=0x2684 ;;
        amd-*)        VENDOR=0x1002; DEVICE=0xFFFF ;;
        *)            VENDOR=0x0000; DEVICE=0x0000 ;;
    esac

    # Compute total size of this device's files
    DEV_TOTAL=0
    for f in "$d"*.bin; do
        [ -f "$f" ] || continue
        FSIZE=$(stat -c%s "$f")
        DEV_TOTAL=$((DEV_TOTAL + FSIZE))
    done

    # Write device entry: vendor(2) + device(2) + file_count(4) + dir_offset(4) + total_size(4)
    { w16 $VENDOR; w16 $DEVICE; w32 $NFILES; w32 $DIR_OFF; w32 $DEV_TOTAL; } >> "$OUTFILE"

    # Save dir_offset and data_off for second pass
    echo "$DIR_OFF" > "$TMPDIR/dev${DEVI}_diroff"
    echo "$DATA_OFF" > "$TMPDIR/dev${DEVI}_dataoff"

    DIR_OFF=$((DIR_OFF + 12 * NFILES))
    DATA_OFF=$((DATA_OFF + DEV_TOTAL))
    DEVI=$((DEVI + 1))
done

# Second pass: write file directories
DEVI=0
for d in gpu/*/; do
    [ -d "$d" ] || continue
    NFILES=$(cat "$TMPDIR/dev${DEVI}_nfiles")
    FILE_DATA_OFF=$(cat "$TMPDIR/dev${DEVI}_dataoff")

    for f in "$d"*.bin; do
        [ -f "$f" ] || continue
        FNAME=$(basename "$f")
        FSIZE=$(stat -c%s "$f")

        # Write file entry: tag(4) + offset(4) + size(4)
        write_tag "$FNAME" >> "$OUTFILE"
        { w32 $FILE_DATA_OFF; w32 $FSIZE; } >> "$OUTFILE"

        FILE_DATA_OFF=$((FILE_DATA_OFF + FSIZE))
    done
    DEVI=$((DEVI + 1))
done

# Pad header to 512 boundary
CURRENT=$(stat -c%s "$OUTFILE")
PAD=$((HEADER_PAD - CURRENT))
[ $PAD -gt 0 ] && dd if=/dev/zero bs=1 count=$PAD 2>/dev/null >> "$OUTFILE"

# Write file data
for d in gpu/*/; do
    [ -d "$d" ] || continue
    for f in "$d"*.bin; do
        [ -f "$f" ] && cat "$f" >> "$OUTFILE"
    done
done

mv "$OUTFILE" "$OUT"
TOTAL=$(stat -c%s "$OUT")
SECTORS=$(( (TOTAL + 511) / 512 ))
echo "gpu firmware: $NDIRS devices, $TOTAL bytes ($SECTORS sectors)"
