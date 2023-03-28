/* SPDX-License-Identifier: ISC */
/*
 * FatFs.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef FAT_FS_H
#define FAT_FS_H

#include "IBlockDevice.h"
#include "fs/FatSuper.h"
#include "fs/FatName.h"
#include "StringUtil.h"
#include "FlagField.h"
#include "Memory.h"

struct FatFile {
	uint32_t cluster;
	uint32_t size;
	FlagField<FatDirent::Flags, uint8_t> flags;
};

class FatFs {
public:
	bool Init(IBlockDevice *blk, const FatSuper *fsSuper) {
		_blk = blk;
		super = fsSuper;

		currentFatSector = 0xFFFFFFFF;
		currentDataCluster = 0xFFFFFFFF;

		fatWindow = (uint8_t *)malloc(blk->SectorSize());
		dataWindow = (uint8_t *)malloc(BytesPerCluster());
		return true;
	}

	void Cleanup() {
		free(fatWindow);
		free(dataWindow);
		fatWindow = nullptr;
		dataWindow = nullptr;
	}

	size_t BytesPerCluster() const {
		return super->SectorsPerCluster() * _blk->SectorSize();
	}

	auto RootDir() const {
		FatFile out;
		out.cluster = super->RootDirIndex();
		out.size = 0;
		out.flags.Clear();
		out.flags.Set(FatDirent::Flags::Directory);
		return out;
	}

	enum class ScanVerdict {
		Ok = 0,
		Error = 1,
		Stop = 2,
	};

	template<typename FUNCTOR>
	bool ForEachClusterInChain(uint32_t index, uint32_t size, FUNCTOR cb) {
		while (size > 0 && index < 0x0FFFFFF0) {
			uint32_t next;

			if (!LoadDataCluster(index))
				return false;

			if (!ReadFatIndex(index, next))
				return false;

			auto diff = BytesPerCluster();
			if (diff > size)
				diff = size;

			ScanVerdict verdict = cb((void *)dataWindow, diff);
			if (verdict == ScanVerdict::Error)
				return false;
			if (verdict == ScanVerdict::Stop)
				break;

			size -= diff;
			index = next;
		}

		return true;
	}

	enum class FindResult {
		Ok = 0,
		NameInvalid = -1,
		IOError = -2,
		NoEntry = -3,
		NotDir = -4,
	};

	FindResult FindInDirectory(const FatFile &in, const char *name, FatFile &out) {
		if (!IsShortName(name))
			return FindResult::NameInvalid;

		if (!in.flags.IsSet(FatDirent::Flags::Directory))
			return FindResult::NotDir;

		auto index = in.cluster;

		while (index < 0x0FFFFFF0) {
			uint32_t next;

			if (!LoadDataCluster(index) || !ReadFatIndex(index, next))
				return FindResult::IOError;

			index = next;

			auto *entS = (FatDirent *)dataWindow;
			auto max = BytesPerCluster() / sizeof(*entS);

			for (decltype(max) i = 0; i < max; ++i) {
				if (entS[i].IsLastInList())
					return FindResult::NoEntry;
				if (entS[i].EntryFlags().IsSet(FatDirent::Flags::LongFileName))
					continue;
				if (entS[i].IsDummiedOut())
					continue;

				char buffer[8 + 1 + 3 + 1];
				entS[i].NameToString(buffer);

				if (StrEqual(buffer, name)) {
					out.cluster = entS[i].ClusterIndex();
					out.size = entS[i].Size();
					out.flags = entS[i].EntryFlags();
					return FindResult::Ok;
				}
			}
		}

		return FindResult::NoEntry;
	}

	FindResult FindByPath(const char *name, FatFile &out) {
		out = RootDir();

		while (*name != '\0') {
			if (*name == '/' || *name == '\\') {
				while (*name == '/' || *name == '\\')
					++name;
				continue;
			}

			char name8_3[8 + 1 + 3 + 1];
			int count = 0;

			while (*name != '\0' && *name != '/' && *name != '\\') {
				if (count >= (8 + 1 + 3))
					return FindResult::NameInvalid;
				name8_3[count++] = *(name++);
			}

			name8_3[count] = '\0';

			auto ret = FindInDirectory(out, name8_3, out);
			if (ret != FindResult::Ok)
				return ret;
		}

		return FindResult::Ok;
	}
private:
	bool LoadDataCluster(uint32_t index) {
		if (index < 2)
			return false;

		if (index == currentDataCluster)
			return true;

		auto lba = super->ClusterIndex2Sector(index);

		for (uint32_t i = 0; i < super->SectorsPerCluster(); ++i) {
			auto *ptr = dataWindow + i * _blk->SectorSize();

			if (!_blk->LoadSector(lba + i, ptr))
				return false;
		}

		currentDataCluster = index;
		return true;
	}

	bool LoadFatSector(uint32_t index) {
		if (index >= super->SectorsPerFat())
			return false;

		if (index != currentFatSector) {
			auto lba = super->ReservedSectors() + index;

			if (!_blk->LoadSector(lba, fatWindow))
				return false;

			currentFatSector = index;
		}
		return true;
	}

	bool ReadFatIndex(uint32_t index, uint32_t &out) {
		auto sector = (index * 4) / _blk->SectorSize();
		auto offset = (index * 4) % _blk->SectorSize();

		if (!LoadFatSector(sector))
			return false;

		out = *((uint32_t *)(fatWindow + offset));
		return true;
	}

	IBlockDevice *_blk;
	uint8_t *fatWindow;
	uint8_t *dataWindow;
	const FatSuper *super;
	uint32_t currentFatSector;
	uint32_t currentDataCluster;
};

#endif /* FAT_FS_H */
