#!/bin/sh

CXX="g++"
AS="as"

ASFLAGS="--32"
CXXFLAGS="-I./include -std=c++17 -Wall -Wextra"
CXXFLAGS_BOOT="$CXXFLAGS -m16 -Wno-main -ffreestanding"
CXXFLAGS_BOOT="$CXXFLAGS_BOOT -fno-exceptions -fno-rtti -O2 -Os"

mkdir -p out

echo "***** Compiling MBR *****"

$AS $ASFLAGS "mbr_ABI.S" -o "out/mbr_ABI.o"
$CXX $CXXFLAGS_BOOT -c "mbr.cpp" -o "out/mbr.o"

$CXX -m16 -O2 -Osize -T "mbr.ld" -o "out/mbr" \
     -ffreestanding -nostdlib "out/mbr.o" "out/mbr_ABI.o"

echo "***** Compiling VBR *****"

$AS $ASFLAGS "vbr_ABI.S" -o "out/vbr_ABI.o"
$CXX $CXXFLAGS_BOOT -c "vbr.cpp" -o "out/vbr.o"

$CXX -m16 -O2 -Osize -T "vbr.ld" -o "out/vbr" \
     -ffreestanding -nostdlib "out/vbr.o" "out/vbr_ABI.o"

echo "***** Compiling Stage 2 *****"

$CXX $CXXFLAGS_BOOT -c "stage2.cpp" -o "out/stage2.o"

$CXX -m16 -O2 -Osize -T "stage2.ld" -o "out/stage2" \
     -ffreestanding -nostdlib "out/stage2.o"

echo "***** Compiling installfat *****"

$CXX $CXXFLAGS "installfat.cpp" -o "out/installfat"

echo "***** Building FAT Partition *****"

dd if=/dev/zero of=./out/fatpart.img bs=1M count=40
mkfs.fat -F 32 "./out/fatpart.img"
./out/installfat -v "out/vbr" -o "out/fatpart.img" --stage2 "out/stage2"

echo "***** Building Disk *****"

dd if=/dev/zero of=./out/disk.img bs=1M count=50

parted --script "./out/disk.img" \
	"mklabel msdos" \
	"mkpart primary fat32 1M 40M" \
	"set 1 boot on"

dd if=./out/mbr of=./out/disk.img conv=notrunc bs=1 count=446
dd if=./out/fatpart.img of=./out/disk.img conv=notrunc bs=512 seek=2048
