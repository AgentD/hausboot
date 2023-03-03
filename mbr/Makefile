LDFLAGS := -m16 -O2 -Osize -ffreestanding -nostdlib
ASFLAGS := --32

CXXFLAGS := -I../include -std=c++17 -Wall -Wextra -m16 -Wno-main \
	-ffreestanding -fno-exceptions -fno-rtti -O2 -Os

HEADERS := ../include/bios.h ../include/video.h ../include/MBRTable.h \
	../include/MBREntry.h ../include/UnalignedInt.h \
	../include/FlagField.h ../include/CHSPacked.h

mbr.bin: mbr.ld abi.o mbr.o
	$(CXX) $(LDFLAGS) -T $< -o $@ $(filter-out $<,$^)

abi.o: abi.S
	$(AS) $(ASFLAGS) $^ -o $@

mbr.o: mbr.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) *.o *.bin
