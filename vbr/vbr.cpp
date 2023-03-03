#include "bios.h"
#include "video.h"
#include "MBREntry.h"
#include "FatSuper.h"

static const char *msgErrLoad = "Error loading stage 2!";

[[noreturn]] static void DumpMessageAndHang(const char *msg)
{
	auto *vidmem = (VidmemEntry *)0xB8000;

	for (unsigned int i = 0; i < 80 * 25; ++i) {
		vidmem[i].color.Set(Color::LightGray);
		vidmem[i].character = 0;
	}

	for (unsigned int i = 0; msg[i] != '\0'; ++i)
		vidmem[i].character = msg[i];

	for (;;) {
		__asm__ __volatile__ ("hlt");
	}
}

void main(uint32_t edx, const MBREntry *ent)
{
	auto *super = (FatSuper *)0x7c00;

	auto count = super->ReservedSectors();
	if (count > 32)
		count = 32;

	auto *dst = (uint8_t *)0x1000;

	CHSPacked src = ent->StartAddressCHS();
	uint8_t driveNum = edx & 0x0FF;

	// XXX: we boldly assume the partition to be cylinder
	// aligned, so the stupid CHS arithmetic won't overflow.
	src.SetSector(src.Sector() + 2);

	if (!LoadSectors(driveNum, src, dst, count - 2))
		DumpMessageAndHang(msgErrLoad);

	// TODO: test for magic number or checksum instead of leap-of-faith
	goto *(dst);
}
