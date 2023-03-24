#ifndef FILE_H
#define FILE_H

#include <string>
#include <sstream>
#include <algorithm>
#include <system_error>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

class File {
public:
	File() : _fd(-1) {
	}

	File(const char *path, bool readonly) {
		_fd = open(path, readonly ? O_RDONLY : O_RDWR);
		if (_fd < 0)
			HandleError(-1, errno);
	}

	~File() {
		if (_fd >= 0)
			close(_fd);
	}

	File(const File &) = delete;
	File &operator= (const File &) = delete;

	File(File &&o) {
		_fd = o._fd;
		o._fd = -1;
	}

	File &operator= (File &&o) {
		std::swap(o._fd, _fd);
		return *this;
	}

	void ReadAt(uint64_t offset, void *data, size_t size) const {
		while (size > 0) {
			auto ret = pread(_fd, data, size, offset);
			if (ret < 0 && errno == EINTR)
				continue;

			if (ret <= 0)
				HandleError(ret, errno);

			data = (char *)data + ret;
			size -= ret;
			offset += ret;
		}
	}

	void WriteAt(uint64_t offset, const void *data, size_t size) {
		while (size > 0) {
			auto ret = pwrite(_fd, data, size, offset);
			if (ret < 0 && errno == EINTR)
				continue;

			if (ret <= 0)
				HandleError(ret, errno);

			data = (const char *)data + ret;
			size -= ret;
			offset += ret;
		}
	}

	ssize_t Read(void *data, size_t size) {
		ssize_t total = 0;

		while (size > 0) {
			auto ret = read(_fd, data, size);
			if (ret == 0)
				break;
			if (ret < 0) {
				if (errno == EINTR)
					continue;
				HandleError(ret, errno);
			}

			data = (char *)data + ret;
			size -= ret;
			total += ret;
		}

		return total;
	}
private:
	void HandleError(ssize_t retCode, int errorCode) const {
		if (retCode < 0) {
			throw std::system_error(errorCode,
						std::system_category(),
						_filename);
		} else {
			std::ostringstream str;

			str << _filename << ": unexpected end-of-file";

			throw std::runtime_error(str.str());
		}
	}

	int _fd;
	std::string _filename;
};

#endif /* FILE_H */
