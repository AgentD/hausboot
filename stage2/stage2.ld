OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386)

SECTIONS
{
	. = 0x1000;

	.text : {
		__start_stage2 = .;
		KEEP(*(.header))
		KEEP(*(.entry))
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
		FILL(0)
		. = ALIGN(512);
		__stop_stage2 = .;
	}

	/DISCARD/ : { *(*) }
}
