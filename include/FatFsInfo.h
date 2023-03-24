/* SPDX-License-Identifier: ISC */
/*
 * FatFsInfo.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef FAT_FS_INFO_H
#define FAT_FS_INFO_H

#include "UnalignedInt.h"
#include "ByteBlob.h"

constexpr uint32_t FatFsInfoMagic1 = 0x41615252;
constexpr uint32_t FatFsInfoMagic2 = 0x61417272;
constexpr uint32_t FatFsInfoMagic3 = 0xAA550000;

class FatFsInfo {
public:
	bool IsValid() const {
		return _magic1.Read() == FatFsInfoMagic1 &&
			_magic2.Read() == FatFsInfoMagic2 &&
			_magic3.Read() == FatFsInfoMagic3;
	}

	void SetNextFreeCluster(uint32_t index) {
		_nextFreeCluster = index;
	}

	void SetNumFreeCluster(uint32_t count) {
		_freeClusters = count;
	}
private:
	UnalignedInt<uint32_t> _magic1 = FatFsInfoMagic1;
	ByteBlob<480> _reserved0;
	UnalignedInt<uint32_t> _magic2 = FatFsInfoMagic2;
	UnalignedInt<uint32_t> _freeClusters;
	UnalignedInt<uint32_t> _nextFreeCluster;
	ByteBlob<12> _reserved1;
	UnalignedInt<uint32_t> _magic3 = FatFsInfoMagic3;
};

static_assert(sizeof(FatFsInfo) == 512);

#endif /* FAT_FS_INFO_H */
