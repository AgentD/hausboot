#!/bin/sh

FATPART="$1"
MBRFILE="$2"
OUTFILE="$3"

dd if=/dev/zero of="$OUTFILE" bs=1M count=50

parted --script "$OUTFILE" \
	"mklabel msdos" \
	"mkpart primary fat32 1M 40M" \
	"set 1 boot on"

dd if="$MBRFILE" of="$OUTFILE" conv=notrunc bs=1 count=446
dd if="$FATPART" of="$OUTFILE" conv=notrunc bs=512 seek=2048

