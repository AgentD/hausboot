/* SPDX-License-Identifier: ISC */
/*
 * kernel.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "kernel/MultiBootInfo.h"

static void WriteToScreen(unsigned int x, unsigned int y, const char *msg)
{
	auto *dst = (uint16_t *)0x0B8000 + y * 80 + x;

	while (*msg != '\0')
		*(dst++) |= *(msg++);
}

static void ClearScreen()
{
	auto *vidmem = (uint16_t *)0x0B8000;

	for (size_t i = 0; i < (80 * 25); ++i)
		vidmem[i] = 0x0700;
}

extern "C" {
	void multiboot_main(const MultiBootInfo *info, uint32_t signature);
};

static const char *msg = "Hello 32 bit world!";

static const char *msgSigMatch = "Multi boot signature matches!";
static const char *msgSigNoMatch = "Multi boot signature is broken!";

void multiboot_main(const MultiBootInfo *info, uint32_t signature)
{
	(void)info;

	ClearScreen();
	WriteToScreen(0, 0, msg);
	WriteToScreen(0, 1, (signature == 0x2BADB002) ?
		      msgSigMatch : msgSigNoMatch);

	for (;;)
		__asm__ ("hlt");
}
