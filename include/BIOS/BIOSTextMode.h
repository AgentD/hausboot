/* SPDX-License-Identifier: ISC */
/*
 * BIOSTextMode.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef BIOS_TEXT_MODE_H
#define BIOS_TEXT_MODE_H

#include <cstdint>
#include <cstddef>

class BIOSTextMode {
public:
	void Reset() {
		__asm__ __volatile__("int $0x10" : : "a"(0x0003));
	}

	void PutChar(uint8_t c) {
		__asm__ __volatile__("int $0x10" : : "a"(0x0e00 | c), "b"(0));
	}
};

[[noreturn, maybe_unused]] static void DumpMessageAndHang(const char *msg)
{
	BIOSTextMode screen;

	screen.Reset();
	while (*msg != '\0')
		screen.PutChar(*(msg++));

	for (;;) {
		__asm__ __volatile__ ("hlt");
	}
}

#endif /* BIOS_TEXT_MODE_H */
