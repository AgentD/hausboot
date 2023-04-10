/* SPDX-License-Identifier: ISC */
/*
 * IBlockDevice.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef IBLOCK_DEVICE_H
#define IBLOCK_DEVICE_H

#include <cstdint>

class IBlockDevice {
public:
	virtual ~IBlockDevice() {
	}

	virtual bool LoadSector(uint32_t index, void *buffer) = 0;

	virtual uint16_t SectorSize() const = 0;
};

#endif /* IBLOCK_DEVICE_H */
