/* SPDX-License-Identifier: ISC */
/*
 * vbr.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "BIOS/BiosDisk.h"
#include "BIOS/BIOSTextMode.h"
#include "part/MBREntry.h"
#include "fs/FatSuper.h"
#include "Stage2Header.h"

static const char *msgErrLoad = "Error loading stage 2!";
static const char *msgErrBroken = "Stage 2 corrupted!";

extern "C" {
	void *main(BiosDisk disk, const MBREntry *ent);
}

void *main(BiosDisk disk, const MBREntry *ent)
{
	auto *super = (FatSuper *)0x7c00;
	MBREntry part = *ent;

	// Get and sanitze number of reserved sectors
	auto count = super->ReservedSectors();
	if (count < 3)
		DumpMessageAndHang(msgErrLoad);

	count -= 2;

	if (count > 30)
		count = 30;

	// XXX: we boldly assume the partition to be cylinder
	// aligned, so the stupid CHS arithmetic won't overflow.
	CHSPacked src = part.StartAddressCHS();

	src.SetSector(src.Sector() + 2);

	auto *dst = (uint8_t *)Stage2Location;

	if (!disk.LoadSectors(src, dst, count))
		DumpMessageAndHang(msgErrLoad);

	auto *hdr = (Stage2Header *)dst;

	if (!hdr->Verify(count))
		DumpMessageAndHang(msgErrBroken);

	// Enter stage 2
	hdr->SetBiosBootDrive(disk);
	hdr->SetBootMBREntry(part);

	return dst + sizeof(*hdr);
}
