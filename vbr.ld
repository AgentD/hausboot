OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386)

SECTIONS
{
	. = 0x7C00;

	.text : {
		KEEP(*(.entry))
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
		FILL(0x00)
		. = 0x01FE;
		SHORT(0xAA55)
	}

	/DISCARD/ : { *(*) }
}
