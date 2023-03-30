/* SPDX-License-Identifier: ISC */
/*
 * copy.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "pm86.h"

__asm__ (".code32");

void CopyMemory32(void *dst, const void *src, size_t count)
{
	if (dst == src || count == 0)
		return;

	if (dst < src) {
		auto *d = (char *)dst;
		auto *s = (char *)src;

		while (count--)
			*(d++) = *(s++);
	} else {
		auto *d = (char *)dst + count - 1;
		auto *s = (char *)src + count - 1;

		while (count--)
			*(d--) = *(s--);
	}
}

void ClearMemory32(void *dst, size_t size)
{
	auto *ptr = (char *)dst;

	while (size--)
		*(ptr++) = 0x00;
}
