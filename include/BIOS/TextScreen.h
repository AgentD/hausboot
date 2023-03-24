/* SPDX-License-Identifier: ISC */
/*
 * TextScreen.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef TEXT_SCREEN_H
#define TEXT_SCREEN_H

#include <cstdint>

class TextScreen {
public:
	void Reset() {
		__asm__ __volatile__("int $0x10" : : "a"(0x0003));
	}

	void PutChar(uint8_t c) {
		__asm__ __volatile__("int $0x10" : : "a"(0x0e00 | c), "b"(0));
	}

	void WriteHex(uint32_t x) {
		char buffer[9];
		char *ptr = buffer + sizeof(buffer) - 1;

		*(ptr--) = '\0';

		for (int i = 0; i < 8; ++i) {
			auto digit = x & 0x0F;
			*(ptr--) = digit < 10 ? (digit + '0') :
				(digit - 10 + 'A');
			x >>= 4;
		}

		(*this) << (ptr + 1);
	}

	void WriteDecimal(uint32_t x) {
		if (x > 0) {
			char buffer[10];
			char *ptr = buffer + sizeof(buffer) - 1;

			*(ptr--) = '\0';

			while (x > 0) {
				*(ptr--) = (x % 10) + '0';
				x /= 10;
			}

			(*this) << (ptr + 1);
		} else {
			PutChar('0');
		}
	}

	TextScreen &operator<< (const char *str) {
		while (*str != '\0')
			PutChar(*(str++));

		return *this;
	}

	TextScreen &operator<< (uint32_t x) {
		WriteDecimal(x);
		return *this;
	}

	TextScreen &operator<< (int32_t x) {
		if (x < 0) {
			PutChar('-');
			x = -x;
		}

		WriteDecimal(x);
		return *this;
	}

	TextScreen &operator<< (void *ptr) {
		WriteHex((uint32_t)ptr);
		return *this;
	}
};

[[noreturn, maybe_unused]] static void DumpMessageAndHang(const char *msg)
{
	TextScreen screen;

	screen.Reset();
	screen << msg;

	for (;;) {
		__asm__ __volatile__ ("hlt");
	}
}

#endif /* TEXT_SCREEN_H */
