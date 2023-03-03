#!/bin/sh

CXX="g++"
AS="as"

ASFLAGS="--32"
CXXFLAGS="-I./include -std=c++17 -Wall -Wextra"
CXXFLAGS_BOOT="$CXXFLAGS -m16 -Wno-main -ffreestanding"
CXXFLAGS_BOOT="$CXXFLAGS_BOOT -fno-exceptions -fno-rtti -O2 -Os"

echo "***** Compiling MBR *****"

$AS $ASFLAGS "mbr/abi.S" -o "mbr/abi.o"
$CXX $CXXFLAGS_BOOT -c "mbr/mbr.cpp" -o "mbr/mbr.o"

$CXX -m16 -O2 -Osize -T "mbr/mbr.ld" -o "mbr/mbr.bin" \
     -ffreestanding -nostdlib "mbr/mbr.o" "mbr/abi.o"

echo "***** Compiling VBR *****"

$AS $ASFLAGS "vbr/abi.S" -o "vbr/abi.o"
$CXX $CXXFLAGS_BOOT -c "vbr/vbr.cpp" -o "vbr/vbr.o"

$CXX -m16 -O2 -Osize -T "vbr/vbr.ld" -o "vbr/vbr.bin" \
     -ffreestanding -nostdlib "vbr/vbr.o" "vbr/abi.o"

echo "***** Compiling Stage 2 *****"

$CXX $CXXFLAGS_BOOT -c "stage2/stage2.cpp" -o "stage2/stage2.o"

$CXX -m16 -O2 -Osize -T "stage2/stage2.ld" -o "stage2/stage2.bin" \
     -ffreestanding -nostdlib "stage2/stage2.o"

echo "***** Compiling installfat *****"

$CXX $CXXFLAGS "installfat.cpp" -o "installfat"

echo "***** Building FAT Partition *****"

dd if=/dev/zero of=./fatpart.img bs=1M count=40
mkfs.fat -F 32 "./fatpart.img"
./installfat -v "vbr/vbr.bin" -o "fatpart.img" --stage2 "stage2/stage2.bin"

echo "***** Building Disk *****"

dd if=/dev/zero of=./disk.img bs=1M count=50

parted --script "./disk.img" \
	"mklabel msdos" \
	"mkpart primary fat32 1M 40M" \
	"set 1 boot on"

dd if=./mbr/mbr.bin of=./disk.img conv=notrunc bs=1 count=446
dd if=./fatpart.img of=./disk.img conv=notrunc bs=512 seek=2048
