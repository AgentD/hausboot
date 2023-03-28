/* SPDX-License-Identifier: ISC */
/*
 * BIOSBlockDevice.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef BIOS_BLOCK_DEVICE_H
#define BIOS_BLOCK_DEVICE_H

#include "BIOS/BiosDisk.h"
#include "IBlockDevice.h"

#include <cstdint>

class BIOSBlockDevice : public IBlockDevice {
public:
	bool Init(BiosDisk disk, uint32_t offset) {
		if (!disk.ReadDriveParameters(_geometry))
			return false;

		_disk = disk;
		_partStart = offset;
		return true;
	}

	virtual bool LoadSector(uint32_t index, void *buffer) override final {
		auto chs = _geometry.LBA2CHS(_partStart + index);

		return _disk.LoadSectors(chs, buffer, 1);
	}

	virtual uint16_t SectorSize() const override final {
		return 512;
	}

	const auto &DriveGeometry() const {
		return _geometry;
	}
private:
	BiosDisk::DriveGeometry _geometry;
	uint32_t _partStart;
	BiosDisk _disk{0};
};

#endif /* BIOS_BLOCK_DEVICE_H */
