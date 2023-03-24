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

	for (;;) {
		__asm__ volatile("hlt");
	}
}
