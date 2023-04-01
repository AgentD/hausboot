/* SPDX-License-Identifier: ISC */
/*
 * FatSuper.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef FAT_SUPER_H
#define FAT_SUPER_H

#include "types/FixedLengthString.h"
#include "types/UnalignedInt.h"
#include "types/ByteBlob.h"

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstring>

class FatSuper {
public:
	FatSuper() = default;

	void SetTotalSectorCount(uint32_t count) {
		_totalSectorCount = count;
	}

	void SetSectorsPerFat(uint32_t count) {
		_sectorsPerFat = count;
	}

	void SetBootCode(const uint8_t *data, size_t size,
			 size_t entryPoint = 0) {
		size = std::min(size, sizeof(_bootCode));

		memcpy(&_bootCode, data, size);

		_introJump.SetEntryPoint(entryPoint);
	}

	void SetOEMName(const char *name) {
		_oemName.Set(name);
	}

	uint16_t BytesPerSector() const {
		return _bytesPerSector.Read();
	}

	uint16_t ReservedSectors() const {
		return _numReservedSectors.Read();
	}

	uint16_t FsInfoIndex() const {
		return _fsInfoIndex.Read();
	}

	void SetFsInfoIndex(uint16_t value) {
		_fsInfoIndex.Set(value);
	}

	uint16_t BackupIndex() const {
		return _bootSecCopyIndex.Read();
	}

	void SetBackupIndex(uint16_t value) {
		_bootSecCopyIndex.Set(value);
	}

	unsigned int SectorsPerCluster() const {
		return _sectorsPerCluster;
	}

	unsigned int NumFats() const {
		return _numFats;
	}

	unsigned int SectorCount() const {
		return _totalSectorCount.Read();
	}

	unsigned int SectorsPerFat() const {
		return _sectorsPerFat.Read();
	}

	unsigned int RootDirIndex() const {
		return _rootDirIndex.Read();
	}

	uint32_t ClusterIndex2Sector(uint32_t N) const {
		auto first = ReservedSectors() + NumFats() * SectorsPerFat();

		if (N <= 2)
			return first;

		return ((N - 2) * SectorsPerCluster()) + first;
	}
private:
	struct {
	public:
		void SetEntryPoint(size_t entry) {
			entry = std::min(entry, sizeof(_bootCode));

			_jump = 0xEB;
			_codeOffset = offsetof(FatSuper, _bootCode) - 2 + entry;
			_nop = 0x90;
		}
	private:
		uint8_t _jump;
		uint8_t _codeOffset;
		uint8_t _nop;
	} _introJump;

	FixedLengthString<8, ' '> _oemName;

	/* DOS 2.0 BPB */
	UnalignedInt<uint16_t> _bytesPerSector;
	uint8_t _sectorsPerCluster;
	UnalignedInt<uint16_t> _numReservedSectors;
	uint8_t _numFats;
	ByteBlob<4> _unused0;
	uint8_t _mediaDescriptor;
	ByteBlob<2> _unused1;

	/* DOS 3.31 BPB */
	UnalignedInt<uint16_t> _sectorsPerTrack;
	UnalignedInt<uint16_t> _headsPerDisk;
	ByteBlob<4> _unused2;
	UnalignedInt<uint32_t> _totalSectorCount;

	/* FAT32 BPB */
	UnalignedInt<uint32_t> _sectorsPerFat;
	UnalignedInt<uint16_t> _mirrorFlags;
	UnalignedInt<uint16_t> _version;
	UnalignedInt<uint32_t> _rootDirIndex;
	UnalignedInt<uint16_t> _fsInfoIndex;
	UnalignedInt<uint16_t> _bootSecCopyIndex;
	ByteBlob<12> _unused3;
	uint8_t _physDriveNum;
	ByteBlob<1> _wtf1;
	uint8_t _extBootSignature;
	UnalignedInt<uint32_t> _volumeId;
	FixedLengthString<11, ' '> _label;
	FixedLengthString<8, ' '> _fsName;

	/* boot sector code */
	ByteBlob<420> _bootCode;

	/* magic signature */
	UnalignedInt<uint16_t> _bootSignature;
};

static_assert(sizeof(FatSuper) == 512);

#endif /* FAT_SUPER_H */
