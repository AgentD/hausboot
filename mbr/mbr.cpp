/* SPDX-License-Identifier: ISC */
/*
 * mbr.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "TextScreen.h"
#include "MBRTable.h"
#include "BiosDisk.h"

#include <cstdint>

static const char *msgResetDrv = "Drive reset failed!";
static const char *msgNoBootPart = "No boot partition found!";
static const char *msgFailLoad = "Failed to load VBR boot sector!";
static const char *msgNoMagic = "VBR boot sector has no magic number!";

static auto *bootSector = (uint16_t *)0x7C00;
static auto *partTable = (MBRTable *)(0x0600 + 446);

extern "C" {
	[[noreturn]] void CallVbr(BiosDisk disk, const MBREntry *ent);
}

void main(BiosDisk disk)
{
	for (const auto &it : partTable->entries) {
		if (!it.IsBootable())
			continue;

		if (!disk.Reset())
			DumpMessageAndHang(msgResetDrv);

		if (!disk.LoadSectors(it.StartAddressCHS(), bootSector, 1))
			DumpMessageAndHang(msgFailLoad);

		if (bootSector[255] != 0xAA55)
			DumpMessageAndHang(msgNoMagic);

		CallVbr(disk, &it);
	}

	DumpMessageAndHang(msgNoBootPart);
}
