/* SPDX-License-Identifier: ISC */
/*
 * installfat.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "Stage2Header.h"
#include "fs/FatFsInfo.h"
#include "fs/FatSuper.h"
#include "host/File.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>

static bool ReadAll(const char *filename, std::vector<uint8_t> &out,
		    size_t max)
{
	File fd(filename, true);

	out.clear();

	for (;;) {
		auto used = out.size();
		out.resize(out.size() + 1024);

		auto ret = fd.Read(out.data() + used, out.size() - used);
		if (ret == 0) {
			out.resize(used);
			break;
		}

		out.resize(used + ret);

		if (out.size() > max) {
			std::cerr << filename << " is too big (max: "
				  << max << ")" << std::endl;
			return false;
		}
	}

	return true;
}

int main(int argc, char **argv)
{
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

	if (stage2File == nullptr) {
		std::cerr << "no stag2 file specified" << std::endl;
		return EXIT_FAILURE;
	}

	try {
		std::vector<uint8_t> stage2;
		std::vector<uint8_t> vbr;
		File file(outFile, false);
		FatFsInfo fsinfo;
		FatSuper super;

		file.ReadAt(0, &super, sizeof(super));
		file.ReadAt(super.FsInfoIndex() * super.BytesPerSector(),
			    &fsinfo, sizeof(fsinfo));

		if (!ReadAll(vbrFile, vbr, 420))
			return EXIT_FAILURE;

		super.SetBackupIndex(0);
		super.SetFsInfoIndex(1);
		super.SetOEMName("Goliath");
		super.SetBootCode(vbr.data(), vbr.size());

		file.WriteAt(0, &super, sizeof(super));
		file.WriteAt(super.BytesPerSector(), &fsinfo, sizeof(fsinfo));

		auto max = (super.ReservedSectors() - 2) *
			super.BytesPerSector();

		if (!ReadAll(stage2File, stage2, max))
			return EXIT_FAILURE;

		auto *hdr = new (stage2.data()) Stage2Header();
		hdr->SetSectorCount(stage2.size());
		hdr->UpdateChecksum();

		if (!hdr->Verify(hdr->SectorCount())) {
			std::cerr << "Stage 2 header verification failed"
				  << std::endl;
			return EXIT_FAILURE;
		}

		file.WriteAt(2 * super.BytesPerSector(),
			     stage2.data(), stage2.size());
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

	return EXIT_SUCCESS;
}
