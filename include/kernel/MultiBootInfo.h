/* SPDX-License-Identifier: ISC */
/*
 * MultiBootInfo.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef MULTIBOOT_INFO_H
#define MULTIBOOT_INFO_H

#include <cstdint>

#include "kernel/MultiBootMmap.h"
#include "FlagField.h"

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
	FlagField<InfoFlag, uint32_t> _flags{};

	uint32_t _memLower = 0;
	uint32_t _memUpper = 0;

	uint32_t _bootDevice = 0;
	char *_cmdline = nullptr;

	uint32_t _modsCount = 0;
	uint32_t _modsAddr = 0;

	uint32_t _syms[4];

	struct {
		uint32_t length = 0;
		MultiBootMmap *base = nullptr;
	} _mmap;

	uint32_t _drivesLength = 0;
	uint32_t _drivesAddr = 0;

	uint32_t _configTable = 0;
	char *_bootLoaderName = nullptr;
	uint32_t _apmTable = 0;

	struct {
		uint32_t controlInfo = 0;
		uint32_t modeInfo = 0;
		uint32_t mode = 0;
		struct {
			uint32_t segment = 0;
			uint32_t offset = 0;
			uint32_t length = 0;
		} interface;
	} _vbe;

	struct {
		uint32_t addr = 0;
		uint32_t pitch = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t bpp = 0;
		uint32_t type = 0;
	} _framebuffer;

	uint8_t color_info[6];
};

static_assert(sizeof(void *) == sizeof(uint32_t));
static_assert(sizeof(MultiBootInfo) == 128);

#endif /* MULTIBOOT_INFO_H */
