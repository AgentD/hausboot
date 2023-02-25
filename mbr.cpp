#include "MBRTable.h"
#include "bios.h"

#include <cstdint>

static const char *msgResetDrv = "Drive reset failed!";
static const char *msgNoBootPart = "No boot partition found!";
static const char *msgFailLoad = "Failed to load VBR boot sector!";
static const char *msgNoMagic = "VBR boot sector has no magic number!";

uint16_t driveNumber;

static auto *vidmem = (uint16_t *)0xB8000;
static auto *bootSector = (uint16_t *)0x7C00;
static auto *mbr = (uint16_t *)0x0600;
static auto *mbrStackPtr = (uint16_t *)0x7E00;
static auto *partTable = (MBRTable *)(0x0600 + 446);

[[noreturn]] static void DumpMessageAndHang(const char *msg)
{
	for (unsigned int i = 0; i < 80 * 25; ++i)
		vidmem[i] = 0x0700;

	for (unsigned int i = 0; msg[i] != '\0'; ++i)
		vidmem[i] |= msg[i];

	for (;;) {
		__asm__ __volatile__ ("hlt");
	}
}

[[noreturn]] void main(void)
{
	for (const auto &it : partTable->entries) {
		if (!it.IsBootable())
			continue;

		if (!ResetDrive(driveNumber))
			DumpMessageAndHang(msgResetDrv);

		if (!LoadSector(driveNumber, it.StartAddressCHS(), bootSector))
			DumpMessageAndHang(msgFailLoad);

		if (bootSector[255] != 0xAA55)
			DumpMessageAndHang(msgNoMagic);

		goto *(bootSector);
	}

	DumpMessageAndHang(msgNoBootPart);
}

__attribute__ ((naked, noreturn, section(".entry")))
void mbrEntry(void)
{
	// Save away the drive number at the _original_ location, before
	// relocation, so we don't trample it.
	auto &dn = bootSector[&driveNumber - mbr];

	__asm__ __volatile__ ("mov %%dx, %0\r\n" : "=m"(dn));

	// Relocate the ourselves to make space for the actual boot sector.
	const auto *src = bootSector;
	auto *dst = mbr;

	for (int i = 0; i < 256; ++i)
		*(dst++) = *(src++);

	// Setup a stack and jump into the relocated main function.
	__asm__ __volatile__ ("mov %0, %%sp\r\n" : : "i"(mbrStackPtr));

	goto *((void *)main);
}
