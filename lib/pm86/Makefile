LDFLAGS := -m16 -O2 -Osize -ffreestanding -nostdlib -march=i386
ASFLAGS := --32
CXXFLAGS := -I../../include -std=c++17 -Wall -Wextra -m16 -Wno-main \
	-ffreestanding -fno-exceptions -fno-rtti -O2 -Os -march=i386 \
	-momit-leaf-frame-pointer -fno-threadsafe-statics

HEADERS := ../../include/PS2Controller.h ../../include/io.h \
	../../include/FlagField.h ../../include/pm86.h

libpm86.a: pmcall.o a20.o
	$(AR) rcs $@ $^
	ranlib $@

pmcall.o: pmcall.S
	$(AS) $(ASFLAGS) $^ -o $@

a20.o: a20.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) *.o *.a
