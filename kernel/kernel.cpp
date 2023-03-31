/* SPDX-License-Identifier: ISC */
/*
 * kernel.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "kernel/MultiBootInfo.h"
#include "device/VideoMemory.h"
#include "TextScreen.h"

extern "C" {
	void multiboot_main(const MultiBootInfo *info, uint32_t signature);
};

static void PrintMemoryMap(TextScreen<VideoMemory>& s,
			   const MultiBootInfo *info)
{
	const auto *mmap = info->MemoryMapBegin();
	const auto *mmapEnd = info->MemoryMapEnd();

	if (mmap == nullptr || mmapEnd == nullptr) {
		s << "Boot loader did not provide a memory map!" << "\r\n";
		return;
	}

	s << "Multiboot memory map:" << "\r\n";

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
		s << "\r\n";
	}
}

void multiboot_main(const MultiBootInfo *info, uint32_t signature)
{
	TextScreen<VideoMemory> s;

	s.Driver().SetColor(VideoMemory::Color::White,
			    VideoMemory::Color::Blue);
	s.Reset();

	s << "Hello 32 bit world!" << "\r\n";

	if (signature != 0x2BADB002) {
		s << "Multi boot signature is broken!" << "\r\n";
		goto fail;
	}

	s << "Boot loader name: " << info->BootLoaderName() << "\r\n";

	PrintMemoryMap(s, info);
fail:
	for (;;)
		__asm__ ("hlt");
}
