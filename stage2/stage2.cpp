/* SPDX-License-Identifier: ISC */
/*
 * stage2.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "BIOS/BIOSTextMode.h"
#include "BIOS/MemoryMap.h"
#include "BIOS/BIOSBlockDevice.h"
#include "kernel/MultiBootHeader.h"
#include "kernel/MultiBootInfo.h"
#include "device/IBlockDevice.h"
#include "device/TextScreen.h"
#include "fs/FatDirentLong.h"
#include "fs/FatDirent.h"
#include "fs/FatSuper.h"
#include "fs/FatName.h"
#include "fs/FatFs.h"
#include "Stage2Header.h"
#include "StringUtil.h"
#include "pm86.h"

__attribute__ ((section(".header")))
uint8_t headerBlob[sizeof(Stage2Header)];
static auto *stage2header = (Stage2Header *)headerBlob;

static const char *bootConfigName = "BOOT.CFG";
static constexpr size_t bootConfigMaxSize = 4096;
static constexpr size_t multiBootMaxSearch = 8192;
static constexpr size_t heapMaxSize = 8192;

static TextScreen<BIOSTextMode> screen;
static MemoryMap<32> mmap;
static BIOSBlockDevice *part;
static FatFs *fs;

static bool haveKernel = false;
static void *kernelEntry = nullptr;

template<class T>
static TextScreen<T> &operator<< (TextScreen<T> &s, FatFs::FindResult type)
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

template<class T>
static TextScreen<T> &operator<< (TextScreen<T> &s, CHSPacked chs)
{
	s << chs.Cylinder() << "/" << chs.Head() << "/" << chs.Sector();
	return s;
}

template<class T>
static TextScreen<T> &operator<< (TextScreen<T> &s,
				  const BiosDisk::DriveGeometry &geom)
{
	s << geom.cylinders << "/" << geom.headsPerCylinder << "/"
	  << geom.sectorsPerTrack;
	return s;
}

/*****************************************************************************/

static bool MBFindHeader(const FatFile &finfo, uint32_t &offset,
			 MultiBootHeader &hdr)
{
	auto scanSize = finfo.size > multiBootMaxSearch ?
		multiBootMaxSearch : finfo.size;

	auto *buffer = (uint32_t *)malloc(1024);
	if (buffer == nullptr) {
		screen << "out of memory" << "\r\n";
		return false;
	}

	for (offset = 0; offset < scanSize; ) {
		auto ret = fs->ReadAt(finfo, (uint8_t *)buffer, offset, 1024);
		if (ret <= 0)
			goto fail;

		for (decltype(ret) i = 0; i < (ret / 4); ++i) {
			if (((uint32_t *)buffer)[i] != MultiBootHeader::Magic)
				continue;

			ret = fs->ReadAt(finfo, (uint8_t *)&hdr,
					 offset + i * 4, sizeof(hdr));
			if (ret <= 0)
				goto fail;

			if (hdr.IsValid()) {
				offset += i * 4;
				free(buffer);
				return true;
			}
		}

		offset += ret;
	}
fail:
	free(buffer);
	return false;
}

static bool MBLoadKernel(const FatFile &finfo, const MultiBootHeader &hdr,
			 uint32_t fileOffset)
{
	uint32_t fileStart, memStart, count;

	if (!hdr.ExtractMemLayout(fileOffset, finfo.size, fileStart,
				  memStart, count)) {
		screen << "Error: " << "Memory layout is broken!" << "\r\n";
		return false;
	}

	// TODO: check if target actually is in high-mem
	// TODO: check if we have enough memory available there

	// load it into memory
	screen << "Loading " << count << " bytes to #";
	screen.WriteHex(memStart);
	screen << "\r\n";

	uint8_t *buffer = (uint8_t *)malloc(1024);
	if (buffer == nullptr) {
		screen << "out of memory" << "\r\n";
		return false;
	}

	for (auto offset = fileStart; offset < (count - fileStart); ) {
		auto ret = fs->ReadAt(finfo, buffer, offset, 1024);

		if (ret == 0)
			break;

		if (ret < 0)
			return false;

		ProtectedModeCall(CopyMemory32, (void *)memStart, buffer, ret);

		memStart += ret;
		offset += ret;
	}

	free(buffer);

	ProtectedModeCall(ClearMemory32, (void *)memStart, hdr.BSSSize());
	return true;
}

static MultiBootInfo *MBGenInfo()
{
	auto *info = new MultiBootInfo();

	info->SetLoadName("hausboot");

	size_t count = mmap.Count();
	auto *mbMmap = new MultiBootMmap[count];

	for (size_t i = 0; i < count; ++i) {
		mbMmap[i] = mmap.At(i);
		mbMmap[i].SetNext(mbMmap + i + 1);
	}

	info->SetMemoryMap(mbMmap, count);
	return info;
}

extern "C" {
	void MBTrampoline(void *address, const MultiBootInfo *info);
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
			screen << ", type: " << it.TypeAsString() << "\r\n";
		}

		return true;
	}

	screen << "Unknown info type: " << what << "\r\n";
	return false;
}

static bool CmdMultiboot(const char *path)
{
	FatFile finfo;

	auto ret = fs->FindByPath(path, finfo);
	if (ret != FatFs::FindResult::Ok) {
		screen << path << ": " << ret << "\r\n";
		return false;
	}

	uint32_t offset;
	MultiBootHeader hdr;

	if (!MBFindHeader(finfo, offset, hdr)) {
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

	if (!MBLoadKernel(finfo, hdr, offset))
		return false;

	haveKernel = true;
	kernelEntry = hdr.EntryPoint();
	return true;
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
	ssize_t rdRet;

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

	rdRet = fs->ReadAt(finfo, (uint8_t *)fileBuffer, 0, finfo.size);
	if (rdRet < 0) {
		screen << "Error loading config file " << "\r\n";
		goto fail;
	}

	fileBuffer[rdRet] = '\0';

	// interpret it
	RunScript(fileBuffer);
	free(fileBuffer);

	// run the kernel
	if (haveKernel) {
		auto *info = MBGenInfo();

		ProtectedModeCall(MBTrampoline, kernelEntry, info);
	} else {
		screen << "No kernel loaded!" << "\r\n";
		goto fail;
	}
fail:
	for (;;) {
		__asm__ volatile("hlt");
	}
}
