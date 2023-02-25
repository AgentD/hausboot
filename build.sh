#!/bin/sh

CXX="g++"

CXXFLAGS="-I./include -m16 -std=c++17 -Wall -Wextra -Wno-main -ffreestanding"
CXXFLAGS="$CXXFLAGS -fno-exceptions -fno-rtti -O2 -Os"

mkdir -p out

echo "***** Compiling MBR *****"

$CXX $CXXFLAGS -c "mbr.cpp" -o "out/mbr.o"

$CXX -m16 -O2 -Osize -T "mbr.ld" -o "out/mbr" \
     -ffreestanding -nostdlib "out/mbr.o"

echo "***** Compiling VBR *****"

$CXX $CXXFLAGS -c "vbr.cpp" -o "out/vbr.o"

$CXX -m16 -O2 -Osize -T "vbr.ld" -o "out/vbr" \
     -ffreestanding -nostdlib "out/vbr.o"

echo "***** Building Disk *****"

dd if=/dev/zero of=./out/disk.img bs=1M count=32

parted --script "./out/disk.img" \
	"mklabel msdos" \
	"mkpart primary fat32 1M 100%" \
	"set 1 boot on"

dd if=./out/mbr of=./out/disk.img conv=notrunc bs=1 count=446
dd if=./out/vbr of=./out/disk.img conv=notrunc bs=512 count=1 seek=2048
