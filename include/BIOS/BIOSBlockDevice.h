/* SPDX-License-Identifier: ISC */
/*
 * BIOSBlockDevice.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef BIOS_BLOCK_DEVICE_H
#define BIOS_BLOCK_DEVICE_H

#include "BIOS/BiosDisk.h"
#include "device/IBlockDevice.h"

#include <cstdint>

class BIOSBlockDevice : public IBlockDevice {
public:
	BIOSBlockDevice() = delete;

	BIOSBlockDevice(BiosDisk disk, uint32_t offset) {
		if (!disk.ReadDriveParameters(_geometry)) {
			_isInitialized = false;
			return;
		}

		_disk = disk;
		_partStart = offset;
		_isInitialized = true;
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

	bool IsInitialized() const {
		return _isInitialized;
	}
private:
	bool _isInitialized;
	BiosDisk::DriveGeometry _geometry;
	uint32_t _partStart;
	BiosDisk _disk{0};
};

#endif /* BIOS_BLOCK_DEVICE_H */
