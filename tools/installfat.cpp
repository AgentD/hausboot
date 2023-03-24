/* SPDX-License-Identifier: ISC */
/*
 * installfat.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "fs/FatSuper.h"
#include "stage2.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <vector>

#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

static bool ReadAll(const char *filename, std::vector<uint8_t> &out,
		    size_t max)
{
	int fd = open(filename, O_RDONLY);

	if (fd < 0) {
		perror(filename);
		return false;
	}

	out.clear();

	for (;;) {
		auto used = out.size();
		out.resize(out.size() + 1024);

		auto ret = read(fd, out.data() + used,
				out.size() - used);

		if (ret < 0) {
			if (errno == EINTR)
				continue;
			perror(filename);
			out.clear();
			close(fd);
			return false;
		}

		if (ret == 0) {
			out.resize(used);
			break;
		}

		out.resize(used + ret);

		if (out.size() > max) {
			std::cerr << filename << " is too big (max: "
				  << max << ")" << std::endl;
			out.clear();
			close(fd);
			return false;
		}
	}

	close(fd);
	return true;
}

static void *disk = nullptr;
static uint64_t diskSize = 0;
static std::vector<uint8_t> vbr;
static std::vector<uint8_t> stage2;

static void InstallVBR(void)
{
	auto *super = static_cast<FatSuper *>(disk);

	super->SetBootCode(vbr.data(), vbr.size());

	// Disable backup copy of the boot sector, and make sure
	// the FS Info is right after the boot sector
	super->SetBackupIndex(0);

	auto idx = super->FsInfoIndex();
	if (idx != 1) {
		std::cout << "Relocating FS Info block from "
			  << idx << std::endl;

		memmove((uint8_t *)disk + 512,
			(uint8_t *)disk + 512 * idx,
			512);

		super->SetFsInfoIndex(1);
	}

	// :-)
	super->SetOEMName("Goliath");
}

static uint64_t Stage2SpaceAvailable(void)
{
	auto *super = static_cast<FatSuper *>(disk);

	uint64_t secs = super->ReservedSectors();

	if (secs <= 2)
		return 0;

	uint64_t space = (secs - 2) * super->BytesPerSector();
	if (space >= diskSize)
		return 0;

	return space;
}

int main(int argc, char **argv)
{
	// process arguments
	const char *vbrFile = nullptr;
	const char *stage2File = nullptr;
	const char *outFile = nullptr;

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--stage2")) {
			if ((i + 1) >= argc) {
				std::cerr << "Missing argument for `" << argv[i]
					  << "`" << std::endl;
				return EXIT_FAILURE;
			}

			stage2File = argv[++i];
			continue;
		}

		if (!strcmp(argv[i], "-v")) {
			if (argv[i][2] != '\0') {
				vbrFile = argv[i] + 2;
				continue;
			}

			if ((i + 1) >= argc) {
				std::cerr << "Missing argument for `" << argv[i]
					  << "`" << std::endl;
				return EXIT_FAILURE;
			}

			vbrFile = argv[++i];
			continue;
		}

		if (!strcmp(argv[i], "-o")) {
			if (argv[i][2] != '\0') {
				outFile = argv[i] + 2;
				continue;
			}

			if ((i + 1) >= argc) {
				std::cerr << "Missing argument for `" << argv[i]
					  << "`" << std::endl;
				return EXIT_FAILURE;
			}

			outFile = argv[++i];
			continue;
		}
	}

	if (outFile == nullptr) {
		std::cerr << "no output file specified" << std::endl;
		return EXIT_FAILURE;
	}

	if (vbrFile == nullptr) {
		std::cerr << "no VBR file specified" << std::endl;
		return EXIT_FAILURE;
	}

	// get the VBR
	if (!ReadAll(vbrFile, vbr, 420))
		return EXIT_FAILURE;

	// map the disk
	int fd = open(outFile, O_RDWR);
	if (fd < 0) {
		perror(outFile);
		return EXIT_FAILURE;
	}

	struct stat sb;
	if (fstat(fd, &sb)) {
		perror(outFile);
		close(fd);
		return EXIT_FAILURE;
	}

	diskSize = sb.st_size;

	disk = mmap(NULL, diskSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (disk == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return EXIT_FAILURE;
	}

	// install the volume boot record
	InstallVBR();

	// Install second stage boot loader
	if (stage2File != nullptr) {
		auto max = Stage2SpaceAvailable();

		if (!ReadAll(stage2File, stage2, max)) {
			munmap(disk, diskSize);
			close(fd);
			return EXIT_FAILURE;
		}

		memcpy((uint8_t *)disk + 2 * 512,
		       stage2.data(), stage2.size());

		memset((uint8_t *)disk + 2 * 512 + stage2.size(),
		       0, max - stage2.size());

		auto *hdr = new ((uint8_t *)disk + 2 * 512) Stage2Info();

		hdr->SetSectorCount(stage2.size());
		hdr->UpdateChecksum();

		if (!hdr->Verify(hdr->SectorCount())) {
			std::cerr << "Stage 2 header verification failed"
				  << std::endl;
			munmap(disk, diskSize);
			close(fd);
			return EXIT_FAILURE;
		}
	}

	// cleanup
	munmap(disk, diskSize);
	close(fd);
	return EXIT_SUCCESS;
}
