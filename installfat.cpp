#include "FatSuper.h"

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

static void InstallVBR(void)
{
	auto *super = static_cast<FatSuper *>(disk);

	super->SetBootCode(vbr.data(), vbr.size());

	// :-)
	super->SetOEMName("Goliath");
}

int main(int argc, char **argv)
{
	// process arguments
	const char *vbrFile = nullptr;
	const char *outFile = nullptr;

	for (int i = 1; i < argc; ++i) {
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

	// cleanup
	munmap(disk, diskSize);
	close(fd);
	return EXIT_SUCCESS;
}
