OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386)

SECTIONS
{
	. = 0x7C5A;

	.text : {
		KEEP(*(.entry))
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
	}

	/DISCARD/ : { *(*) }
}
