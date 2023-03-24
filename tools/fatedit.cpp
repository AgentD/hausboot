/* SPDX-License-Identifier: ISC */
/*
 * fatedit.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "fs/FatSuper.h"
#include "fs/FatFsInfo.h"
#include "fs/FatDirent.h"
#include "fs/FatDirentLong.h"
#include "host/File.h"
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
static File file;

class FatReader {
public:
	uint32_t NextClusterInFile(uint32_t N) const {
		auto idx = N * 4;

		// XXX: std::vector<T>::at() does bounds checking, this
		// should throw an exception if N is bonkers.
		uint32_t out = _fatRaw.at(idx++);
		out |= static_cast<uint32_t>(_fatRaw.at(idx++)) << 8;
		out |= static_cast<uint32_t>(_fatRaw.at(idx++)) << 16;
		out |= static_cast<uint32_t>(_fatRaw.at(idx++)) << 24;

		return out & 0x0FFFFFFF;
	}

	void LoadFromImage() {
		if (super.FsInfoIndex() >= super.ReservedSectors()) {
			std::ostringstream ss;
			ss << "FS Info index is " << super.ReservedSectors()
			   << ", which is past reserved sector count ("
			   << super.ReservedSectors() << ")";
			throw std::runtime_error(ss.str());
		}

		file.ReadAt(super.FsInfoIndex() * super.BytesPerSector(),
			    &_fsinfo, sizeof(_fsinfo));

		if (!_fsinfo.IsValid())
			throw std::runtime_error("FS info sector is broken!");

		auto offset = super.ReservedSectors() * super.BytesPerSector();
		auto size = super.SectorsPerFat() * super.BytesPerSector();

		_fatRaw.resize(size);

		file.ReadAt(offset, _fatRaw.data(), _fatRaw.size());
	}

	void WriteToImage() {
		auto offset = super.ReservedSectors() * super.BytesPerSector();

		for (size_t i = 0; i < super.NumFats(); ++i) {
			file.WriteAt(offset, _fatRaw.data(), _fatRaw.size());

			offset += _fatRaw.size();
		}

		_fsinfo.SetNextFreeCluster(FindFreeCluster());
		_fsinfo.SetNumFreeCluster(NumFreeClusters());

		file.WriteAt(super.FsInfoIndex() * super.BytesPerSector(),
			     &_fsinfo, sizeof(_fsinfo));
	}

	uint32_t AllocateCluster() {
		auto out = FindFreeCluster();
		if (out >= 0x0FFFFFF8)
			throw std::runtime_error("No free cluster available");
		Set(out, 0x0FFFFFF8);
		return out;
	}

	uint32_t AllocateCluster(uint32_t lastInFile) {
		auto out = AllocateCluster();
		Set(lastInFile, out);
		return out;
	}

	uint64_t ClusterFileOffset(uint32_t index) const {
		return super.ClusterIndex2Sector(index) *
			super.BytesPerSector();
	}

	size_t BytesPerCluster() const {
		return super.SectorsPerCluster() * super.BytesPerSector();
	}
private:
	void Set(size_t N, uint32_t value) {
		auto idx = N * 4;

		if (_fatRaw.size() < 4 || idx >= (_fatRaw.size() - 3))
			return;

		_fatRaw[idx++] = value & 0xFF;
		_fatRaw[idx++] = (value >> 8) & 0xFF;
		_fatRaw[idx++] = (value >> 16) & 0xFF;
		_fatRaw[idx++] = (value >> 24) & 0x0F;
	}

	uint32_t NumFreeClusters() const {
		uint32_t count = 0;

		for (size_t i = 0; i < (_fatRaw.size() / 4); ++i) {
			if (NextClusterInFile(i) == 0)
				++count;
		}

		return count;
	}

	uint32_t FindFreeCluster() {
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
			if (NextClusterInFile(i) == 0)
				return i;
		}

		return 0x0FFFFFFF8;
	}

	std::vector<uint8_t> _fatRaw;
	FatFsInfo _fsinfo;
};

static FatReader fat;

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

			uint64_t offset = fat.ClusterFileOffset(_cluster) + _offset;

			if (diff > size)
				diff = size;

			file.ReadAt(offset, data, diff);

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
		size_t max = fat.BytesPerCluster();

		size_t avail = _offset < max ? (max - _offset) : 0;

		return avail < _size ? avail : _size;
	}

	void NextCluster() {
		_offset = 0;

		if (_size == 0) {
			_cluster = 0;
			return;
		}

		auto ent = fat.NextClusterInFile(_cluster);

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
		_cluster = fat.AllocateCluster();
		_firstCluster = _cluster;
		_offset = 0;
		_totalSize = 0;
	}

	FileWriter(uint32_t idx, uint32_t size) : _cluster(idx), _offset(size),
						  _firstCluster(idx), _totalSize(size) {
		auto clusterSize = fat.BytesPerCluster();

		while (size > clusterSize) {
			auto ent = fat.NextClusterInFile(_cluster);

			if (ent < 2 || ent >= 0x0FFFFFF8)
				throw std::runtime_error("Error following cluster chain");

			_cluster = ent;
			size -= clusterSize;
		}

		auto ent = fat.NextClusterInFile(_cluster);
		if (ent < 0x0FFFFFF8)
			throw std::runtime_error("Cluster chain not properly termminated");

		_offset = size;
	}

	void Append(const void *data, size_t size) {
		while (size > 0) {
			auto diff = SpaceAvailable();
			if (diff > size)
				diff = size;

			if (diff == 0) {
				_cluster = fat.AllocateCluster(_cluster);
				_offset = 0;
				continue;
			}

			uint64_t offset = fat.ClusterFileOffset(_cluster) + _offset;

			file.WriteAt(offset, data, diff);

			_offset += diff;
			_totalSize += diff;
			data = (const char *)data + diff;
			size -= diff;
		}
	}

	auto FirstCluster() const {
		return _firstCluster;
	}

	auto BytesWritten() const {
		return _totalSize;
	}
private:
	size_t SpaceAvailable() const {
		return fat.BytesPerCluster() - _offset;
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

static void InitializeEmptyDirectory(uint32_t parentIdx, uint32_t &indexOut)
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

	data.Append(&dot, sizeof(dot));
	data.Append(&dotdot, sizeof(dotdot));
	data.Append(&zero, sizeof(zero));
}

static bool FindParent(const std::list<std::string> &path, DirEntry &parent)
{
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
			return false;

		if (!parent.isDirectory) {
			std::cerr << prefix << ": not a directory" << std::endl;
			return false;
		}
	} else {
		parent.size = 0;
		parent.firstCluster = super.RootDirIndex();
		parent.isDirectory = true;
	}

	return true;
}

static void AppendDirectoryEntry(uint32_t cluster, size_t size, bool isDir,
				 const std::string &name,
				 uint32_t fileCluster, size_t fileSize)
{
	FatDirent ent;

	auto pos = name.find('.');
	if (pos == std::string::npos) {
		ent.SetName(name.c_str());
	} else {
		auto pre = name.substr(0, pos);
		auto post = name.substr(pos + 1, std::string::npos);

		ent.SetName(pre.c_str());
		ent.SetExtension(post.c_str());
	}

	ent.SetClusterIndex(fileCluster);

	if (isDir) {
		FlagField<FatDirent::Flags, uint8_t> flags;
		flags.Set(FatDirent::Flags::Directory);
		ent.SetEntryFlags(flags);
	} else {
		ent.SetSize(fileSize);
	}

	FileWriter parentDir(cluster, size);
	parentDir.Append(&ent, sizeof(ent));

	memset(&ent, 0, sizeof(ent));
	parentDir.Append(&ent, sizeof(ent));
}

static bool CheckEntryNotInDir(uint32_t cluster, size_t &actualSize,
			       const std::string &name)
{
	FileReader rd(cluster, 0xFFFFFFFF);
	std::vector<DirEntry> entries;

	if (!ScanDirectory(rd, entries, actualSize))
		return false;

	for (const auto &it : entries) {
		if (it.shortName == name || it.longName == name) {
			std::cerr << name << ": already exists" << std::endl;
			return false;
		}
	}

	return true;
}

static void CreateDirectory(std::string args)
{
	auto path = SplitPath(args);
	if (path.empty())
		return;

	auto name = path.back();
	if (!IsShortName(name)) {
		std::cerr << name << ": is not a short name (sorry)" << std::endl;
		return;
	}

	if (name.find('.') != std::string::npos) {
		std::cerr << name << ": directory name must not contain `.`" << std::endl;
		return;
	}

	DirEntry parent;
	size_t actualSize;

	if (!FindParent(path, parent))
		return;

	if (!CheckEntryNotInDir(parent.firstCluster, actualSize, name))
		return;

	uint32_t index;

	InitializeEmptyDirectory(parent.firstCluster, index);
	AppendDirectoryEntry(parent.firstCluster, actualSize, true, name, index, 0);
}

static void PackDirectory(std::string args)
{
	// isolate the input filename
	size_t count = 0;
	while (count < args.size() && !isspace(args.at(count)))
		++count;

	if (count >= args.size())
		return;

	auto input = args.substr(0, count);

	while (isspace(args.at(count)))
		++count;

	args.erase(0, count);

	// get the target path
	auto path = SplitPath(args);
	if (path.empty())
		return;

	auto name = path.back();
	if (!IsShortName(name)) {
		std::cerr << name << ": is not a short name (sorry)" << std::endl;
		return;
	}

	// locate the parent directory
	DirEntry parent;
	size_t parentDirSize;

	if (!FindParent(path, parent))
		return;

	if (!CheckEntryNotInDir(parent.firstCluster, parentDirSize, name))
		return;

	// pack the input file
	File infile(input.c_str(), true);
	FileWriter wr;

	for (;;) {
		char buffer[512];

		auto diff = infile.Read(buffer, sizeof(buffer));
		if (diff == 0)
			break;

		wr.Append(buffer, diff);
	}

	std::cout << "Packed `" << input << "`" << std::endl;

	// update the directory
	AppendDirectoryEntry(parent.firstCluster, parentDirSize, false,
			     name, wr.FirstCluster(), wr.BytesWritten());
}

static struct {
	const char *name;
	void (*callback)(std::string);
} commands[] = {
	{ "dir", ListDirectory },
	{ "type", DumpFile },
	{ "mkdir", CreateDirectory },
	{ "pack", PackDirectory },
};

int main(int argc, char **argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
		std::cerr << "Usage: fatedit <image>" << std::endl << std::endl
			  << "edit commands are read from STDIN" << std::endl;
		return EXIT_FAILURE;
	}

	try {
		file = File(argv[1], false);
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	// read the super block
	file.ReadAt(0, &super, sizeof(super));

	// read the entire FAT into memory
	fat.LoadFromImage();

	// do the thing
	for (;;) {
		if (isatty(STDIN_FILENO)) {
			std::cout << "C:\\> ";
			std::cout.flush();
		}

		std::string line;
		if (!std::getline(std::cin, line))
			break;
		trim(line);

		if (line.empty() || MatchCommand(line, "rem"))
			continue;

		bool found = false;

		for (const auto &it : commands) {
			if (MatchCommand(line, it.name)) {
				it.callback(line);
				found = true;
				break;
			}
		}

		if (!found) {
			std::cerr << "Unknown command `" << line << "`" << std::endl;
		}
	}

	// Update all the FATs
	fat.WriteToImage();

	// Write back super block
	file.WriteAt(0, &super, sizeof(super));

	if (super.BackupIndex() > 0 &&
	    super.BackupIndex() < super.ReservedSectors()) {
		file.WriteAt(super.BackupIndex() * super.BytesPerSector(),
			     &super, sizeof(super));
	}

	return EXIT_SUCCESS;
}
