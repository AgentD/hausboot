CXXFLAGS := -I../include

HEADERS := ../include/FatSuper.h ../include/FixedLengthString.h \
	../include/UnalignedInt.h ../include/ByteBlob.h \
	../include/stage2.h ../include/MBREntry.h ../include/CHSPacked.h \
	../include/FatFsInfo.h ../include/FatDirent.h \
	../include/FatDirentLong.h ../include/BiosDisk.h util.h

.PHONY: all
all: installfat fatedit

installfat: installfat.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@

fatedit: fatedit.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	$(RM) installfat fatedit
