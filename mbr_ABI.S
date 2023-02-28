	.code16
	.global main
	.global CallVbr
	.section ".entry"
/*
  Main entry point. The BIOS loads the MBR to 0000:7c00 and then jump here.

  We relocate the MBR to 0000:0600 (the linker "thinks" we are there already,
  so the addresses are fixed up if we use the symbols, but we need to be
  carefull about relative addressing!)

  We also setup a stack and call into the main() function, passing along the
  original EDX register value, containing the boot device ID (for use
  with INT 13) in the lower 8 bit.
 */
_start:
	xor	%ax, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss

	mov	$0x7e00, %sp
	pushl	%edx

	mov	$0x0600, %di
	mov	$0x7c00, %si
	mov	$512, %cx

	rep
	movsb

	ljmp	$00,$_relocated

_relocated:
	calll	main

/*
 void CallVbr(uint32_t edx, const MBREntry *ent);

 Jumps into the volume boot record loaded to 0000:7c00, setting up the
 binary interface as expected:
  - DL must containing the boot device number for INT 13
  - DS:SI pointing to the partition table entry that was selected
 */
CallVbr:
	pop	%edx
	pop	%edx
	pop	%esi
	ljmp	$00,$0x7c00
