/* SPDX-License-Identifier: ISC */
/*
 * stage2.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "BIOS/TextScreen.h"
#include "fs/FatDirentLong.h"
#include "fs/FatDirent.h"
#include "fs/FatSuper.h"
#include "FatDisk.h"
#include "stage2.h"

__attribute__ ((section(".header")))
uint8_t headerBlob[sizeof(Stage2Info)];

static const char *bootConfigName = "BOOT.CFG";

static auto *stage2header = (Stage2Info *)headerBlob;
static TextScreen screen;
static FatDisk disk;

static bool StrEqual(const char *a, const char *b)
{
	for (;;) {
		if (*a != *b)
			return false;
		if (*a == '\0')
			break;
		++a;
		++b;
	}
	return true;
}

struct FatFile {
	uint32_t cluster;
	uint32_t size;
	FlagField<FatDirent::Flags, uint8_t> flags;

	bool FindInDirectory(const char *name, FatFile &out) const {
		uint32_t index = cluster;

		do {
			uint32_t next;

			if (!disk.LoadDataCluster(index))
				return false;

			if (!disk.ReadFatIndex(index, next))
				return false;

			auto *entS = (FatDirent *)disk.dataWindow;
			auto max = disk.BytesPerCluster() / sizeof(*entS);

			for (decltype(max) i = 0; i < max; ++i) {
				if (entS[i].IsLastInList())
					return true;
				if (entS[i].IsDummiedOut())
					continue;
				if (entS[i].EntryFlags().IsSet(FatDirent::Flags::LongFileName))
					continue;

				char buffer[13];
				entS[i].NameToString(buffer);

				if (StrEqual(buffer, name)) {
					out.cluster = entS[i].ClusterIndex();
					out.size = entS[i].Size();
					out.flags = entS[i].EntryFlags();
					return true;
				}
			}

			index = next;
		} while (index < 0x0FFFFFF0);

		screen << name << ": no such file or directory" << "\r\n";
		return false;
	}

	bool DumpToScreen() {
		uint32_t index = cluster;
		uint32_t fileSize = size;

		while (fileSize > 0 && index < 0x0FFFFFF0) {
			uint32_t next;

			if (!disk.LoadDataCluster(index))
				return false;

			if (!disk.ReadFatIndex(index, next))
				return false;

			auto diff = disk.BytesPerCluster();
			if (diff > fileSize)
				diff = fileSize;

			screen.WriteCharacters((char *)disk.dataWindow, diff);

			fileSize -= diff;
			index = next;
		}

		return true;
	}
};

void main(void *heapPtr)
{
	screen.Reset();

	auto ret = disk.Init(screen,
			     stage2header->BiosBootDrive(),
			     stage2header->BootMBREntry(),
			     (const FatSuper *)0x7C00, heapPtr);

	if (!ret)
		goto fail;

	{
		FatFile finfo;
		finfo.cluster = disk.RootDirIndex();
		finfo.size = 0;
		finfo.flags.Set(FatDirent::Flags::Directory);

		if (!finfo.FindInDirectory(bootConfigName, finfo))
			goto fail;

		if (!finfo.DumpToScreen())
			goto fail;
	}
fail:
	for (;;) {
		__asm__ volatile("hlt");
	}
}
