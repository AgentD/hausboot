/* SPDX-License-Identifier: ISC */
/*
 * multiboot.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <cstdint>

#include "FlagField.h"
#include "UnalignedInt.h"
#include "BIOS/MemoryMap.h"

static_assert(sizeof(void *) == sizeof(uint32_t),
	      "Multiboot is usable for 32 bit x86 code only!");

class MultiBootMmap {
public:
	auto Type() const {
		return _wrapped.Type();
	}

	auto BaseAddress() const {
		return _wrapped.BaseAddress();
	}

	auto Size() const {
		return _wrapped.Size();
	}

	const auto *Next() const {
		auto next = (uint32_t)this + _size.Read() + sizeof(_size);

		return (const MultiBootMmap *)next;
	}
private:
	UnalignedInt<uint32_t> _size;
	MemoryMapEntry _wrapped;
};

static_assert(sizeof(MultiBootMmap) == 24);

class MultiBootInfo {
public:
	enum class InfoFlag : uint32_t {
		MemInfo = 0x00000001,
		BootDev = 0x00000002,
		CmdLine = 0x00000004,
		Mods = 0x00000008,
		AoutSyms = 0x00000010,
		ELFShdr = 0x00000020,
		MemMap = 0x00000040,
		DeviceInfo = 0x00000080,
		ConfigTable = 0x00000100,
		BootLoaderName = 0x00000200,
		APMTable = 0x00000400,
		VBEInfo = 0x00000800,
		FrameBufferInfo = 0x00001000,
	};

	const char *BootLoaderName() const {
		if (!_flags.IsSet(InfoFlag::BootLoaderName))
			return nullptr;

		return _bootLoaderName;
	}

	const char *CommandLine() const {
		if (!_flags.IsSet(InfoFlag::CmdLine))
			return nullptr;

		return _cmdline;
	}

	const MultiBootMmap *MemoryMapBegin() const {
		if (!_flags.IsSet(InfoFlag::MemMap))
			return nullptr;

		return _mmap.base;
	}

	const MultiBootMmap *MemoryMapEnd() const {
		if (!_flags.IsSet(InfoFlag::MemMap))
			return nullptr;

		auto *end = (const char *)_mmap.base + _mmap.length;

		return (const MultiBootMmap *)end;
	}
private:
	FlagField<InfoFlag, uint32_t> _flags;

	uint32_t _memLower;
	uint32_t _memUpper;

	uint32_t _bootDevice;
	char *_cmdline;

	uint32_t _modsCount;
	uint32_t _modsAddr;

	uint32_t _syms[4];

	struct {
		uint32_t length;
		MultiBootMmap *base;
	} _mmap;

	uint32_t _drivesLength;
	uint32_t _drivesAddr;

	uint32_t _configTable;
	char *_bootLoaderName;
	uint32_t _apmTable;

	struct {
		uint32_t controlInfo;
		uint32_t modeInfo;
		uint32_t mode;
		struct {
			uint32_t segment;
			uint32_t offset;
			uint32_t length;
		} interface;
	} _vbe;

	struct {
		uint32_t addr;
		uint32_t pitch;
		uint32_t width;
		uint32_t height;
		uint32_t bpp;
		uint32_t type;
	} _framebuffer;

	uint8_t color_info[6];
};

static_assert(sizeof(MultiBootInfo) == 128);

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
private:
	uint32_t _magic;
	FlagField<KernelFlags, uint32_t> _flags;
	uint32_t _checksum;
	uint32_t _hdrAddr;
	uint32_t _loadAddr;
	uint32_t _loadEndAddr;
	uint32_t _bssEndAddr;
};

static_assert(sizeof(MultiBootHeader) == 28);

extern "C" {
	void multiboot_main(const MultiBootInfo *info);
};

#endif /* MULTIBOOT_H */
