#include "MBRTable.h"
#include "bios.h"
#include "video.h"

#include <cstdint>

static const char *msgResetDrv = "Drive reset failed!";
static const char *msgNoBootPart = "No boot partition found!";
static const char *msgFailLoad = "Failed to load VBR boot sector!";
static const char *msgNoMagic = "VBR boot sector has no magic number!";

static auto *vidmem = (VidmemEntry *)0xB8000;
static auto *bootSector = (uint16_t *)0x7C00;
static auto *partTable = (MBRTable *)(0x0600 + 446);

extern "C" {
	[[noreturn]] void CallVbr(uint32_t edx, const MBREntry *ent);
}

[[noreturn]] static void DumpMessageAndHang(const char *msg)
{
	for (unsigned int i = 0; i < 80 * 25; ++i) {
		vidmem[i].color.Set(Color::LightGray);
		vidmem[i].character = 0;
	}

	for (unsigned int i = 0; msg[i] != '\0'; ++i) {
		vidmem[i].character = msg[i];
	}

	for (;;) {
		__asm__ __volatile__ ("hlt");
	}
}

void main(uint32_t edx)
{
	uint16_t driveNumber = edx & 0x0FF;

	for (const auto &it : partTable->entries) {
		if (!it.IsBootable())
			continue;

		if (!ResetDrive(driveNumber))
			DumpMessageAndHang(msgResetDrv);

		if (!LoadSector(driveNumber, it.StartAddressCHS(), bootSector))
			DumpMessageAndHang(msgFailLoad);

		if (bootSector[255] != 0xAA55)
			DumpMessageAndHang(msgNoMagic);

		CallVbr(driveNumber, &it);
	}

	DumpMessageAndHang(msgNoBootPart);
}
