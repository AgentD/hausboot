LDFLAGS := -m16 -O2 -Osize -ffreestanding -nostdlib -march=i386
ASFLAGS := --32

CXXFLAGS := -I../include -std=c++17 -Wall -Wextra -m16 -Wno-main \
	-ffreestanding -fno-exceptions -fno-rtti -O2 -Os -march=i386 \
	-momit-leaf-frame-pointer -fno-threadsafe-statics

HEADERS := ../include/BIOS/BiosDisk.h ../include/BIOS/TextScreen.h \
	../include/part/MBRTable.h ../include/part/MBREntry.h \
	../include/part/CHSPacked.h ../include/UnalignedInt.h \
	../include/FlagField.h

mbr.bin: mbr.ld abi.o mbr.o
	$(CXX) $(LDFLAGS) -T $< -o $@ $(filter-out $<,$^)

abi.o: abi.S
	$(AS) $(ASFLAGS) $^ -o $@

mbr.o: mbr.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) *.o *.bin
