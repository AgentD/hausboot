OUTPUT_FORMAT("binary")
OUTPUT_ARCH(i386)

SECTIONS
{
	. = 1M;

	.text : ALIGN(4K) {
		__start_text = .;
		KEEP(*(.mbheader))
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
		. = ALIGN(4K);
		__stop_text = .;
	}

	.data : ALIGN(4K) {
		__start_data = .;
		*(.data)
		*(.data.*)
		. = ALIGN(4K);
		__stop_data = .;
	}

	.bss : ALIGN(4K) {
		__start_bss = .;
		*(COMMON)
		*(.bss)
		*(.bss.*)
		. = ALIGN(4K);
		__stop_bss = .;
	}

	/DISCARD/ : { *(*) }
}
