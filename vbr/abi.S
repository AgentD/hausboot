	.code16
	.global main
	.section ".entry"
_start:
	xor	%ax, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss
	mov	$0x7e00, %sp
	pushl	%esi
	pushl	%edx
	calll	main
