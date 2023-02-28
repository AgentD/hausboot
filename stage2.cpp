#include "video.h"

static const char *msg = "Hello from Stage 2!";

__attribute__ ((section(".entry")))
void main(void)
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
