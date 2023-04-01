/* SPDX-License-Identifier: ISC */
/*
 * heap.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "Memory.h"
#include "types/FlagField.h"

#include <cstdint>

struct HeapEntry {
	enum class HeapFlags {
		Reserved = 0x01,
	};

	HeapEntry() = delete;
	~HeapEntry() = delete;
	HeapEntry(HeapEntry &&) = delete;
	HeapEntry(const HeapEntry &) = delete;
	HeapEntry &operator= (const HeapEntry &) = delete;
	HeapEntry &operator= (HeapEntry &&) = delete;

	bool IsFree() const {
		return !_flags.IsSet(HeapFlags::Reserved);
	}

	size_t Size() const {
		return ((char *)_next - (char *)this) - sizeof(*this);
	}

	void *Reserve(size_t count) {
		auto sz = Size();

		if (!IsFree() || (count > sz))
			return nullptr;

		if ((sz - count) >= (sizeof(*this) + 4)) {
			HeapEntry *ent = (HeapEntry *)((char *)DataPtr() + count);
			ent->Init(_next);
			_next = ent;
		}

		_flags.Set(HeapFlags::Reserved);
		return DataPtr();
	}

	void Release() {
		_flags.Clear(HeapFlags::Reserved);
	}

	void Init(HeapEntry *next) {
		_next = next;
		_flags.Clear();
	}

	HeapEntry *Next() {
		return _next;
	}

	bool TryConsolidate() {
		if (IsFree() && _next->IsFree()) {
			_next = _next->_next;
			return true;
		}
		return false;
	}
private:
	void *DataPtr() {
		return (char *)this + sizeof(*this);
	}

	HeapEntry *_next;
	FlagField<HeapFlags, uint32_t> _flags;
};

static_assert(sizeof(HeapEntry) == 8);

static HeapEntry *heap;
static HeapEntry *heapEnd;

void HeapInit(void *basePtr, size_t maxSize)
{
	if (maxSize % 4)
		maxSize -= maxSize % 4;

	heap = (HeapEntry *)basePtr;
	heapEnd = (HeapEntry *)((char *)heap + maxSize);

	heap->Init(heapEnd);
}

void *malloc(size_t count)
{
	if (count % 4)
		count += 4 - (count % 4);

	HeapEntry *found = nullptr;

	for (auto *it = heap; it != heapEnd; it = it->Next()) {
		if (it->IsFree() && it->Size() >= count) {
			if (found == nullptr || it->Size() < found->Size())
				found = it;
		}
	}

	return (found != nullptr) ? found->Reserve(count) : nullptr;
}

void free(void *ptr)
{
	if (ptr == nullptr)
		return;

	auto *ent = (HeapEntry *)((char *)ptr - sizeof(HeapEntry));

	ent->Release();

	for (;;) {
		bool merged = false;

		for (ent = heap; ent->Next() != heapEnd; ent = ent->Next()) {
			if (ent->TryConsolidate()) {
				merged = true;
				break;
			}
		}

		if (!merged)
			break;
	}
}
