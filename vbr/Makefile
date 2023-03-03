LDFLAGS := -m16 -O2 -Osize -ffreestanding -nostdlib
ASFLAGS := --32

CXXFLAGS := -I../include -std=c++17 -Wall -Wextra -m16 -Wno-main \
	-ffreestanding -fno-exceptions -fno-rtti -O2 -Os

HEADERS := ../include/bios.h ../include/CHSPacked.h ../include/video.h \
	../include/MBREntry.h ../include/UnalignedInt.h \
	../include/FlagField.h ../include/FatSuper.h \
	../include/FixedLengthString.h ../include/ByteBlob.h \
	../include/stage2.h

vbr.bin: vbr.ld abi.o vbr.o
	$(CXX) $(LDFLAGS) -T $< -o $@ $(filter-out $<,$^)

abi.o: abi.S
	$(AS) $(ASFLAGS) $^ -o $@

vbr.o: vbr.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) *.o *.bin
