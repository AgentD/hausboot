/* SPDX-License-Identifier: ISC */
/*
 * FatDirent.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef FAT_DIRENT_H
#define FAT_DIRENT_H

#include "FixedLengthString.h"
#include "UnalignedInt.h"
#include "FlagField.h"
#include "ByteBlob.h"

class FatDirent {
public:
	enum class Flags : uint8_t {
		ReadOnly = 0x01,
		Hidden = 0x02,
		System = 0x04,
		VolumeID = 0x08,
		Directory = 0x10,
		Archive = 0x20,
		LongFileName = 0x0F,
	};

	uint32_t ClusterIndex() const {
		uint32_t out = _clusterIndexHigh.Read();
		out <<= 16;
		out |= _clusterIndexLow.Read();
		return out;
	}

	void SetClusterIndex(uint32_t index) {
		_clusterIndexLow = index & 0x0FFFF;
		_clusterIndexHigh = (index >> 16) & 0x0FFFF;
	}

	uint32_t Size() const {
		return _size.Read();
	}

	void SetSize(uint32_t size) {
		_size = size;
	}

	auto EntryFlags() const {
		return _flags;
	}

	uint8_t Checksum() const {
		uint8_t sum = 0;

		for (int i = 0; i < 8; ++i)
			sum = ((sum & 1) << 7) + (sum >> 1) + _name.At(i);

		for (int i = 0; i < 3; ++i)
			sum = ((sum & 1) << 7) + (sum >> 1) + _extension.At(i);

		return sum;
	}

	bool IsDummiedOut() const {
		return _name.At(0) == 0xE5;
	}

	bool IsLastInList() const {
		return _name.At(0) == 0x00;
	}

	bool NameToString(char buffer[13]) const {
		if (_name.At(0) == ' ')
			return false;

		size_t idx = 0;

		for (size_t i = 0; i < 8; ++i)
			buffer[idx++] = _name.At(i);

		while (idx > 0 && buffer[idx - 1] == ' ')
			--idx;

		if (_extension.At(0) != ' ') {
			buffer[idx++] = '.';

			for (size_t i = 0; i < 3; ++i)
				buffer[idx++] = _extension.At(i);

			while (idx > 0 && buffer[idx - 1] == ' ')
				--idx;
		}

		buffer[idx] = '\0';
		return true;
	}
private:
	FixedLengthString<8, ' '> _name;
	FixedLengthString<3, ' '> _extension;
	FlagField<Flags, uint8_t> _flags;
	ByteBlob<2> _unused;
	UnalignedInt<uint16_t> _ctimeHMS;
	UnalignedInt<uint16_t> _ctimeYMD;
	UnalignedInt<uint16_t> _atimeYMD;
	UnalignedInt<uint16_t> _clusterIndexHigh;
	UnalignedInt<uint16_t> _mtimeHMS;
	UnalignedInt<uint16_t> _mtimeYMD;
	UnalignedInt<uint16_t> _clusterIndexLow;
	UnalignedInt<uint32_t> _size;
};

static_assert(sizeof(FatDirent) == 32);

#endif /* FAT_DIRENT_H */
