/* SPDX-License-Identifier: ISC */
/*
 * kernel.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "kernel/MultiBootInfo.h"
#include "device/TextScreen.h"

extern "C" {
	void multiboot_main(const MultiBootInfo *info, uint32_t signature);
};

static void PrintMemoryMap(TextScreen &s, const MultiBootInfo *info)
{
	const auto *mmap = info->MemoryMapBegin();
	const auto *mmapEnd = info->MemoryMapEnd();

	if (mmap == nullptr || mmapEnd == nullptr) {
		s << "Boot loader did not provide a memory map!\n";
		return;
	}

	s << "Multiboot memory map:\n";

	for (; mmap < mmapEnd; mmap = mmap->Next()) {
		auto start = mmap->BaseAddress();
		auto end = start;

		if (mmap->Size() > 0) {
			end += mmap->Size();

			if (end < start) {
				end = 0xFFFF'FFFF'FFFF'FFFF;
			} else {
				end -= 1;
			}
		}

		s.WriteHex(start);
		s << " | ";
		s.WriteHex(end);
		s << '\n';
	}
}

void multiboot_main(const MultiBootInfo *info, uint32_t signature)
{
	TextScreen s;

	s.SetColor(TextScreen::Color::White, TextScreen::Color::Blue);
	s.Clear();

	s << "Hello 32 bit world!\n";

	if (signature != 0x2BADB002) {
		s << "Multi boot signature is broken!\n";
		goto fail;
	}

	s << "Boot loader name: " << info->BootLoaderName() << '\n';

	PrintMemoryMap(s, info);
fail:
	for (;;)
		__asm__ ("hlt");
}
