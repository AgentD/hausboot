/* SPDX-License-Identifier: ISC */
/*
 * MultiBootMmap.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef MULTIBOOT_MMAP_H
#define MULTIBOOT_MMAP_H

#include <cstdint>

#include "FlagField.h"
#include "UnalignedInt.h"
#include "BIOS/MemoryMap.h"

class MultiBootMmap {
public:
	auto Type() const {
		return _wrapped.Type();
	}

	auto BaseAddress() const {
		return _wrapped.BaseAddress();
	}

	auto Size() const {
		return _wrapped.Size();
	}

	const auto *TypeAsString() const {
		return _wrapped.TypeAsString();
	}

	const auto *Next() const {
		auto next = (uint32_t)this + _size.Read() + sizeof(_size);

		return (const MultiBootMmap *)next;
	}

	MultiBootMmap &operator= (const MemoryMapEntry &e820) {
		_wrapped = e820;
		return *this;
	}

	void SetNext(MultiBootMmap *next) {
		_size = (char *)next - (char *)this - sizeof(_size);
	}
private:
	UnalignedInt<uint32_t> _size;
	MemoryMapEntry _wrapped;
};

static_assert(sizeof(void *) == sizeof(uint32_t));
static_assert(sizeof(MultiBootMmap) == 24);

#endif /* MULTIBOOT_MMAP_H */
