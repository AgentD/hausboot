/* SPDX-License-Identifier: ISC */
/*
 * MultiBootHeader.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef MULTIBOOT_HEADER_H
#define MULTIBOOT_HEADER_H

#include <cstdint>

#include "FlagField.h"

class MultiBootHeader {
public:
	enum class KernelFlags : uint32_t {
		Force4KAlign = 0x00000001,
		WantMemoryMap = 0x00000002,
		WantVidmode = 0x00000004,
		HaveLayoutInfo = 0x00010000,
	};

	bool IsValid() const {
		return _magic == 0x1BADB002 &&
			_checksum == (~(_magic + _flags.RawValue()) + 1);
	}

	auto Flags() const {
		return _flags;
	}

	bool ExtractMemLayout(uint32_t hdrFileOffset, uint32_t filesize,
			      uint32_t &fileStart, uint32_t &memStart,
			      uint32_t &count) const {
		// end address must be after start address
		if (_loadEndAddr <= _loadAddr)
			return false;

		fileStart = 0;
		memStart = _loadAddr;
		count = _loadEndAddr - _loadAddr;

		// BSS must be after the text & data blob
		if (_bssEndAddr < _loadAddr)
			return false;

		// header & entry point must be inside the text & data blob
		if (_hdrAddr < _loadAddr || _hdrAddr >= _loadEndAddr)
			return false;

		if (_entryAddr < _loadAddr || _entryAddr >= _loadEndAddr)
			return false;

		// distance of header from load address cannot be more than
		// there is in the file
		auto hdrMemOffset = _hdrAddr - _loadAddr;

		if (hdrMemOffset > hdrFileOffset)
			return false;

		// but there can be extra data in front of the text segment
		if (hdrFileOffset > hdrMemOffset)
			fileStart = hdrFileOffset - hdrMemOffset;

		// must fit in file
		return count <= (filesize - fileStart);
	}

	void *EntryPoint() const {
		return (void *)_entryAddr;
	}

	size_t BSSSize() const {
		return _bssEndAddr > _loadEndAddr ?
			(_bssEndAddr - _loadEndAddr) : 0;
	}
private:
	uint32_t _magic;
	FlagField<KernelFlags, uint32_t> _flags;
	uint32_t _checksum;
	uint32_t _hdrAddr;
	uint32_t _loadAddr;
	uint32_t _loadEndAddr;
	uint32_t _bssEndAddr;
	uint32_t _entryAddr;
};

static_assert(sizeof(MultiBootHeader) == 32);

#endif /* MULTIBOOT_HEADER_H */
