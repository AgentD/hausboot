OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386)

SECTIONS
{
	. = 0x1000;

	.text : {
		KEEP(*(.header))
		KEEP(*(.entry))
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
	}

	/DISCARD/ : { *(*) }
}
