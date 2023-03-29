/* SPDX-License-Identifier: ISC */
/*
 * pm.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
__asm__ (".code32");

#include <cstdint>
#include <cstddef>

static const char *msg = "Protected Mode Test";

void PmTest(void)
{
	auto *vidmem = (uint16_t *)0x0B8000;

	auto *ptr = vidmem + 24 * 80;

	while (*msg != '\0')
		*(ptr++) |= *(msg++);
}
