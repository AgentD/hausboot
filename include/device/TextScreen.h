/* SPDX-License-Identifier: ISC */
/*
 * TextScreen.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef TEXTSCREEN_H
#define TEXTSCREEN_H

#include <cstdint>

#include "device/io.h"

class TextScreen {
public:
	enum class Color : uint8_t {
		Black = 0,
		Blue = 1,
		Green = 2,
		Cyan = 3,
		Red = 4,
		Magenta = 5,
		Brown = 6,
		LightGray = 7,
		DarkGray = 8,
		LightBlue = 9,
		LightGreen = 10,
		LightCyan = 11,
		LightRed = 12,
		LightMagenta = 13,
		Yellow = 14,
		White = 15
	};

	void Clear() {
		for (size_t i = 0; i < (80 * 25); ++i) {
			_vidmem[i].SetColor(_fg, _bg);
			_vidmem[i].SetChar(' ');
		}

		_x = 0;
		_y = 0;
	}

	void SetColor(Color foreground, Color background) {
		_fg = foreground;
		_bg = background;
	}

	void PutChar(uint8_t c) {
		if (c >= 0x7F)
			return;

		if (c >= 0x20) {
			_vidmem[_y * _width + _x].SetChar(c);
			_x += 1;
		} else if (c == '\t') {
			_x = (_x + 8) & ~0x07;
		}

		if (_x >= _width || c == '\n') {
			_x = 0;
			_y += 1;

			if (_y >= _height) {
				ScrollUp();
				return;
			}
		}

		SyncCursor();
	}

	void WriteString(const char *str) {
		while (*str != '\0')
			PutChar(*(str++));
	}

	void ScrollUp() {
		auto *dst = _vidmem;
		auto *src = _vidmem + _width;
		auto *end = _vidmem + _width * _height;

		while (src != end)
			*(dst++) = *(src++);

		while (dst != end) {
			dst->SetChar(' ');
			dst->SetColor(_fg, _bg);
		}

		if (_y > 0)
			_y -= 1;

		SyncCursor();
	}

	void WriteHex(uint64_t x) {
		char buffer[sizeof(x) * 2 + 1];
		char *ptr = buffer + sizeof(buffer) - 1;

		*(ptr--) = '\0';

		for (size_t i = 0; i < (sizeof(x) * 2); ++i) {
			auto digit = x & 0x0F;
			*(ptr--) = digit < 10 ? (digit + '0') :
				(digit - 10 + 'A');
			x >>= 4;
		}

		(*this) << (ptr + 1);
	}

	TextScreen &operator<< (char c) {
		PutChar(c);
		return *this;
	}

	TextScreen &operator<< (const char *str) {
		WriteString(str);
		return *this;
	}
private:
	struct VidMemCell {
	public:
		void SetColor(Color fg, Color bg) {
			uint16_t attrib = (uint8_t)bg << 4 | (uint8_t)fg;

			_raw = (_raw & 0x00FF) | (attrib << 8);
		}

		void SetChar(uint8_t c) {
			_raw = (_raw & 0xFF00) | c;
		}
	private:
		uint16_t _raw;
	};

	void SyncCursor() {
		uint16_t pos = _y * _width + _x;

		IoWriteByte(0x3D4, 14);
		IoWriteByte(0x3D5, (pos >> 8) & 0xFF);
		IoWriteByte(0x3D4, 15);
		IoWriteByte(0x3D5, pos & 0xFF);
	}

	VidMemCell *_vidmem = (VidMemCell *)0x0B8000;
	uint8_t _width = 80;
	uint8_t _height = 25;
	uint8_t _x = 0;
	uint8_t _y = 0;
	Color _fg = Color::LightGray;
	Color _bg = Color::Black;
};

#endif /* TEXTSCREEN_H */
