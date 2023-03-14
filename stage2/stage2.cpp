/* SPDX-License-Identifier: ISC */
/*
 * stage2.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "TextScreen.h"
#include "stage2.h"

__attribute__ ((section(".header")))
Stage2Info header;

__attribute__ ((section(".entry")))
void main(void)
{
	TextScreen screen;

	screen.Reset();

	screen << "Hello from Stage 2!\n"
	       << "\tSector count: " << header.SectorCount() << "\n"
	       << "\tBIOS boot drive: " << header.BiosBootDrive() << "\n";

	for (;;) {
		__asm__ volatile("hlt");
	}
}
