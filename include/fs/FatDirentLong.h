/* SPDX-License-Identifier: ISC */
/*
 * FatDirentLong.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef FAT_DIRENT_LONG_H
#define FAT_DIRENT_LONG_H

#include "FatDirent.h"

class FatDirentLong {
public:
	bool IsLast() const {
		return _seqNumber & 0x40;
	}

	uint8_t SequenceNumber() const {
		return _seqNumber & 0x3F;
	}

	auto EntryFlags() const {
		return _attrib;
	}

	auto ShortNameChecksum() const {
		return _shortChecksum;
	}

	void NameToString(char buffer[14]) const {
		char *ptr = Stringify(_namePart, 5, buffer);
		ptr = Stringify(_namePart2, 6, ptr);
		ptr = Stringify(_namePart3, 2, ptr);
		*ptr = '\0';
	}
private:
	char *Stringify(const UnalignedInt<uint16_t> *in, size_t count, char *out) const {
		for (size_t i = 0; i < count; ++i) {
			auto x = in[i].Read();

			if ((x > 0 && x < 0x20) || x >= 0x7F) {
				*(out++) = '?';
			} else {
				*(out++) = x;
			}
		}
		return out;
	}

	uint8_t _seqNumber;
	UnalignedInt<uint16_t> _namePart[5];
	FlagField<FatDirent::Flags, uint8_t> _attrib;
	uint8_t _entryType;
	uint8_t _shortChecksum;
	UnalignedInt<uint16_t> _namePart2[6];
	UnalignedInt<uint16_t> _unused;
	UnalignedInt<uint16_t> _namePart3[2];
};

static_assert(sizeof(FatDirentLong) == 32);
static_assert(sizeof(FatDirentLong) == sizeof(FatDirent));

#endif /* FAT_DIRENT_LONG_H */
