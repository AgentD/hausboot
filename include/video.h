/* SPDX-License-Identifier: ISC */
/*
 * viedo.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef VIDEO_H
#define VIDEO_H

#include <cstdint>

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
	White = 15,
};

struct VidmemEntry {
	char character;

	struct {
	public:
		Color Foreground() const {
			return static_cast<Color>(_raw & 0x0F);
		}

		Color Background() const {
			return static_cast<Color>((_raw >> 4) & 0x0F);
		}

		void SetForeground(Color c) {
			_raw &= 0xF0;
			_raw |= static_cast<uint8_t>(c);
		}

		void SetBackground(Color c) {
			_raw &= 0x0F;
			_raw |= static_cast<uint8_t>(c) << 4;
		}

		void Set(Color fg, Color bg = Color::Black) {
			_raw = static_cast<uint8_t>(fg) |
				(static_cast<uint8_t>(bg) << 4);
		}
	private:
		uint8_t _raw;
	} color;
};

static_assert(sizeof(VidmemEntry) == 2);

[[noreturn]] static void DumpMessageAndHang(const char *msg)
{
	auto *vidmem = (VidmemEntry *)0xB8000;

	for (unsigned int i = 0; i < 80 * 25; ++i) {
		vidmem[i].color.Set(Color::LightGray);
		vidmem[i].character = 0;
	}

	for (unsigned int i = 0; msg[i] != '\0'; ++i)
		vidmem[i].character = msg[i];

	for (;;) {
		__asm__ __volatile__ ("hlt");
	}
}

#endif /* VIDEO_H */
