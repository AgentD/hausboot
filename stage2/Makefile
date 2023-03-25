LDFLAGS := -m16 -O2 -Osize -ffreestanding -nostdlib -march=i386
ASFLAGS := --32

CXXFLAGS := -I../include -std=c++17 -Wall -Wextra -m16 -Wno-main \
	-ffreestanding -fno-exceptions -fno-rtti -O2 -Os -march=i386 \
	-momit-leaf-frame-pointer

HEADERS := ../include/BIOS/TextScreen.h ../include/stage2.h \
	../include/part/MBREntry.h ../include/part/CHSPacked.h \
	../include/UnalignedInt.h ../include/FlagField.h \
	../include/BIOS/BiosDisk.h ../include/fs/FatSuper.h \
	../include/FixedLengthString.h ../include/ByteBlob.h

stage2.bin: stage2.ld abi.o stage2.o
	$(CXX) $(LDFLAGS) -T $< -o $@ $(filter-out $<,$^)

abi.o: abi.S
	$(AS) $(ASFLAGS) $^ -o $@

stage2.o: stage2.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) *.o *.bin
