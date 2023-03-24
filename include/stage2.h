/* SPDX-License-Identifier: ISC */
/*
 * stage2.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef STAGE2_H
#define STAGE2_H

#include <cstdint>
#include <cstddef>

#include "MBREntry.h"
#include "BiosDisk.h"

constexpr uint32_t Stage2Magic = 0xD0D0CACA;
constexpr uint16_t Stage2Location = 0x1000;

struct Stage2Info {
public:
	void SetSectorCount(uint32_t size) {
		_sectorCount = size / 512;

		if (size % 512)
			_sectorCount += 1;
	}

	size_t SectorCount() const {
		return _sectorCount;
	}

	void UpdateChecksum() {
		_checksum = 0;
		_checksum = ~(ComputeChecksum()) + 1;
	}

	uint32_t ComputeChecksum() const {
		auto *ptr = (uint32_t *)this;
		auto count = _sectorCount * 128;
		uint32_t acc = 0;

		while (count--)
			acc += *(ptr++);

		return acc;
	}

	bool Verify(uint32_t maxSectors) const {
		if (_magic != Stage2Magic)
			return false;

		if (_sectorCount == 0 || _sectorCount > maxSectors)
			return false;

		return (_checksum != 0) && (ComputeChecksum() == 0);
	}

	void SetBiosBootDrive(BiosDisk drive) {
		_biosBootDrive = drive;
	}

	BiosDisk BiosBootDrive() const {
		return _biosBootDrive;
	}

	void SetBootMBREntry(const MBREntry &ent) {
		_bootMbrEntry = ent;
	}

	const MBREntry &BootMBREntry() const {
		return _bootMbrEntry;
	}
private:
	const uint32_t _magic = Stage2Magic;
	uint32_t _checksum = 0;
	uint16_t _sectorCount = 0;
	BiosDisk _biosBootDrive{0};
	const uint8_t _pad0 = 0;
	MBREntry _bootMbrEntry{};
};

static_assert(sizeof(Stage2Info) == 28);

#endif /* STAGE2_H */
