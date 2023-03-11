/* SPDX-License-Identifier: ISC */
/*
 * mbr.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "MBRTable.h"
#include "bios.h"
#include "video.h"

#include <cstdint>

static const char *msgResetDrv = "Drive reset failed!";
static const char *msgNoBootPart = "No boot partition found!";
static const char *msgFailLoad = "Failed to load VBR boot sector!";
static const char *msgNoMagic = "VBR boot sector has no magic number!";

static auto *bootSector = (uint16_t *)0x7C00;
static auto *partTable = (MBRTable *)(0x0600 + 446);

extern "C" {
	[[noreturn]] void CallVbr(uint32_t edx, const MBREntry *ent);
}

void main(uint32_t edx)
{
	uint16_t driveNumber = edx & 0x0FF;

	for (const auto &it : partTable->entries) {
		if (!it.IsBootable())
			continue;

		if (!ResetDrive(driveNumber))
			DumpMessageAndHang(msgResetDrv);

		if (!LoadSectors(driveNumber, it.StartAddressCHS(),
				 bootSector, 1)) {
			DumpMessageAndHang(msgFailLoad);
		}

		if (bootSector[255] != 0xAA55)
			DumpMessageAndHang(msgNoMagic);

		CallVbr(driveNumber, &it);
	}

	DumpMessageAndHang(msgNoBootPart);
}
