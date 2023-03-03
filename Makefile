CXXFLAGS := -I./include

disk.img: fatpart.img mbr/mbr.bin
	dd if=/dev/zero of=$@ bs=1M count=50
	parted --script $@ \
		"mklabel msdos" \
		"mkpart primary fat32 1M 40M" \
		"set 1 boot on"
	dd if=./mbr/mbr.bin of=$@ conv=notrunc bs=1 count=446
	dd if=./fatpart.img of=$@ conv=notrunc bs=512 seek=2048

fatpart.img: installfat vbr/vbr.bin stage2/stage2.bin
	dd if=/dev/zero of=$@ bs=1M count=40
	mkfs.fat -F 32 $@
	./installfat -v "vbr/vbr.bin" -o $@ --stage2 "stage2/stage2.bin"

installfat: installfat.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

mbr/mbr.bin:
	$(MAKE) -C mbr

vbr/vbr.bin:
	$(MAKE) -C vbr

stage2/stage2.bin:
	$(MAKE) -C stage2

.PHONY: clean
clean:
	$(MAKE) -C mbr clean
	$(MAKE) -C vbr clean
	$(MAKE) -C stage2 clean
	$(RM) *.img *.o installfat
