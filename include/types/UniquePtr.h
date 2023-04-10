/* SPDX-License-Identifier: ISC */
/*
 * UniquePtr.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef UNIQUE_PTR_H
#define UNIQUE_PTR_H

#include <cstddef>
#include <utility>
#include <type_traits>

template<typename T>
class UniquePtr {
public:
	constexpr UniquePtr() : _raw(nullptr) {}
	constexpr UniquePtr(std::nullptr_t null) : _raw(null) {}
	explicit UniquePtr(T *ptr) : _raw(ptr) {}

	UniquePtr(UniquePtr<T> &&o) : _raw(o._raw) {
		o._raw = nullptr;
	}

	template<typename U>
	UniquePtr(UniquePtr<U> &&o) : _raw(o.Release()) {
		static_assert(std::is_base_of<T, U>::value);
	}

	UniquePtr<T> &operator= (UniquePtr<T> &&o) {
		T *temp = _raw;
		_raw = o._raw;
		o._raw = temp;
		return *this;
	}

	~UniquePtr() {
		delete _raw;
	}

	UniquePtr(const UniquePtr<T> &o) = delete;
	UniquePtr<T> &operator= (const UniquePtr<T> &o) = delete;

	UniquePtr<T> &operator= (std::nullptr_t null) {
		delete _raw;
		_raw = null;
		return *this;
	}

	bool operator== (std::nullptr_t null) const { return _raw == null; }
	bool operator!= (std::nullptr_t null) const { return _raw == null; }

	T *operator->() const { return _raw; }
	T &operator*() const { return *_raw; }

	T *Release() {
		auto *temp = _raw;
		_raw = nullptr;
		return temp;
	}
private:
	T *_raw;
};

template<typename T, typename... Args>
UniquePtr<T> MakeUnique(Args&&... args)
{
	T *obj = new T(std::forward<Args>(args)...);
	return UniquePtr<T>(obj);
}

#endif /* UNIQUE_PTR_H */
