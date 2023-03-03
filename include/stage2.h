#ifndef STAGE2_H
#define STAGE2_H

#include <cstdint>

constexpr uint32_t Stage2Magic = 0xD0D0CACA;
constexpr uint16_t Stage2Location = 0x1000;

struct Stage2Info {
public:
	void SetSectorCount(uint32_t size) {
		_sectorCount = size / 512;

		if (size % 512)
			_sectorCount += 1;
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
private:
	const uint32_t _magic = Stage2Magic;
	uint32_t _checksum = 0;
	uint16_t _sectorCount = 0;
	const uint16_t _pad0 = 0;
};

static_assert(sizeof(Stage2Info) == 12);

#endif /* STAGE2_H */
