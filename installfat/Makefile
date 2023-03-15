CXXFLAGS := -I../include

HEADERS := ../include/FatSuper.h ../include/FixedLengthString.h \
	../include/UnalignedInt.h ../include/ByteBlob.h \
	../include/stage2.h ../include/MBREntry.h ../include/CHSPacked.h

installfat: installfat.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	$(RM) installfat
