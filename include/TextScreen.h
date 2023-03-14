/* SPDX-License-Identifier: ISC */
/*
 * TextScreen.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef TEXT_SCREEN_H
#define TEXT_SCREEN_H

#include "video.h"

class TextScreen {
public:
	void Reset() {
		for (unsigned int i = 0; i < _width * _height; ++i) {
			_vidmem[i].color.Set(Color::LightGray);
			_vidmem[i].character = 0;
		}

		_cursorX = 0;
		_cursorY = 0;
		SyncCursor();
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

	void PutChar(uint8_t c) {
		if (c >= 0x20 && c <= 0x7E) {
			_vidmem[_cursorY * _width + _cursorX].character = c;
			_cursorX += 1;
		}

		if (c == '\t')
			_cursorX += 8;

		if (_cursorX >= _width || c == '\n') {
			_cursorX = 0;
			_cursorY += 1;

			if (_cursorY >= _height) {
				ScrollUp();
				return;
			}
		}

		SyncCursor();
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
private:
	void SyncCursor() {
		auto pos = _cursorY * _width + _cursorX;

		OutByte(0x3D4, 14);
		OutByte(0x3D5, (pos >> 8) & 0xFF);
		OutByte(0x3D4, 15);
		OutByte(0x3D5, pos & 0xFF);
	}

	void OutByte(unsigned int port, uint8_t byte) {
		__asm__ volatile("outb %0, %1"  : : "a"(byte), "Nd"(port));
	}

	void ScrollUp() {
		auto *dst = _vidmem;
		auto *src = _vidmem + _width;

		for (unsigned int i = 0; i < (_height - 1) * _width; ++i)
			*(dst++) = *(src++);

		for (unsigned int i = 0; i < _width; ++i) {
			dst[i].color.Set(Color::LightGray);
			dst[i].character = 0;
		}

		if (_cursorY > 0) {
			_cursorY -= 1;
			SyncCursor();
		}
	}

	VidmemEntry *_vidmem = (VidmemEntry *)0xB8000;
	const unsigned int _width = 80;
	const unsigned int _height = 25;
	unsigned int _cursorX = 0;
	unsigned int _cursorY = 0;
};

#endif /* TEXT_SCREEN_H */
