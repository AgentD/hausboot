/* SPDX-License-Identifier: ISC */
/*
 * stage2.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "BIOS/TextScreen.h"
#include "BIOS/MemoryMap.h"
#include "fs/FatDirentLong.h"
#include "stage2/Stage2Info.h"
#include "stage2/FatDisk.h"
#include "stage2/Heap.h"
#include "fs/FatDirent.h"
#include "fs/FatSuper.h"

__attribute__ ((section(".header")))
uint8_t headerBlob[sizeof(Stage2Info)];

static const char *bootConfigName = "BOOT.CFG";
static size_t bootConfigMaxSize = 4096;

static auto *stage2header = (Stage2Info *)headerBlob;
static TextScreen screen;
static FatDisk disk;
static MemoryMap<32> mmap;

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

static bool IsSpace(int x)
{
	return x == ' ' || x == '\t';
}

static bool IsAlnum(int x)
{
	return (x >= 'A' && x <= 'Z') || (x >= 'a' && x <= 'z') ||
		(x >= '0' && x <= '9');
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

static bool LoadFileToBuffer(FatFile &f, void *buffer)
{
	auto cb = [&buffer](void *data, uint32_t size) {
		for (uint32_t i = 0; i < size; ++i)
			((uint8_t *)buffer)[i] = ((uint8_t *)data)[i];

		buffer = (uint8_t *)buffer + size;
		return FatDisk::ScanVerdict::Ok;
	};

	return disk.ForEachClusterInChain(f.cluster, f.size, cb);
}

/*****************************************************************************/

static bool CmdEcho(const char *line)
{
	screen << line << "\r\n";
	return true;
}

static bool CmdInfo(const char *what)
{
	if (StrEqual(what, "disk")) {
		auto geom = disk.DriveGeometry();
		auto lba = stage2header->BootMBREntry().StartAddressLBA();
		auto chs = geom.LBA2CHS(lba);

		screen << "Boot disk: " << "\r\n"
		       << "    geometry (C/H/S): " << geom << "\r\n"
		       << "Boot partition: " << "\r\n"
		       << "    LBA: " << lba << "\r\n"
		       << "    CHS: " << chs << "\r\n";

		return true;
	}

	if (StrEqual(what, "memory")) {
		screen << "Memory:" << "\r\n";

		for (const auto &it : mmap) {
			screen << "    base: ";
			screen.WriteHex(it.BaseAddress());
			screen << ", size: ";
			screen.WriteHex(it.Size());
			screen << ", type: " << it.Type() << "\r\n";
		}

		return true;
	}

	screen << "Unknown info type: " << what << "\r\n";
	return false;
}

static const struct {
	const char *name;
	bool (*callback)(const char *arg);
} commands[] = {
	{ "echo", CmdEcho },
	{ "info", CmdInfo },
};

static void RunScript(char *ptr)
{
	while (*ptr != '\0') {
		// isolate the current line
		char *line = ptr;

		while (*ptr != '\0' && *ptr != '\n')
			++ptr;

		if (*ptr == '\n')
			*(ptr++) = '\0';

		// isolate command string
		while (*line == ' ' || *line == '\t')
			++line;

		if (!IsAlnum(*line))
			continue;

		const char *cmd = line;

		while (IsAlnum(*line))
			++line;

		if (!IsSpace(*line) || *cmd == '\0' || *cmd == '#')
			continue;

		*(line++) = '\0';

		while (IsSpace(*line))
			++line;

		const char *arg = line;

		// dispatch
		bool found = false;

		for (const auto &it : commands) {
			if (StrEqual(it.name, cmd)) {
				if (!it.callback(arg))
					return;
				found = true;
				break;
			}
		}

		if (!found) {
			screen << "Error, unknown command: " << cmd << "\r\n";
			return;
		}
	}
}

/*****************************************************************************/

void main(void *heapPtr)
{
	Heap heap(heapPtr);
	char *fileBuffer;
	FatFile finfo;

	// initialization
	screen.Reset();

	if (!mmap.Load()) {
		screen << "Error loading BIOS memory map!" << "\r\n";
		goto fail;
	}

	if (!disk.Init(screen, *stage2header,
		       (const FatSuper *)0x7C00, heap)) {
		goto fail;
	}

	// find the boot loader config file
	finfo.cluster = disk.RootDirIndex();
	finfo.size = 0;
	finfo.flags.Set(FatDirent::Flags::Directory);

	if (!FindInDirectory(finfo, bootConfigName, finfo))
		goto fail;

	if (finfo.size > bootConfigMaxSize) {
		screen << bootConfigName << ": too big (max size: "
		       << bootConfigMaxSize << ")" << "\r\n";
		goto fail;
	}

	// load it into memory
	fileBuffer = (char *)heap.AllocateRaw(finfo.size + 1);

	if (!LoadFileToBuffer(finfo, fileBuffer))
		goto fail;

	fileBuffer[finfo.size] = '\0';

	// interpret it
	RunScript(fileBuffer);
fail:
	for (;;) {
		__asm__ volatile("hlt");
	}
}
