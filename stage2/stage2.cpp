/* SPDX-License-Identifier: ISC */
/*
 * stage2.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "BIOS/TextScreen.h"
#include "BIOS/MemoryMap.h"
#include "BIOS/BIOSBlockDevice.h"
#include "kernel/MultiBootHeader.h"
#include "stage2/Stage2Info.h"
#include "fs/FatDirentLong.h"
#include "fs/FatDirent.h"
#include "fs/FatSuper.h"
#include "fs/FatName.h"
#include "fs/FatFs.h"
#include "IBlockDevice.h"
#include "StringUtil.h"
#include "pm86.h"

__attribute__ ((section(".header")))
uint8_t headerBlob[sizeof(Stage2Info)];
static auto *stage2header = (Stage2Info *)headerBlob;

static const char *bootConfigName = "BOOT.CFG";
static constexpr size_t bootConfigMaxSize = 4096;
static constexpr size_t multiBootMaxSearch = 8192;
static constexpr size_t heapMaxSize = 8192;

static TextScreen screen;
static MemoryMap<32> mmap;
static BIOSBlockDevice *part;
static FatFs *fs;

static TextScreen &operator<< (TextScreen &s, MemoryMapEntry::MemType type)
{
	const char *str = "unknown";

	switch (type) {
	case MemoryMapEntry::MemType::Usable: str = "free"; break;
	case MemoryMapEntry::MemType::Reserved: str = "reserved"; break;
	case MemoryMapEntry::MemType::ACPI: str = "ACPI"; break;
	case MemoryMapEntry::MemType::Preserve: str = "preserve"; break;
	default:
		break;
	}

	s << str;
	return s;
}

static TextScreen &operator<< (TextScreen &s, FatFs::FindResult type)
{
	const char *str = "no such file or directory";

	switch (type) {
	case FatFs::FindResult::Ok: str = "ok"; break;
	case FatFs::FindResult::NameInvalid: str = "name invalid"; break;
	case FatFs::FindResult::IOError: str = "I/O error"; break;
	case FatFs::FindResult::NotDir:
		str = "component is not a directory";
		break;
	default:
		break;
	}

	s << str;
	return s;
}

static TextScreen &operator<< (TextScreen &s, CHSPacked chs)
{
	s << chs.Cylinder() << "/" << chs.Head() << "/" << chs.Sector();
	return s;
}

static TextScreen &operator<< (TextScreen &s,
			       const BiosDisk::DriveGeometry &geom)
{
	s << geom.cylinders << "/" << geom.headsPerCylinder << "/"
	  << geom.sectorsPerTrack;
	return s;
}

static bool LoadFileToBuffer(FatFile &f, void *buffer)
{
	auto cb = [&buffer](void *data, uint32_t size) {
		for (uint32_t i = 0; i < size; ++i)
			((uint8_t *)buffer)[i] = ((uint8_t *)data)[i];

		buffer = (uint8_t *)buffer + size;
		return FatFs::ScanVerdict::Ok;
	};

	return fs->ForEachClusterInChain(f.cluster, f.size, cb);
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
		auto geom = part->DriveGeometry();
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

static bool CmdMultiboot(const char *path)
{
	FatFile finfo;

	screen << "multiboot: ";

	// Try to locate the file
	auto ret = fs->FindByPath(path, finfo);
	if (ret != FatFs::FindResult::Ok) {
		screen << path << ": " << ret << "\r\n";
		return false;
	}

	screen << "found `" << path
	       << "`: cluster " << finfo.cluster
	       << ", size: " << finfo.size << "\r\n";

	// Sift through the file to find the header
	MultiBootHeader hdr;
	uint32_t offset = 0;
	bool found = false;

	auto scanSize = finfo.size > multiBootMaxSearch ?
		multiBootMaxSearch : finfo.size;

	if (scanSize >= sizeof(hdr))
		scanSize -= sizeof(hdr);

	auto sarchCb = [&hdr, &found, &offset](void *data, uint32_t size) {
		if (size >= sizeof(hdr))
			size -= sizeof(hdr);

		for (uint32_t i = 0; i < (size / 4); ++i) {
			const auto *tryHdr = (MultiBootHeader *)((uint32_t *)data + i);

			if (tryHdr->IsValid()) {
				hdr = *tryHdr;
				found = true;
				offset += 4 * i;
				return FatFs::ScanVerdict::Stop;
			}
		}

		offset += size;
		return FatFs::ScanVerdict::Ok;
	};

	if (!fs->ForEachClusterInChain(finfo.cluster, scanSize, sarchCb))
		return false;

	// make sure we have a header and it makes sense
	if (!found) {
		screen << "Error: " << "No multiboot header found!" << "\r\n";
		return false;
	}

	if (hdr.Flags().IsSet(MultiBootHeader::KernelFlags::WantVidmode)) {
		screen << "Error: "
		       << "video mode info required (unsupported)!" << "\r\n";
		return false;
	}

	if (!hdr.Flags().IsSet(MultiBootHeader::KernelFlags::HaveLayoutInfo)) {
		screen << "Error: "
		       << "No memory layout provided (unsupported)!" << "\r\n";
		return false;
	}

	// Determine memory layout
	uint32_t fileStart, memStart, count;

	if (!hdr.ExtractMemLayout(offset, finfo.size, fileStart,
				  memStart, count)) {
		screen << "Error: " << "Memory layout is broken!" << "\r\n";
		return false;
	}

	// load it into memory
	screen << "Loading " << count << " bytes from `" << path << "`@"
	       << fileStart << " to #";
	screen.WriteHex(memStart);
	screen << "\r\n";

	offset = 0;

	auto loadCb = [&offset, &fileStart,
		       &memStart, &count](void *data, uint32_t size) {
		if (offset < fileStart) {
			auto diff = fileStart - offset;
			if (size <= diff) {
				offset += size;
				return FatFs::ScanVerdict::Ok;
			}

			offset += diff;
			data = (char *)data + diff;
			size -= diff;
		}

		if (size > count)
			size = count;

		const auto *dst = (void *)memStart;

		screen << ".";

		ProtectedModeCall(CopyMemory32, dst, data, size);

		memStart += size;
		count -= size;
		return count > 0 ? FatFs::ScanVerdict::Ok :
			FatFs::ScanVerdict::Stop;
	};

	if (!fs->ForEachClusterInChain(finfo.cluster, finfo.size, loadCb))
		return false;

	ProtectedModeCall(ClearMemory32, (void *)memStart, hdr.BSSSize());

	screen << "done" << "\r\n";

	// "now hold on to your butts..."
	ProtectedModeCall((void(*)(void))hdr.EntryPoint());
	return false;
}

static const struct {
	const char *name;
	bool (*callback)(const char *arg);
} commands[] = {
	{ "echo", CmdEcho },
	{ "info", CmdInfo },
	{ "multiboot", CmdMultiboot },
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
	FatFs::FindResult ret;
	char *fileBuffer;
	FatFile finfo;

	// initialization
	HeapInit(heapPtr, heapMaxSize);

	screen.Reset();

	if (!EnableA20()) {
		screen << "Error enabling A20 line!" << "\r\n";
		goto fail;
	}

	if (!mmap.Load()) {
		screen << "Error loading BIOS memory map!" << "\r\n";
		goto fail;
	}

	part = new BIOSBlockDevice(stage2header->BiosBootDrive(),
				   stage2header->BootMBREntry().StartAddressLBA());

	if (part == nullptr || !part->IsInitialized()) {
		screen << "Error initializing FAT partition wrapper!" << "\r\n";
		goto fail;
	}

	fs = new FatFs(part, *((FatSuper *)0x7C00));
	if (fs == nullptr) {
		screen << "Error initializing FAT FS wrapper!" << "\r\n";
		goto fail;
	}

	// find the boot loader config file
	ret = fs->FindByPath(bootConfigName, finfo);
	if (ret != FatFs::FindResult::Ok) {
		screen << bootConfigName << ": " << ret << "\r\n";
		goto fail;
	}

	if (finfo.size > bootConfigMaxSize) {
		screen << bootConfigName << ": too big (max size: "
		       << bootConfigMaxSize << ")" << "\r\n";
		goto fail;
	}

	// load it into memory
	fileBuffer = (char *)malloc(finfo.size + 1);

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
