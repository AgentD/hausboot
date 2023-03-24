/* SPDX-License-Identifier: ISC */
/*
 * kernel.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "multiboot.h"

static const char *msg = "Hello 32 bit world!";

void multiboot_main(const MultiBootInfo *info)
{
	(void)info;

	auto *vidmem = (uint16_t *)0x0B8000;

	for (size_t i = 0; i < (80 * 25); ++i)
		vidmem[i] = 0x0700;

	while (*msg != '\0')
		*(vidmem++) |= *(msg++);

	for (;;)
		__asm__ ("hlt");
}
