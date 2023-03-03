CXXFLAGS := -I../include

HEADERS := ../include/FatSuper.h ../include/FixedLengthString.h \
	../include/UnalignedInt.h ../include/ByteBlob.h

installfat: installfat.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	$(RM) installfat
