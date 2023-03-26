/* SPDX-License-Identifier: ISC */
/*
 * FatDisk.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef FAT_DISK_H
#define FAT_DISK_H

#include "stage2/Stage2Info.h"
#include "stage2/Heap.h"
#include "BIOS/TextScreen.h"
#include "BIOS/BiosDisk.h"
#include "fs/FatSuper.h"

static TextScreen &operator<< (TextScreen &s, CHSPacked chs)
{
	s << chs.Cylinder() << "/" << chs.Head() << "/" << chs.Sector();
	return s;
}

static TextScreen &operator<< (TextScreen &s,
			       const BiosDisk::DriveGeometry &geom)
{
	s << geom.cylinders << "/" << geom.headsPerCylinder << "/"
	  << geom.sectorsPerTrack;
	return s;
}

class FatDisk {
public:
	bool Init(TextScreen &ts, const Stage2Info &hdr,
		  const FatSuper *fsSuper, Heap &heap) {
		disk = hdr.BiosBootDrive();
		screen = &ts;
		partStart = hdr.BootMBREntry().StartAddressLBA();
		super = fsSuper;

		if (!disk.ReadDriveParameters(driveGeometry)) {
			(*screen) << "Error querying disk geometry!"
				  << "\r\n";
			return false;
		}

		currentFatSector = 0xFFFFFFFF;
		currentDataCluster = 0xFFFFFFFF;

		fatWindow = (uint8_t *)
			heap.AllocateRaw(super->BytesPerSector());
		dataWindow = (uint8_t *)heap.AllocateRaw(BytesPerCluster());
		return true;
	}

	size_t BytesPerCluster() const {
		return super->SectorsPerCluster() * super->BytesPerSector();
	}

	auto RootDirIndex() const {
		return super->RootDirIndex();
	}

	enum class ScanVerdict {
		Ok = 0,
		Error = 1,
		Stop = 2,
	};

	template<typename FUNCTOR>
	bool ForEachClusterInChain(uint32_t index, uint32_t size, FUNCTOR cb) {
		while (size > 0 && index < 0x0FFFFFF0) {
			uint32_t next;

			if (!LoadDataCluster(index))
				return false;

			if (!ReadFatIndex(index, next))
				return false;

			auto diff = BytesPerCluster();
			if (diff > size)
				diff = size;

			ScanVerdict verdict = cb((void *)dataWindow, diff);
			if (verdict == ScanVerdict::Error)
				return false;
			if (verdict == ScanVerdict::Stop)
				break;

			size -= diff;
			index = next;
		}

		return true;
	}

	template<typename FUNCTOR>
	bool ForEachDirectoryEntry(uint32_t index, FUNCTOR cb) {
		auto wrap = [&cb](void *data, uint32_t size) {
			auto *entS = (FatDirent *)data;
			auto max = size / sizeof(*entS);

			for (decltype(max) i = 0; i < max; ++i) {
				if (entS[i].IsLastInList())
					return ScanVerdict::Stop;
				if (entS[i].EntryFlags().IsSet(FatDirent::Flags::LongFileName))
					continue;
				if (entS[i].IsDummiedOut())
					continue;

				ScanVerdict ret = cb(entS[i]);
				if (ret != ScanVerdict::Ok)
					return ret;
			}

			return FatDisk::ScanVerdict::Ok;
		};

		return ForEachClusterInChain(index, 0xFFFFFFFF, wrap);
	}

	const auto &DriveGeometry() const {
		return driveGeometry;
	}
private:
	bool LoadDataCluster(uint32_t index) {
		if (index < 2) {
			*screen << "[BUG] tried to access data cluster < 2"
			       << "\r\n";
			return false;
		}

		if (index == currentDataCluster)
			return true;

		auto sector = partStart + super->ClusterIndex2Sector(index);
		auto chs = driveGeometry.LBA2CHS(sector);

		if (!disk.LoadSectors(chs, dataWindow,
				      super->SectorsPerCluster())) {
			*screen << "Error loading cluster " << index
			       << " from sector " << sector
			       << " (CHS: " << chs << ")" << "\r\n";
			return false;
		}

		currentDataCluster = index;
		return true;
	}

	bool LoadFatSector(uint32_t index) {
		if (index >= super->SectorsPerFat()) {
			*screen << "[BUG] tried to access out-of-bounds "
				"FAT sector: " << index << " >= "
			       << super->SectorsPerFat() << "\r\n";
			return false;
		}

		if (index == currentFatSector)
			return true;

		auto lba = partStart + super->ReservedSectors() + index;
		auto chs = driveGeometry.LBA2CHS(lba);

		if (!disk.LoadSectors(chs, fatWindow, 1)) {
			*screen << "Error loading disk sector " << lba
			       << " while accessing FAT index " << index
			       << "(CHS: " << chs << ")" << "\r\n";
			return false;
		}

		currentFatSector = index;
		return true;
	}

	bool ReadFatIndex(uint32_t index, uint32_t &out) {
		auto sector = (index * 4) / super->BytesPerSector();
		auto offset = (index * 4) % super->BytesPerSector();

		if (!LoadFatSector(sector))
			return false;

		out = *((uint32_t *)(fatWindow + offset));
		return true;
	}

	uint8_t *fatWindow;
	uint8_t *dataWindow;
	TextScreen *screen;
	const FatSuper *super;
	uint32_t currentFatSector;
	uint32_t currentDataCluster;
	BiosDisk::DriveGeometry driveGeometry;
	BiosDisk disk{0};
	uint32_t partStart;
};

#endif /* FAT_DISK_H */
