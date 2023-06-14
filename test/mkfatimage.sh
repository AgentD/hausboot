#!/bin/sh

VBRFILE="$1"
STAGE2FILE="$2"
KERNELFILE="$3"
CFGFILE="$4"
FATEDIT="$5"
INSTALLFAT="$6"
IMGFILE="$7"

dd if=/dev/zero of="$IMGFILE" bs=1M count=40
mkfs.fat -F 32 "$IMGFILE"

"$INSTALLFAT" -v "$VBRFILE" -o "$IMGFILE" --stage2 "$STAGE2FILE"

echo "mkdir BOOT" | "$FATEDIT" "$IMGFILE"
echo "pack $KERNELFILE BOOT/KRNL386.SYS" | "$FATEDIT" "$IMGFILE"
echo "pack $CFGFILE BOOT.CFG" | "$FATEDIT" "$IMGFILE"
