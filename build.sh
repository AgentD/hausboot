#!/bin/sh

CXX="g++"

CXXFLAGS="-I./include -std=c++17 -Wall -Wextra"
CXXFLAGS_BOOT="$CXXFLAGS -m16 -Wno-main -ffreestanding"
CXXFLAGS_BOOT="$CXXFLAGS_BOOT -fno-exceptions -fno-rtti -O2 -Os"

mkdir -p out

echo "***** Compiling MBR *****"

$CXX $CXXFLAGS_BOOT -c "mbr.cpp" -o "out/mbr.o"

$CXX -m16 -O2 -Osize -T "mbr.ld" -o "out/mbr" \
     -ffreestanding -nostdlib "out/mbr.o"

echo "***** Compiling VBR *****"

$CXX $CXXFLAGS_BOOT -c "vbr.cpp" -o "out/vbr.o"

$CXX -m16 -O2 -Osize -T "vbr.ld" -o "out/vbr" \
     -ffreestanding -nostdlib "out/vbr.o"

echo "***** Compiling installfat *****"

$CXX $CXXFLAGS "installfat.cpp" -o "out/installfat"

echo "***** Building FAT Partition *****"

dd if=/dev/zero of=./out/fatpart.img bs=1M count=40
mkfs.fat -F 32 "./out/fatpart.img"
./out/installfat -v "out/vbr" -o "out/fatpart.img"

echo "***** Building Disk *****"

dd if=/dev/zero of=./out/disk.img bs=1M count=50

parted --script "./out/disk.img" \
	"mklabel msdos" \
	"mkpart primary fat32 1M 40M" \
	"set 1 boot on"

dd if=./out/mbr of=./out/disk.img conv=notrunc bs=1 count=446
dd if=./out/fatpart.img of=./out/disk.img conv=notrunc bs=512 seek=2048
