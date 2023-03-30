LDFLAGS := -m16 -O2 -Osize -ffreestanding -nostdlib -march=i386
ASFLAGS := --32

CXXFLAGS := -I../include -std=c++17 -Wall -Wextra -m16 -Wno-main \
	-ffreestanding -fno-exceptions -fno-rtti -O2 -Os -march=i386 \
	-momit-leaf-frame-pointer -fno-threadsafe-statics

HEADERS := ../include/BIOS/TextScreen.h ../include/stage2/Stage2Info.h \
	../include/part/MBREntry.h ../include/part/CHSPacked.h \
	../include/UnalignedInt.h ../include/FlagField.h \
	../include/BIOS/BiosDisk.h ../include/fs/FatSuper.h \
	../include/FixedLengthString.h ../include/ByteBlob.h \
	../include/fs/FatDirent.h ../include/fs/FatDirentLong.h \
	../include/fs/FatFs.h ../include/Memory.h \
	../include/BIOS/MemoryMap.h ../include/fs/FatName.h \
	../include/StringUtil.h ../include/IBlockDevice.h \
	../include/BIOS/BIOSBlockDevice.h ../include/pm86.h \
	../include/kernel/MultiBootHeader.h

stage2.bin: stage2.ld abi.o heap.o stage2.o ../lib/BIOS/libBIOS.a \
	../lib/pm86/libpm86.a ../lib/cxxabi/libcxxabi.a
	$(CXX) $(LDFLAGS) -T $< -o $@ $(filter-out $<,$^)

abi.o: abi.S
	$(AS) $(ASFLAGS) $^ -o $@

stage2.o: stage2.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

heap.o: heap.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) *.o *.bin
