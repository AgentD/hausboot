OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386)

SECTIONS
{
	. = 0x0600;

	.text : {
		KEEP(*(.entry))
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
		*(.data)
		*(.data.*)
		*(.bss)
		*(.bss.*)
		FILL(0x00)
		. = 446;
	}

	/DISCARD/ : { *(*) }
}
