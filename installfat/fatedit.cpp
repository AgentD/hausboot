/* SPDX-License-Identifier: ISC */
/*
 * fatedit.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "FatSuper.h"
#include "FatFsInfo.h"
#include "FatDirent.h"
#include "FatDirentLong.h"
#include "util.h"

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>
#include <cctype>
#include <string>

struct DirEntry {
	std::string shortName;
	std::string longName;
	uint32_t size;
	uint32_t firstCluster;
	bool isDirectory;
};

static FatSuper super;
static FatFsInfo fsinfo;
static const char *filename;
static std::vector<uint8_t> _fatRaw;
static int fd = -1;

static size_t ClusterIndex2Sector(size_t N)
{
	auto FirstDataSector = super.ReservedSectors() +
		super.NumFats() * super.SectorsPerFat();

	return ((N - 2) * super.SectorsPerCluster()) + FirstDataSector;
}

static uint32_t FatEntryRead(size_t N)
{
	auto idx = N * 4;

	// XXX: std::vector<T>::at() does bounds checking, this should throw
	// an exception if N is bonkers.
	uint32_t out = _fatRaw.at(idx++);
	out |= static_cast<uint32_t>(_fatRaw.at(idx++)) << 8;
	out |= static_cast<uint32_t>(_fatRaw.at(idx++)) << 16;
	out |= static_cast<uint32_t>(_fatRaw.at(idx++)) << 24;

	return out & 0x0FFFFFFF;
}

static void FatEntrySet(size_t N, uint32_t value)
{
	auto idx = N * 4;

	if (_fatRaw.size() < 4 || idx >= (_fatRaw.size() - 3))
		return;

	_fatRaw[idx++] = value & 0xFF;
	_fatRaw[idx++] = (value >> 8) & 0xFF;
	_fatRaw[idx++] = (value >> 16) & 0xFF;
	_fatRaw[idx++] = (value >> 24) & 0x0F;
}

static uint32_t FindFreeCluster(void)
{
	auto fatSectors = super.SectorsPerFat() * super.NumFats();
	auto total = super.SectorCount();

	if (total <= fatSectors)
		return 0x0FFFFFFF8;
	total -= fatSectors;

	if (total <= super.ReservedSectors())
		return 0x0FFFFFFF8;
	total -= super.ReservedSectors();

	total /= super.SectorsPerCluster();
	if (total < 1)
		return 0x0FFFFFFF8;

	for (size_t i = 0; i < total; ++i) {
		if (FatEntryRead(i) == 0)
			return i;
	}

	return 0x0FFFFFFF8;
}

class FileReader {
public:
	FileReader(uint32_t idx, uint32_t size) : _cluster(idx), _offset(0),
						  _size(size), _totalRead(0) {
	}

	ssize_t Read(void *data, size_t size) {
		ssize_t total = 0;

		while (size > 0) {
			auto diff = DataLeftInCluster();

			if (diff == 0) {
				NextCluster();

				diff = DataLeftInCluster();
				if (diff == 0)
					break;
			}

			uint64_t offset = ClusterIndex2Sector(_cluster) * super.BytesPerSector() + _offset;

			if (diff > size)
				diff = size;

			if (!ReadRetry(fd, filename, offset, data, diff))
				return -1;

			_offset += diff;
			_size -= diff;

			data = (char *)data + diff;
			total += diff;
			size -= diff;
		}

		_totalRead += total;
		return total;
	}

	size_t TotalRead() const {
		return _totalRead;
	}
private:
	size_t DataLeftInCluster() {
		size_t max = super.SectorsPerCluster() * super.BytesPerSector();

		size_t avail = _offset < max ? (max - _offset) : 0;

		return avail < _size ? avail : _size;
	}

	void NextCluster() {
		_offset = 0;

		if (_size == 0) {
			_cluster = 0;
			return;
		}

		auto ent = FatEntryRead(_cluster);

		if (ent < 2 || ent >= 0x0FFFFFF8) {
			_size = 0;
			_cluster = 0;
		} else {
			_cluster = ent;
		}
	}

	uint32_t _cluster;
	uint32_t _offset;
	uint32_t _size;
	size_t _totalRead;
};

class FileWriter {
public:
	FileWriter() {
		_cluster = FindFreeCluster();
		if (_cluster >= 0x0FFFFFF8)
			throw std::runtime_error("No free cluster available");

		_offset = 0;
		_firstCluster = _cluster;
		_totalSize = 0;

		FatEntrySet(_cluster, 0x0FFFFFF8);
	}

	FileWriter(uint32_t idx, uint32_t size) : _cluster(idx), _offset(size),
						  _firstCluster(idx), _totalSize(size) {
		auto clusterSize = super.SectorsPerCluster() * super.BytesPerSector();

		while (size > clusterSize) {
			auto ent = FatEntryRead(_cluster);

			if (ent < 2 || ent >= 0x0FFFFFF8)
				throw std::runtime_error("Error following cluster chain");

			_cluster = ent;
			size -= clusterSize;
		}

		auto ent = FatEntryRead(_cluster);
		if (ent < 0x0FFFFFF8)
			throw std::runtime_error("Cluster chain not properly termminated");

		_offset = size;
	}

	bool Append(const void *data, size_t size) {
		while (size > 0) {
			auto diff = SpaceAvailable();
			if (diff > size)
				diff = size;

			if (diff == 0) {
				auto next = FindFreeCluster();
				if (next >= 0x0FFFFFF8) {
					std::cerr << "No more free clusters!" << std::endl;
					return false;
				}

				FatEntrySet(_cluster, next);
				FatEntrySet(next, 0x0FFFFFF8);

				_cluster = next;
				_offset = 0;
				continue;
			}

			uint64_t offset = ClusterIndex2Sector(_cluster) * super.BytesPerSector() + _offset;

			if (!WriteRetry(fd, filename, offset, data, diff))
				return false;

			_offset += diff;
			_totalSize += diff;
			data = (const char *)data + diff;
			size -= diff;
		}

		return true;
	}

	auto FirstCluster() const {
		return _firstCluster;
	}
private:
	size_t SpaceAvailable() const {
		return super.SectorsPerCluster() * super.BytesPerSector() - _offset;
	}

	uint32_t _cluster;
	uint32_t _offset;

	uint32_t _firstCluster;
	uint32_t _totalSize;
};

static bool ScanDirectory(FileReader &rd, std::vector<DirEntry> &out,
			  size_t &totalRead)
{
	size_t overread = 0;

	out.clear();

	for (;;) {
		std::vector<FatDirentLong> lEntList;
		FatDirent sEnt;

		auto ret = rd.Read(&sEnt, sizeof(sEnt));
		if (ret < 0)
			return false;
		if (ret < sizeof(sEnt)) {
			overread = ret;
			break;
		}

		if (sEnt.IsDummiedOut())
			continue;

		if (sEnt.IsLastInList()) {
			overread = ret;
			break;
		}

		if (sEnt.EntryFlags().IsSet(FatDirent::Flags::LongFileName)) {
			// re-encode the entry, get count and checksum
			FatDirentLong lEnt;
			memcpy(&lEnt, &sEnt, sizeof(sEnt));

			auto count = lEnt.SequenceNumber();
			auto chksum = lEnt.ShortNameChecksum();

			if (!lEnt.IsLast() || count < 1) {
				std::cerr << "Long name list needs to begin with last entry!" << std::endl;
				return false;
			}

			lEntList.push_back(lEnt);

			// read all subsequence entries belonging to this one
			for (decltype(count) i = 1; i < count; ++i) {
				ret = rd.Read(&lEnt, sizeof(lEnt));
				if (ret < 0)
					return false;

				if (ret < sizeof(lEnt)) {
					std::cerr << "Not enouth space for long name list!" << std::endl;
					return false;
				}

				if (!lEnt.EntryFlags().IsSet(FatDirent::Flags::LongFileName) ||
				    lEnt.IsLast()) {
					std::cerr << "Broken long name entry!" << std::endl;
					return false;
				}

				lEntList.push_back(lEnt);
			}

			// Sanitize the list
			std::reverse(lEntList.begin(), lEntList.end());

			for (size_t i = 0; i < lEntList.size(); ++i) {
				if (lEntList.at(i).SequenceNumber() != (i + 1) ||
				    lEntList.at(i).ShortNameChecksum() != chksum) {
					std::cerr << "Broken long filename sequence!" << std::endl;
					return false;
				}
			}

			// Get the corresponding short name entry
			ret = rd.Read(&sEnt, sizeof(sEnt));
			if (ret < 0)
				return false;
			if (ret < sizeof(sEnt)) {
				std::cerr << "Long name entry not followed by short name!" << std::endl;
				return false;
			}

			if (sEnt.EntryFlags().IsSet(FatDirent::Flags::LongFileName)) {
				std::cerr << "Short name entry is broken!" << std::endl;
				return false;
			}

			if (sEnt.Checksum() != chksum) {
				std::cerr << "Short name checksum does not match!" << std::endl;
				return false;
			}
		}

		char shortName[12];
		if (!sEnt.NameToString(shortName)) {
			std::cerr << "File with empty short name found!" << std::endl;
			return false;
		}

		DirEntry ent;
		ent.size = sEnt.Size();
		ent.firstCluster = sEnt.ClusterIndex();
		ent.shortName = shortName;
		ent.isDirectory = sEnt.EntryFlags().IsSet(FatDirent::Flags::Directory);

		if (lEntList.empty())
			ent.longName = shortName;

		for (const auto &it : lEntList) {
			char buffer[14];

			it.NameToString(buffer);

			if (strlen(buffer) == 0) {
				std::cerr << "File with empty long name found!" << std::endl;
				return false;
			}

			ent.longName += buffer;
		}

		out.push_back(ent);
	}

	totalRead = rd.TotalRead() - overread;
	return true;
}

static bool FindFile(std::string args, DirEntry &out)
{
	DirEntry ent;

	ent.size = 0;
	ent.firstCluster = super.RootDirIndex();
	ent.isDirectory = true;

	auto path = SplitPath(args);

	while (!path.empty()) {
		if (!ent.isDirectory) {
			std::cerr << args << ": component is not a directory" << std::endl;
			return false;
		}

		FileReader rd(ent.firstCluster, 0xFFFFFFFF);
		std::vector<DirEntry> list;
		size_t total;

		if (!ScanDirectory(rd, list, total))
			return false;

		auto name = path.front();
		path.pop_front();

		bool found = false;

		for (const auto &it : list) {
			if (it.longName == name || it.shortName == name) {
				found = true;
				ent = it;
				break;
			}
		}

		if (!found) {
			std::cerr << name << ": no such file or directory" << std::endl;
			return false;
		}
	}

	out = ent;
	return true;
}

static bool MatchCommand(std::string &str, const char *cmd)
{
	auto len = strlen(cmd);

	if (str.size() < len || strncmp(str.c_str(), cmd, len) != 0)
		return false;

	if (str.size() > len && !isspace(str.at(len)))
		return false;

	while (len < str.size() && isspace(str.at(len)))
		++len;

	str.erase(0, len);
	return true;
}

static void ListDirectory(std::string args)
{
	DirEntry ent;

	if (!FindFile(args, ent))
		return;

	if (!ent.isDirectory) {
		std::cerr << args << ": not a directory" << std::endl;
		return;
	}

	FileReader rd(ent.firstCluster, 0xFFFFFFFF);
	std::vector<DirEntry> list;
	size_t total;

	if (!ScanDirectory(rd, list, total))
		return;

	for (const auto &it : list) {
		std::cout << it.longName << " (" << it.shortName << "), "
			  << it.size;

		if (it.isDirectory)
			std::cout << ", directory";

		std::cout << std::endl;
	}
}

static void DumpFile(std::string args)
{
	DirEntry ent;

	if (!FindFile(args, ent))
		return;

	if (ent.isDirectory) {
		std::cerr << args << ": is a directory" << std::endl;
		return;
	}

	FileReader rd(ent.firstCluster, ent.size);

	for (;;) {
		char buffer[512];

		auto ret = rd.Read(buffer, sizeof(buffer));
		if (ret <= 0)
			return;

		std::cout.write(buffer, ret);
	}
}

static bool InitializeEmptyDirectory(uint32_t parentIdx, uint32_t &indexOut)
{
	FlagField<FatDirent::Flags, uint8_t> flags;
	flags.Set(FatDirent::Flags::Directory);

	FileWriter data;

	FatDirent dot;
	dot.SetName(".");
	dot.SetEntryFlags(flags);
	dot.SetClusterIndex(data.FirstCluster());

	FatDirent dotdot;
	dotdot.SetName("..");
	dotdot.SetEntryFlags(flags);
	dotdot.SetClusterIndex(parentIdx);

	FatDirent zero;
	memset(&zero, 0, sizeof(zero));

	indexOut = data.FirstCluster();

	return data.Append(&dot, sizeof(dot)) &&
		data.Append(&dotdot, sizeof(dotdot)) &&
		data.Append(&zero, sizeof(zero));
}

static void CreateDirectory(std::string args)
{
	auto path = SplitPath(args);
	if (path.empty())
		return;

	DirEntry parent;

	if (path.size() > 1) {
		std::ostringstream ss;
		size_t idx = 0;

		for (const auto &it : path) {
			if (idx < path.size() - 1) {
				if (idx > 0)
					ss << '/';
				ss << it;
				++idx;
			}
		}

		auto prefix = ss.str();

		if (!FindFile(prefix, parent))
			return;

		if (!parent.isDirectory) {
			std::cerr << prefix << ": not a directory" << std::endl;
			return;
		}
	} else {
		parent.size = 0;
		parent.firstCluster = super.RootDirIndex();
		parent.isDirectory = true;
	}

	auto name = path.back();
	if (!IsShortName(name)) {
		std::cerr << name << ": is not a short name (sorry)" << std::endl;
		return;
	}

	if (name.find('.') != std::string::npos) {
		std::cerr << name << ": directory name must not contain `.`" << std::endl;
		return;
	}

	FileReader rd(parent.firstCluster, 0xFFFFFFFF);
	std::vector<DirEntry> entries;
	size_t actualSize;

	if (!ScanDirectory(rd, entries, actualSize))
		return;

	for (const auto &it : entries) {
		if (it.shortName == name || it.longName == name) {
			std::cerr << args << ": already exists" << std::endl;
			return;
		}
	}

	uint32_t index;

	if (!InitializeEmptyDirectory(parent.firstCluster, index))
		return;

	FileWriter parentDir(parent.firstCluster, actualSize);

	FlagField<FatDirent::Flags, uint8_t> flags;
	flags.Set(FatDirent::Flags::Directory);

	FatDirent ent;
	ent.SetName(name.c_str());
	ent.SetEntryFlags(flags);
	ent.SetClusterIndex(index);

	parentDir.Append(&ent, sizeof(ent));

	memset(&ent, 0, sizeof(ent));
	parentDir.Append(&ent, sizeof(ent));
}

int main(int argc, char **argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
		std::cerr << "Usage: fatedit <image>" << std::endl << std::endl
			  << "edit commands are read from STDIN" << std::endl;
		return EXIT_FAILURE;
	}

	filename = argv[1];

	fd = open(filename, O_RDWR);
	if (fd < 0) {
		perror(filename);
		return EXIT_FAILURE;
	}

	// read the super block
	if (!ReadRetry(fd, filename, 0, &super, sizeof(super)))
		goto fail;

	if (super.BytesPerSector() != 512) {
		std::cerr << "Bytes per sector is " << super.BytesPerSector()
			  << ", expected 512!" << std::endl;
		goto fail;
	}

	// read the FS info sector
	if (super.FsInfoIndex() >= super.ReservedSectors()) {
		std::cerr << "FS Info index is " << super.ReservedSectors()
			  << ", which is past reserved sector count ("
			  << super.ReservedSectors() << ")" << std::endl;
		goto fail;
	}

	if (!ReadRetry(fd, filename, super.FsInfoIndex() * 512, &fsinfo, sizeof(fsinfo))) {
		std::cerr << "Error reading FS info sector" << std::endl;
		goto fail;
	}

	if (!fsinfo.IsValid()) {
		std::cerr << "FS info sector is broken!" << std::endl;
		goto fail;
	}

	// read the entire FAT into memory
	_fatRaw.resize(super.SectorsPerFat() * 512);

	if (!ReadRetry(fd, filename, super.ReservedSectors() * 512,
		       _fatRaw.data(), _fatRaw.size())) {
		std::cerr << "Error reading raw FAT data" << std::endl;
		goto fail;
	}

	// do the thing
	for (;;) {
		if (isatty(STDOUT_FILENO)) {
			std::cout << "C:\\> ";
			std::cout.flush();
		}

		std::string line;
		if (!std::getline(std::cin, line))
			break;
		trim(line);

		if (line.empty() || MatchCommand(line, "rem"))
			continue;

		if (MatchCommand(line, "dir")) {
			ListDirectory(line);
			continue;
		}

		if (MatchCommand(line, "type")) {
			DumpFile(line);
			continue;
		}

		if (MatchCommand(line, "mkdir")) {
			CreateDirectory(line);
			continue;
		}

		std::cerr << "Unknown command `" << line << "`" << std::endl;
	}

	// Update all the FATs
	for (size_t i = 0; i < super.NumFats(); ++i) {
		uint64_t offset = super.ReservedSectors() * 512;

		offset += i * _fatRaw.size();

		if (!WriteRetry(fd, filename, offset,
				_fatRaw.data(), _fatRaw.size())) {
			std::cerr << "Error updating FAT #" << i << std::endl;
			goto fail;
		}
	}

	// Write back super block
	if (!WriteRetry(fd, filename, 0, &super, sizeof(super))) {
		std::cerr << "Error updating super block" << std::endl;
		goto fail;
	}

	if (super.BackupIndex() > 0 &&
	    super.BackupIndex() < super.ReservedSectors()) {
		if (!WriteRetry(fd, filename, super.BackupIndex() * 512,
				&super, sizeof(super))) {
			std::cerr << "Error updating super block backup" << std::endl;
			goto fail;
		}
	}

	// write back FS info sector
	if (!WriteRetry(fd, filename, super.FsInfoIndex() * 512,
			&fsinfo, sizeof(fsinfo))) {
		std::cerr << "Error updating FS info sector" << std::endl;
		goto fail;
	}

	// cleanup
	close(fd);
	return EXIT_SUCCESS;
fail:
	close(fd);
	return EXIT_FAILURE;
}
