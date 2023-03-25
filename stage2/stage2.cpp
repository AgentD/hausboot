/* SPDX-License-Identifier: ISC */
/*
 * stage2.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "BIOS/TextScreen.h"
#include "stage2.h"

__attribute__ ((section(".header")))
Stage2Info header;

static uint16_t cylinders = 0;
static uint16_t sectorsPerTrack = 0;
static uint16_t headsPerCylinder = 0;

static CHSPacked LBA2CHS(uint32_t lba)
{
	CHSPacked out;

	out.SetCylinder(lba / ((uint32_t)headsPerCylinder * sectorsPerTrack));
	out.SetHead((lba / sectorsPerTrack) % headsPerCylinder);
	out.SetSector((lba % sectorsPerTrack) + 1);

	return out;
}

void main(void *heapPtr)
{
	TextScreen screen;

	screen.Reset();

	auto lba = header.BootMBREntry().StartAddressLBA();
	auto chs = header.BootMBREntry().StartAddressCHS();

	screen << "Hello from Stage 2!\r\n"
	       << "    Sector count: " << header.SectorCount() << "\r\n"
	       << "    Boot Partition LBA: " << lba << "\r\n"
	       << "    CHS: " << chs.Cylinder() << "/" << chs.Head() << "/"
	       << chs.Sector() << "\r\n"
	       << "Heap start: " << heapPtr << "\r\n";

	auto disk = header.BiosBootDrive();

	if (!disk.ReadDriveParameters(headsPerCylinder, cylinders, sectorsPerTrack)) {
		screen << "Error querying disk geometry!\r\n";
		goto fail;
	}

	screen << "Disk geometry:\r\n"
	       << "    Heads/Cylinder: " << headsPerCylinder << "\r\n"
	       << "    Sectors/Track: " << sectorsPerTrack << "\r\n";
fail:
	for (;;) {
		__asm__ volatile("hlt");
	}
}
