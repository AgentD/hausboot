/* SPDX-License-Identifier: ISC */
/*
 * Heap.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef STAGE2_HEAP_H
#define STAGE2_HEAP_H

#include <cstdint>

class Heap {
public:
	Heap() = delete;
	Heap(void *ptr) : _ptr((uint8_t *)ptr) {
	}

	void *AllocateRaw(size_t count) {
		auto *ret = _ptr;
		_ptr += count;
		return ret;
	}
private:
	uint8_t *_ptr;
};

#endif /* STAGE2_HEAP_H */
