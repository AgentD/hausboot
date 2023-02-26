#ifndef BYTE_BLOB_H
#define BYTE_BLOB_H

#include <cstdint>
#include <cstddef>

template<size_t COUNT, uint8_t INIT = 0>
class ByteBlob {
public:
	ByteBlob() {
		for (auto &x : _raw)
			x = INIT;
	}
private:
	uint8_t _raw[COUNT];
};

#endif /* BYTE_BLOB_H */
