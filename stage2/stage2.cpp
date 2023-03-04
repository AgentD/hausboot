#include "video.h"
#include "stage2.h"

__attribute__ ((section(".header")))
Stage2Info header;

__attribute__ ((section(".entry")))
void main(void)
{
	DumpMessageAndHang("Hello from Stage 2!");
}
