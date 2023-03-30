/* SPDX-License-Identifier: ISC */
/*
 * Memory.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef MEMORY_H
#define MEMORY_H

#include <cstddef>

extern "C" {
	void HeapInit(void *basePtr, size_t maxSize);
	void *malloc(size_t count);
	void free(void *ptr);
}

inline void *operator new(size_t size) { return malloc(size); }
inline void *operator new[](size_t size) { return malloc(size); }
inline void operator delete(void *p) { free(p); }
inline void operator delete(void *p, size_t) { free(p); }
inline void operator delete[](void *p) { free(p); }
inline void operator delete[](void *p, size_t) { free(p); }

#endif /* MEMORY_H */

