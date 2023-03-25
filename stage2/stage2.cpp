/* SPDX-License-Identifier: ISC */
/*
 * stage2.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "BIOS/TextScreen.h"
#include "fs/FatDirentLong.h"
#include "stage2/Stage2Info.h"
#include "stage2/FatDisk.h"
#include "stage2/Heap.h"
#include "fs/FatDirent.h"
#include "fs/FatSuper.h"

__attribute__ ((section(".header")))
uint8_t headerBlob[sizeof(Stage2Info)];

static const char *bootConfigName = "BOOT.CFG";

static auto *stage2header = (Stage2Info *)headerBlob;
static TextScreen screen;
static FatDisk disk;

struct FatFile {
	uint32_t cluster;
	uint32_t size;
	FlagField<FatDirent::Flags, uint8_t> flags;
};

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

static bool FindInDirectory(const FatFile &in, const char *name, FatFile &out)
{
	bool found = false;

	auto cb = [name, &found, &out](FatDirent &ent) {
		char buffer[13];
		ent.NameToString(buffer);

		if (!StrEqual(buffer, name))
			return FatDisk::ScanVerdict::Ok;

		out.cluster = ent.ClusterIndex();
		out.size = ent.Size();
		out.flags = ent.EntryFlags();
		found = true;
		return FatDisk::ScanVerdict::Stop;
	};

	if (!disk.ForEachDirectoryEntry(in.cluster, cb))
		return false;

	if (!found) {
		screen << name << ": no such file or directory"
		       << "\r\n";
		return false;
	}

	return true;
}

static bool DumpToScreen(FatFile &f)
{
	auto cb = [](void *data, uint32_t size) {
		screen.WriteCharacters((char *)data, size);
		return FatDisk::ScanVerdict::Ok;
	};

	return disk.ForEachClusterInChain(f.cluster, f.size, cb);
}

void main(void *heapPtr)
{
	Heap heap(heapPtr);

	screen.Reset();

	auto ret = disk.Init(screen, *stage2header,
			     (const FatSuper *)0x7C00, heap);

	if (!ret)
		goto fail;

	{
		FatFile finfo;
		finfo.cluster = disk.RootDirIndex();
		finfo.size = 0;
		finfo.flags.Set(FatDirent::Flags::Directory);

		if (!FindInDirectory(finfo, bootConfigName, finfo))
			goto fail;

		if (!DumpToScreen(finfo))
			goto fail;
	}
fail:
	for (;;) {
		__asm__ volatile("hlt");
	}
}
