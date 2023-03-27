/* SPDX-License-Identifier: ISC */
/*
 * MemoryMap.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef BIOS_MEMORY_MAP_H
#define BIOS_MEMORY_MAP_H

#include "UnalignedInt.h"

#include <cstdint>

extern "C" {
	int IntCallE820(uint32_t *ebxInOut, uint8_t dst[20]);
};

class MemoryMapEntry {
public:
	enum class MemType : uint32_t {
		Usable = 1,
		Reserved = 2,
		ACPI = 3,
		Preserve = 4,
		Broken = 5,
	};

	int Load(uint32_t &ebxInOut) {
		static MemoryMapEntry temp;

		/*
		  XXX: Linux memory.c says that some BIOSes assume we always
		  use the same buffer, so we use a static scratch buffer for
		  the call and copy the result afterwards.
		*/
		auto ret = IntCallE820(&ebxInOut, (uint8_t *)&temp);
		*this = temp;

		return ret;
	}

	auto BaseAddress() const {
		return _base.Read();
	}

	auto Size() const {
		return _size.Read();
	}

	MemType Type() const {
		auto value = _type.Read();

		return value < 1 || value > 5 ?
			MemType::Broken : (MemType)value;
	}
private:
	UnalignedInt<uint64_t> _base;
	UnalignedInt<uint64_t> _size;
	UnalignedInt<uint32_t> _type;
};

static_assert(sizeof(MemoryMapEntry) == 20);

template<size_t MAX_COUNT>
class MemoryMap {
public:
	bool Load() {
		uint32_t ebx = 0;

		_count = 0;

		do {
			int ret = _ent[_count].Load(ebx);
			if (ret < 0) {
				_count = 0;
				return false;
			}
			if (ret > 0)
				break;
			++_count;
		} while (_count < MAX_COUNT && ebx != 0);

		return true;
	}

	const auto *begin() const {
		return _ent;
	}

	const auto *end() const {
		return _ent + _count;
	}
private:
	MemoryMapEntry _ent[MAX_COUNT];
	size_t _count = 0;
};

#endif /* BIOS_MEMORY_MAP_H */
