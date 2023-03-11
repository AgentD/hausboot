/* SPDX-License-Identifier: ISC */
/*
 * MBREntry.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef MBR_ENTRY_H
#define MBR_ENTRY_H

#include <cstdint>
#include "CHSPacked.h"
#include "UnalignedInt.h"
#include "FlagField.h"

class MBREntry {
public:
	enum class Attribute : uint8_t {
		Bootable = 0x80
	};

	bool IsBootable() const {
		return _attrib.IsSet(Attribute::Bootable);
	}

	const CHSPacked &StartAddressCHS() const {
		return _chsStart;
	}
private:
	FlagField<Attribute, uint8_t> _attrib;
	CHSPacked _chsStart;
	uint8_t _type;
	CHSPacked _chsEnd;
	UnalignedInt<uint32_t> _lbaStart;
	UnalignedInt<uint32_t> _sectorCount;
};

static_assert(sizeof(MBREntry) == 16);

#endif /* MBR_ENTRY_H */
