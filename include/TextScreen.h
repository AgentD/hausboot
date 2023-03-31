/* SPDX-License-Identifier: ISC */
/*
 * TextScreen.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef TEXT_SCREEN_H
#define TEXT_SCREEN_H

#include <cstdint>
#include <cstddef>

template<class DRIVER>
class TextScreen {
public:
	void Reset() { _driver.Reset(); }
	void PutChar(uint8_t c) { _driver.PutChar(c); }
	auto &Driver() { return _driver; }

	void WriteString(const char *str) {
		while (*str != '\0')
			_driver.PutChar(*(str++));
	}

	void WriteString(const char *str, size_t count) {
		while (count--) {
			_driver.PutChar(*(str++));
		}
	}

	void WriteHex(uint64_t x) {
		char buffer[sizeof(x) * 2 + 1];
		char *ptr = buffer + sizeof(buffer) - 1;

		*(ptr--) = '\0';

		for (size_t i = 0; i < (sizeof(buffer) - 1); ++i) {
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

	auto &operator<< (const char *str) {
		while (*str != '\0')
			PutChar(*(str++));

		return *this;
	}

	auto &operator<< (uint32_t x) {
		WriteDecimal(x);
		return *this;
	}

	auto &operator<< (int32_t x) {
		if (x < 0) {
			PutChar('-');
			x = -x;
		}

		WriteDecimal(x);
		return *this;
	}

	auto &operator<< (void *ptr) {
		WriteHex((uint32_t)ptr);
		return *this;
	}
private:
	DRIVER _driver;
};

#endif /* TEXT_SCREEN_H */
