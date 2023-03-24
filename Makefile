disk.img: fatpart.img mbr/mbr.bin
	dd if=/dev/zero of=$@ bs=1M count=50
	parted --script $@ \
		"mklabel msdos" \
		"mkpart primary fat32 1M 40M" \
		"set 1 boot on"
	dd if=./mbr/mbr.bin of=$@ conv=notrunc bs=1 count=446
	dd if=./fatpart.img of=$@ conv=notrunc bs=512 seek=2048

fatpart.img: tools/installfat tools/fatedit \
	vbr/vbr.bin stage2/stage2.bin kernel/KRNL386.SYS
	dd if=/dev/zero of=$@ bs=1M count=40
	mkfs.fat -F 32 $@
	./tools/installfat -v "vbr/vbr.bin" -o $@ \
			   --stage2 "stage2/stage2.bin"
	echo "mkdir BOOT" | ./tools/fatedit $@
	echo "pack kernel/KRNL386.SYS BOOT/KRNL386.SYS" | ./tools/fatedit $@

kernel/KRNL386.SYS:
	$(MAKE) -C kernel

tools/fatedit:
	$(MAKE) -C tools

tools/installfat:
	$(MAKE) -C tools

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
	$(MAKE) -C tools clean
	$(MAKE) -C kernel clean
	$(RM) *.img

.PHONY: runqemu
runqemu: disk.img
	qemu-system-i386 -drive format=raw,file=./disk.img

.PHONY: runbochs
runbochs: disk.img
	bochs -q -f ./bochsrc.txt
