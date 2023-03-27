LDFLAGS := -m32 -O2 -Osize -ffreestanding -nostdlib -march=i386
ASFLAGS := --32

CXXFLAGS := -I../include -std=c++17 -Wall -Wextra -m32 -Wno-main \
	-ffreestanding -fno-exceptions -fno-rtti -O2 -Os -march=i386 \
	-momit-leaf-frame-pointer

HEADERS := ../include/multiboot.h ../include/UnalignedInt.h \
	../include/FlagField.h ../include/BIOS/MemoryMap.h

KRNL386.SYS: kernel.ld kernel.o abi.o
	$(CXX) $(LDFLAGS) -T $< -o $@ $(filter-out $<,$^)

kernel.o: kernel.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

abi.o: abi.S
	$(AS) $(ASFLAGS) $^ -o $@

.PHONY: clean
clean:
	$(RM) *.o *.SYS
