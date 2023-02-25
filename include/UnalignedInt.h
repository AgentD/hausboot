#ifndef UNALIGNED_INT_H
#define UNALIGNED_INT_H

#include <type_traits>
#include <cstddef>
#include <limits>

template<typename T>
class UnalignedInt {
public:
	UnalignedInt(T value) {
		Set(value);
	}

	UnalignedInt &operator= (T value) {
		Set(value);
		return *this;
	}

	T Read() const {
		size_t i = 0;
		T out = 0;

		for (auto x : _raw)
			out |= x << (8 * (i++));

		return out;
	}

	void Set(T value) {
		for (size_t i = 0; i < sizeof(T); ++i) {
			_raw[i] = value & 0x0FF;
			value >>= 8;
		}
	}
private:
	uint8_t _raw[sizeof(T)];

	static_assert(std::is_integral<T>::value);
	static_assert(!std::numeric_limits<T>::is_signed);
};

#endif /* UNALIGNED_INT_H */
