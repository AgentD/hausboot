LDFLAGS := -m16 -O2 -Osize -ffreestanding -nostdlib

CXXFLAGS := -I../include -std=c++17 -Wall -Wextra -m16 -Wno-main \
	-ffreestanding -fno-exceptions -fno-rtti -O2 -Os

HEADERS := ../include/video.h

stage2.bin: stage2.ld stage2.o
	$(CXX) $(LDFLAGS) -T $< -o $@ $(filter-out $<,$^)

stage2.o: stage2.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) *.o *.bin
