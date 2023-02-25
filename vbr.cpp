#include <cstdint>

static const char *msg = "Hello from VBR!";

__attribute__ ((naked, noreturn, section(".entry")))
void main(void)
{
	uint16_t *vidmem = (uint16_t *)0xB8000;

	for (unsigned int i = 0; i < 80 * 25; ++i)
		vidmem[i] = 0x0700;

	for (unsigned int i = 0; msg[i] != '\0'; ++i)
		vidmem[i] |= msg[i];

	for (;;) {
		__asm__ __volatile__ ("hlt");
	}
}
