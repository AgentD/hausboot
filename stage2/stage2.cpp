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
#include "stage2.h"

__attribute__ ((section(".header")))
uint8_t headerBlob[sizeof(Stage2Info)];

static const char *bootConfigName = "BOOT.CFG";

static auto *stage2header = (Stage2Info *)headerBlob;
static TextScreen screen;
static BiosDisk::DriveGeometry driveGeometry;
static auto *fatSuper = (FatSuper *)0x7C00;

static uint8_t *fatWindow;
static uint8_t *dataWindow;
static uint32_t currentFatSector;
static uint32_t currentDataCluster;

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

static bool LoadFatSector(uint32_t index)
{
	if (index >= fatSuper->SectorsPerFat()) {
		screen << "[BUG] tried to access out-of-bounds FAT sector: "
		       << index << " >= "
		       << fatSuper->SectorsPerFat() << "\r\n";
		return false;
	}

	if (index == currentFatSector)
		return true;

	auto lba = stage2header->BootMBREntry().StartAddressLBA() + index;
	lba += fatSuper->ReservedSectors();

	auto chs = driveGeometry.LBA2CHS(lba);
	auto disk = stage2header->BiosBootDrive();

	if (!disk.LoadSectors(chs, fatWindow, 1)) {
		screen << "Error loading disk sector " << lba
		       << " while accessing FAT index " << index
		       << "(CHS: " << chs << ")" << "\r\n";
		return false;
	}

	currentFatSector = index;
	return true;
}

static bool ReadFatIndex(uint32_t index, uint32_t &out)
{
	auto sector = (index * 4) / fatSuper->BytesPerSector();
	auto offset = (index * 4) % fatSuper->BytesPerSector();

	if (!LoadFatSector(sector))
		return false;

	out = *((uint32_t *)(fatWindow + offset));
	return true;
}

static bool LoadDataCluster(uint32_t index)
{
	if (index < 2) {
		screen << "[BUG] tried to access data cluster < 2"
		       << "\r\n";
		return false;
	}

	if (index == currentDataCluster)
		return true;

	auto sector = stage2header->BootMBREntry().StartAddressLBA();
	sector += fatSuper->ClusterIndex2Sector(index);

	auto chs = driveGeometry.LBA2CHS(sector);
	auto disk = stage2header->BiosBootDrive();

	if (!disk.LoadSectors(chs, dataWindow,
			      fatSuper->SectorsPerCluster())) {
		screen << "Error loading cluster " << index
		       << " from sector " << sector
		       << " (CHS: " << chs << ")" << "\r\n";
		return false;
	}

	currentDataCluster = index;
	return true;
}

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

			if (!LoadDataCluster(index))
				return false;

			if (!ReadFatIndex(index, next))
				return false;

			auto *entS = (FatDirent *)dataWindow;
			auto max = (fatSuper->SectorsPerCluster() *
				    fatSuper->BytesPerSector()) / sizeof(*entS);

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

			if (!LoadDataCluster(index))
				return false;

			if (!ReadFatIndex(index, next))
				return false;

			auto diff = fatSuper->SectorsPerCluster() *
				fatSuper->BytesPerSector();
			if (diff > fileSize)
				diff = fileSize;

			screen.WriteCharacters((char *)dataWindow, diff);

			fileSize -= diff;
			index = next;
		}

		return true;
	}
};

void main(void *heapPtr)
{
	screen.Reset();

	// allocate sliding windows to read from the disk
	fatWindow = (uint8_t *)heapPtr;
	heapPtr = fatWindow + fatSuper->BytesPerSector();

	dataWindow = (uint8_t *)heapPtr;
	heapPtr = dataWindow +
		fatSuper->BytesPerSector() * fatSuper->SectorsPerCluster();

	// Get drive geometry
	auto disk = stage2header->BiosBootDrive();

	if (!disk.ReadDriveParameters(driveGeometry)) {
		screen << "Error querying disk geometry!\r\n";
		goto fail;
	}

	screen << "Drive geometry (C/H/S): " << driveGeometry << "\r\n";

	// Initialize FAT and cluster sliding windows
	currentFatSector = 0xFFFFFFFF;
	currentDataCluster = 0xFFFFFFFF;

	// DUMP
	{
		FatFile finfo;
		finfo.cluster = fatSuper->RootDirIndex();
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
