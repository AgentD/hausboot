CXXFLAGS := -I../include

HEADERS := ../include/fs/FatSuper.h ../include/types/FixedLengthString.h \
	../include/types/UnalignedInt.h ../include/types/ByteBlob.h \
	../include/Stage2Header.h ../include/part/MBREntry.h \
	../include/part/CHSPacked.h \
	../include/fs/FatFsInfo.h ../include/fs/FatDirent.h \
	../include/fs/FatDirentLong.h ../include/BIOS/BiosDisk.h util.h \
	../include/host/File.h ../include/fs/FatName.h

.PHONY: all
all: installfat fatedit

installfat: installfat.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@

fatedit: fatedit.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	$(RM) installfat fatedit
