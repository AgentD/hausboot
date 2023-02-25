#ifndef BIOS_H
#define BIOS_H

#include <cstdint>
#include "CHSPacked.h"

static inline bool ResetDrive(uint16_t driveNum)
{
	int error;
	__asm__ __volatile__ ("int $0x13\r\n"
			      "sbb %0,%0"
			      : "=r"(error)
			      : "a"(0), "d"(driveNum));
	return error == 0;
}

static inline bool LoadSector(uint8_t driveNum, CHSPacked source, void *out)
{
	uint16_t dx = (static_cast<uint16_t>(source.Head()) << 8) | driveNum;
	uint16_t cx = (source.Cylinder() << 8) |
		((source.Cylinder() & 0x0300) >> 2) | source.Sector();

	int error;

	__asm__ __volatile__ ("int $0x13\r\n"
			      "sbb %0,%0"
			      : "=r"(error)
			      : "a"(0x0201), "c"(cx), "b"(out), "d"(dx));
	return error == 0;
}

#endif /* BIOS_H */
