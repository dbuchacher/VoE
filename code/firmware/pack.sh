#!/bin/bash
# Pack CPU microcode blobs into a single firmware image
#
# Format:
#   Header:
#     u32 magic = 0x46574D43 ("FWMC" = firmware microcode)
#     u32 entry_count
#     entries[]:
#       u8  vendor (0=AMD, 1=Intel)
#       u8  family
#       u8  model
#       u8  stepping (0xFF = any)
#       u32 offset (from start of file)
#       u32 size
#   Padding to 512-byte boundary
#   Blob data (concatenated)
#
# AMD blobs: vendor=0, family from filename, model=0xFF (blob covers all models)
# Intel blobs: vendor=1, family/model/stepping from filename (06-XX-YY)

set -e
cd "$(dirname "$0")"

OUT=cpu_firmware.bin
HEADER=$(mktemp)
BLOBS=$(mktemp)

# Count entries
NFILES=$(ls cpu/*.bin cpu/intel-* 2>/dev/null | wc -l)

# Header: magic + count (8 bytes)
printf '\x43\x4D\x57\x46' > "$HEADER"  # FWMC
printf "$(printf '\\x%02x\\x%02x\\x%02x\\x%02x' \
  $((NFILES & 0xFF)) $(((NFILES >> 8) & 0xFF)) \
  $(((NFILES >> 16) & 0xFF)) $(((NFILES >> 24) & 0xFF)))" >> "$HEADER"

# Each entry = 12 bytes. Header = 8 + 12*N bytes
HEADER_SIZE=$((8 + 12 * NFILES))
# Pad to 512-byte boundary
HEADER_PAD=$(( ((HEADER_SIZE + 511) / 512) * 512 ))
DATA_OFFSET=$HEADER_PAD

# Process AMD blobs
for f in cpu/amd-*.bin; do
  [ -f "$f" ] || continue
  NAME=$(basename "$f")
  SIZE=$(stat -c%s "$f")

  case "$NAME" in
    amd-fam17h.bin) FAMILY=0x17 ;;
    amd-fam19h.bin) FAMILY=0x19 ;;
    amd-fam1ah.bin) FAMILY=0x1A ;;
    amd-legacy.bin) FAMILY=0x0F ;;
    *) continue ;;
  esac

  # vendor=0 (AMD), family, model=0xFF (any), stepping=0xFF (any)
  printf "$(printf '\\x00\\x%02x\\xFF\\xFF' $FAMILY)" >> "$HEADER"
  # offset (u32 LE)
  printf "$(printf '\\x%02x\\x%02x\\x%02x\\x%02x' \
    $((DATA_OFFSET & 0xFF)) $(((DATA_OFFSET >> 8) & 0xFF)) \
    $(((DATA_OFFSET >> 16) & 0xFF)) $(((DATA_OFFSET >> 24) & 0xFF)))" >> "$HEADER"
  # size (u32 LE)
  printf "$(printf '\\x%02x\\x%02x\\x%02x\\x%02x' \
    $((SIZE & 0xFF)) $(((SIZE >> 8) & 0xFF)) \
    $(((SIZE >> 16) & 0xFF)) $(((SIZE >> 24) & 0xFF)))" >> "$HEADER"

  cat "$f" >> "$BLOBS"
  DATA_OFFSET=$((DATA_OFFSET + SIZE))
done

# Process Intel blobs
for f in cpu/intel-*; do
  [ -f "$f" ] || continue
  NAME=$(basename "$f")
  SIZE=$(stat -c%s "$f")

  # Parse filename: intel-06-XX-YY
  FAMILY=$(echo "$NAME" | sed 's/intel-\([0-9a-f]*\)-.*/\1/')
  MODEL=$(echo "$NAME" | sed 's/intel-[0-9a-f]*-\([0-9a-f]*\)-.*/\1/')
  STEP=$(echo "$NAME" | sed 's/intel-[0-9a-f]*-[0-9a-f]*-\([0-9a-f]*\)/\1/')

  FAMILY=$((16#$FAMILY))
  MODEL=$((16#$MODEL))
  STEP=$((16#$STEP))

  # vendor=1 (Intel), family, model, stepping
  printf "$(printf '\\x01\\x%02x\\x%02x\\x%02x' $FAMILY $MODEL $STEP)" >> "$HEADER"
  printf "$(printf '\\x%02x\\x%02x\\x%02x\\x%02x' \
    $((DATA_OFFSET & 0xFF)) $(((DATA_OFFSET >> 8) & 0xFF)) \
    $(((DATA_OFFSET >> 16) & 0xFF)) $(((DATA_OFFSET >> 24) & 0xFF)))" >> "$HEADER"
  printf "$(printf '\\x%02x\\x%02x\\x%02x\\x%02x' \
    $((SIZE & 0xFF)) $(((SIZE >> 8) & 0xFF)) \
    $(((SIZE >> 16) & 0xFF)) $(((SIZE >> 24) & 0xFF)))" >> "$HEADER"

  cat "$f" >> "$BLOBS"
  DATA_OFFSET=$((DATA_OFFSET + SIZE))
done

# Pad header to boundary
CURRENT=$(stat -c%s "$HEADER")
PAD=$((HEADER_PAD - CURRENT))
if [ $PAD -gt 0 ]; then
  dd if=/dev/zero bs=1 count=$PAD 2>/dev/null >> "$HEADER"
fi

# Combine
cat "$HEADER" "$BLOBS" > "$OUT"
rm "$HEADER" "$BLOBS"

TOTAL=$(stat -c%s "$OUT")
SECTORS=$(( (TOTAL + 511) / 512 ))
echo "firmware: $NFILES blobs, $TOTAL bytes ($SECTORS sectors)"
