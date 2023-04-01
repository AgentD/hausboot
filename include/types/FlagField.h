/* SPDX-License-Identifier: ISC */
/*
 * FlagField.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef FLAG_FIELD_H
#define FLAG_FIELD_H

#include <initializer_list>
#include <type_traits>
#include <limits>

template<typename T, typename U = unsigned int>
class FlagField {
private:
	class FlagRef {
	public:
		FlagRef(FlagField &ref, T flag) : _ref(ref), _flag(flag) {
		}

		FlagRef() = delete;
		FlagRef(const FlagRef &) = delete;
		FlagRef(FlagRef &&) = delete;

		constexpr FlagRef &operator= (FlagRef &&o) {
			_ref.Set(_flag, (bool)o);
			return *this;
		}

		constexpr FlagRef &operator= (const FlagRef &o) {
			_ref.Set(_flag, (bool)o);
			return *this;
		}

		constexpr bool operator= (bool set) {
			_ref.Set(_flag, set);
			return set;
		}

		constexpr operator bool() const {
			return _ref.IsSet(_flag);
		}
	private:
		FlagField &_ref;
		T _flag;
	};

	U _value;
	static_assert(std::is_enum<T>::value);
	static_assert(std::is_integral<U>::value);
	static_assert(!std::numeric_limits<U>::is_signed);
	static_assert(sizeof(U) >= sizeof(T));
public:
	FlagField() : _value(0) {
	}

	explicit constexpr FlagField(U raw) : _value(raw) {
	}

	constexpr FlagField(const std::initializer_list<T> &init) : _value(0) {
		for (auto &flag : init) {
			_value |= static_cast<U>(flag);
		}
	}

	constexpr void Set(T flag) {
		_value |= static_cast<U>(flag);
	}

	constexpr void Clear(T flag) {
		_value &= ~static_cast<U>(flag);
	}

	constexpr void Clear() {
		_value = 0;
	}

	constexpr void Set(T flag, bool setval) {
		if (setval) {
			Set(flag);
		} else {
			Clear(flag);
		}
	}

	constexpr bool IsSet(T flag) const {
		return (_value & static_cast<U>(flag)) == static_cast<U>(flag);
	}

	constexpr const FlagRef operator[] (T flag) const && {
		return FlagRef(*this, flag);
	}

	constexpr FlagRef operator[] (T flag) & {
		return FlagRef(*this, flag);
	}

	constexpr U RawValue() const {
		return _value;
	}
};

#endif /* FLAG_FIELD_H */
