LDFLAGS := -m16 -O2 -Osize -ffreestanding -nostdlib -march=i386
ASFLAGS := --32

CXXFLAGS := -I../include -std=c++17 -Wall -Wextra -m16 -Wno-main \
	-ffreestanding -fno-exceptions -fno-rtti -O2 -Os -march=i386 \
	-momit-leaf-frame-pointer -fno-threadsafe-statics

HEADERS := ../include/part/CHSPacked.h ../include/BIOS/BIOSTextMode.h \
	../include/part/MBREntry.h ../include/types/UnalignedInt.h \
	../include/types/FlagField.h ../include/fs/FatSuper.h \
	../include/types/FixedLengthString.h ../include/types/ByteBlob.h \
	../include/Stage2Header.h ../include/BIOS/BiosDisk.h

vbr.bin: vbr.ld abi.o vbr.o
	$(CXX) $(LDFLAGS) -T $< -o $@ $(filter-out $<,$^)

abi.o: abi.S
	$(AS) $(ASFLAGS) $^ -o $@

vbr.o: vbr.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) *.o *.bin
