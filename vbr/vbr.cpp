/* SPDX-License-Identifier: ISC */
/*
 * vbr.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "bios.h"
#include "video.h"
#include "MBREntry.h"
#include "FatSuper.h"
#include "stage2.h"

static const char *msgErrLoad = "Error loading stage 2!";
static const char *msgErrBroken = "Stage 2 corrupted!";

void main(uint32_t edx, const MBREntry *ent)
{
	auto *super = (FatSuper *)0x7c00;

	// Get and sanitze number of reserved sectors
	auto count = super->ReservedSectors();
	if (count < 3)
		DumpMessageAndHang(msgErrLoad);

	count -= 2;

	if (count > 30)
		count = 30;

	// XXX: we boldly assume the partition to be cylinder
	// aligned, so the stupid CHS arithmetic won't overflow.
	CHSPacked src = ent->StartAddressCHS();
	uint8_t driveNum = edx & 0x0FF;

	src.SetSector(src.Sector() + 2);

	auto *dst = (uint8_t *)Stage2Location;

	if (!LoadSectors(driveNum, src, dst, count))
		DumpMessageAndHang(msgErrLoad);

	auto *hdr = (Stage2Info *)dst;

	if (!hdr->Verify(count))
		DumpMessageAndHang(msgErrBroken);

	// Enter stage 2
	hdr->SetBiosBootDrive(driveNum);

	goto *(dst + sizeof(*hdr));
}
