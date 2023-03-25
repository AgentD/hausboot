/* SPDX-License-Identifier: ISC */
/*
 * BiosDisk.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef BIOS_H
#define BIOS_H

#include <cstdint>
#include "part/CHSPacked.h"

class BiosDisk {
public:
	BiosDisk() = delete;

	BiosDisk(uint8_t driveNum) : _driveNum(driveNum) {
	}

	inline bool Reset() {
		int error;
		__asm__ __volatile__ ("int $0x13\r\n"
				      "sbb %0,%0"
				      : "=r"(error)
				      : "a"(0), "d"(_driveNum));
		return error == 0;
	}

	inline bool LoadSectors(CHSPacked source,
				void *out, uint8_t count) const {
		uint16_t dx = (static_cast<uint16_t>(source.Head()) << 8) |
			_driveNum;
		uint16_t cx = (source.Cylinder() << 8) |
			((source.Cylinder() & 0x0300) >> 2) | source.Sector();

		int error;

		__asm__ __volatile__ ("int $0x13\r\n"
				      "sbb %0,%0"
				      : "=r"(error)
				      : "a"(0x0200 | count), "c"(cx),
					"b"(out), "d"(dx));
		return error == 0;
	}

	struct DriveGeometry {
		uint16_t cylinders;
		uint16_t headsPerCylinder;
		uint16_t sectorsPerTrack;

		CHSPacked LBA2CHS(uint32_t lba) const {
			CHSPacked out;

			out.SetCylinder(lba / ((uint32_t)headsPerCylinder *
					       sectorsPerTrack));
			out.SetHead((lba / sectorsPerTrack) % headsPerCylinder);
			out.SetSector((lba % sectorsPerTrack) + 1);

			return out;
		}
	};

	bool ReadDriveParameters(DriveGeometry &out) const {
		uint16_t cxOut, dxOut;
		int error;
		__asm__ __volatile__ ("int $0x13\r\n"
				      "sbb %0,%0"
				      : "=r"(error), "=cx"(cxOut), "=dx"(dxOut)
				      : "a"(0x0800), "d"(_driveNum),
					"e"(0), "D"(0)
				      : "bx");
		if (error != 0)
			return false;

		out.cylinders = (((cxOut & 0x0C0) << 2) | ((cxOut >> 8) & 0x0FF)) + 1;
		out.sectorsPerTrack = cxOut & 0x3F;
		out.headsPerCylinder = ((dxOut >> 8) & 0x0FF) + 1;
		return true;
	}
private:
	uint8_t _driveNum;
};

static_assert(sizeof(BiosDisk) == sizeof(uint8_t));

#endif /* BIOS_H */
