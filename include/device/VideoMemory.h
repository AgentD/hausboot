/* SPDX-License-Identifier: ISC */
/*
 * VideoMemory.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef VIDEO_MEMORY_H
#define VIDEO_MEMORY_H

#include <cstdint>

#include "device/io.h"

class VideoMemory {
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

	void Reset() {
		for (size_t i = 0; i < (80 * 25); ++i) {
			_vidmem[i].SetColor(_fg, _bg);
			_vidmem[i].SetChar(' ');
		}

		_x = 0;
		_y = 0;
	}

	void PutChar(uint8_t c) {
		if (c == '\r') {
			_x = 0;
			SyncCursor();
			return;
		}

		if (c == '\n') {
			_y += 1;
			if (_y >= _height) {
				ScrollUp();
			} else {
				SyncCursor();
			}
			return;
		}

		if (c == '\t') {
			auto diff = 8 - (_x % 8);
			while (diff--)
				PutChar(' ');
			return;
		}

		if (c >= 0x20 && c <= 0x7E) {
			_vidmem[_y * _width + _x].SetChar(c);
			_x += 1;

			if (_x >= _width) {
				_x = 0;
				_y += 1;

				if (_y >= _height) {
					ScrollUp();
					return;
				}
			}

			SyncCursor();
		}
	}

	void SetColor(Color foreground, Color background) {
		_fg = foreground;
		_bg = background;
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

#endif /* VIDEO_MEMORY_H */
