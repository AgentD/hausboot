#ifndef FIXED_LENGTH_STRING_H
#define FIXED_LENGTH_STRING_H

#include <cstdint>
#include <cstddef>

template<size_t COUNT, char FILL>
class FixedLengthString {
public:
	FixedLengthString(const char *value) {
		Set(value);
	}

	void Set(const char *value) {
		size_t i = 0;

		for (; i < COUNT && value[i] != '\0'; ++i)
			_raw[i] = value[i];

		for (; i < COUNT; ++i)
			_raw[i] = FILL;
	}
private:
	uint8_t _raw[COUNT];
};


#endif /* FIXED_LENGTH_STRING_H */
