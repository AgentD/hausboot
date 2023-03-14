LDFLAGS := -m16 -O2 -Osize -ffreestanding -nostdlib -march=i386

CXXFLAGS := -I../include -std=c++17 -Wall -Wextra -m16 -Wno-main \
	-ffreestanding -fno-exceptions -fno-rtti -O2 -Os -march=i386 \
	-momit-leaf-frame-pointer

HEADERS := ../include/TextScreen.h ../include/video.h ../include/stage2.h

stage2.bin: stage2.ld stage2.o
	$(CXX) $(LDFLAGS) -T $< -o $@ $(filter-out $<,$^)

stage2.o: stage2.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) *.o *.bin
