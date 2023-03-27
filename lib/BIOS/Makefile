ASFLAGS := --32

libBIOS.a: e820.o
	$(AR) rcs $@ $^
	ranlib $@

e820.o: e820.S
	$(AS) $(ASFLAGS) $^ -o $@

.PHONY: clean
clean:
	$(RM) *.o *.a
