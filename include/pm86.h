/* SPDX-License-Identifier: ISC */
/*
 * pm86.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef PM86_H
#define PM86_H

#include <cstddef>
#include <utility>

// Try to enable the A20 line by various means and test if it worked
extern bool EnableA20();

extern "C" {
	/*
	  Switch to protected mode, call a 32 bit function with possible
	  arguments and switch back into 16 bit real-mode before returning.
	*/
	void ProtectedModeCall(...);
}

void CopyMemory32(void *dst, const void *src, size_t count);

void ClearMemory32(void *dst, size_t size);

#endif /* PM86_H */
