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

static_assert(sizeof(void *) == sizeof(uint32_t),
	      "Multiboot is usable for 32 bit x86 code only!");

class MultiBootMmap {
public:
	enum class Type : uint32_t {
		Usable = 1,
		Reserved = 2,
		ACPI = 3,
		Preserve = 4,
		Broken = 5,
	};

	Type RegionType() const {
		if (_type.Read() < 1 || _type.Read() > 5)
			return Type::Broken;

		return static_cast<Type>(_type.Read());
	}

	auto BaseAddress() const {
		return _baseAddress.Read();
	}

	auto RegionSize() const {
		return _length.Read();
	}

	const auto *Next() const {
		auto next = (uint32_t)this + _size.Read() + sizeof(_size);

		return (const MultiBootMmap *)next;
	}
private:
	UnalignedInt<uint32_t> _size;
	UnalignedInt<uint64_t> _baseAddress;
	UnalignedInt<uint64_t> _length;
	UnalignedInt<uint32_t> _type;
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

extern "C" {
	void multiboot_main(const MultiBootInfo *info);
};

#endif /* MULTIBOOT_H */