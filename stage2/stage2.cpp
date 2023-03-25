/* SPDX-License-Identifier: ISC */
/*
 * stage2.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "BIOS/TextScreen.h"
#include "fs/FatSuper.h"
#include "stage2.h"

__attribute__ ((section(".header")))
uint8_t headerBlob[sizeof(Stage2Info)];

static auto *stage2header = (Stage2Info *)headerBlob;
static TextScreen screen;
static BiosDisk::DriveGeometry driveGeometry;
static auto *fatSuper = (FatSuper *)0x7C00;

static uint8_t *fatWindow;
static uint8_t *dataWindow;

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

	index += stage2header->BootMBREntry().StartAddressLBA();
	index += fatSuper->ReservedSectors();

	auto chs = driveGeometry.LBA2CHS(index);
	auto disk = stage2header->BiosBootDrive();

	if (!disk.LoadSectors(chs, fatWindow, 1)) {
		screen << "Error loading disk sector " << index
		       << " while accessing FAT (CHS: " << chs
		       << ")" << "\r\n";
		return false;
	}

	return true;
}

static bool LoadDataCluster(uint32_t index)
{
	if (index < 2) {
		screen << "[BUG] tried to access data cluster < 2"
		       << "\r\n";
		return false;
	}

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

	return true;
}

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

	// Test load a data cluster and a FAT sector
	if (!LoadDataCluster(3))
		goto fail;

	if (!LoadFatSector(0))
		goto fail;

	// DUMP
	for (int i = 0; i < 128; ++i) {
		screen.WriteHex(((uint32_t *)dataWindow)[i]);

		if ((i % 8) == 7) {
			screen << "\r\n";
		} else {
			screen << " ";
		}
	}
fail:
	for (;;) {
		__asm__ volatile("hlt");
	}
}
