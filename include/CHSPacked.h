#ifndef CHS_PACKED_H
#define CHS_PACKED_H

#include <cstdint>

class CHSPacked {
public:
	uint8_t Head() const {
		return _lo;
	}

	void SetHead(uint8_t value) {
		_lo = value;
	}

	uint8_t Sector() const {
		return _mid & 0x3F;
	}

	void SetSector(uint8_t value) {
		_mid = value;
	}

	uint16_t Cylinder() const {
		return (static_cast<uint16_t>(_mid & 0xC0) << 8) | _hi;
	}

	void SetCylinder(uint16_t value) {
		_mid &= ~(0xC0);
		_mid |= (value >> 8) & 0xC0;

		_hi = value & 0xFF;
	}
private:
	uint8_t _lo = 0;
	uint8_t _mid = 0;
	uint8_t _hi = 0;
};

static_assert(sizeof(CHSPacked) == 3);

#endif /* CHS_PACKED_H */
