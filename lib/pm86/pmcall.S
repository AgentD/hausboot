/* SPDX-License-Identifier: ISC */
/*
 * pmcall.S
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
	.code16
	.section ".text"
	.globl	ProtectedModeCallRAW
	.type	ProtectedModeCallRAW, @function
ProtectedModeCallRAW:
	/* save away the 16 bit context */
	popl	%eax
	movl	%eax, (_scratch)
	movl	%ebp, (_scratch + 4)
	movw	%ss, %ax
	movw	%ax, (_scratch + 8)
	movw	%sp, (_scratch + 10)
	movw	%ds, (_scratch + 12)
	movw	%es, (_scratch + 14)

	/* calculate abolute, 32 bit stack pointer */
	movzx	%sp, %ebp
	shl	$4, %eax
	addl	%eax, %ebp
	movl	%ebp, %esp

	/* enter protected mode */
	cli
	cld

	xor	%eax, %eax
	movw	%ax, %ds
	movl	$gdt_desc, %eax
	lgdt	(%eax)

	movl	%cr0, %eax
	or	0x01, %al
	movl	%eax, %cr0
	ljmp	$0x08,$_enterpm

	.code32
_enterpm:
	/* call into the desired 32 bit function */
	mov	$0x10, %eax
	mov	%eax, %ss
	mov	%eax, %ds

	pop	%eax
	call	*%eax

	/* jump into a 16 bit segment */
	cli
	cld
	ljmp	$0x18,$_seg16
	.code16
_seg16:
	movw	$0x20, %ax
	movw	%ax, %es
	movw	%ax, %ds
	movw	%ax, %ss

	/* leave protected mode */
	movl	%cr0, %eax
	and	~0x01, %al
	movl	%eax, %cr0
	ljmp	$0,$_leavepm
_leavepm:
	/* restore old 16 bit environment */
	movl	(_scratch + 4), %ebp
	movw	(_scratch + 8), %ss
	movw	(_scratch + 10), %ax
	movzx	%ax, %esp
	movw	(_scratch + 12), %ds
	movw	(_scratch + 14), %es

	/* repair stack and return */
	movl	(_scratch), %eax
	pushl	%eax
	retl
gdt:
	.quad	0x0000000000000000	/* 0x00: null segment */
	.quad	0x00CF9A000000FFFF	/* 0x08: 32 bit code segment */
	.quad	0x00CF92000000FFFF	/* 0x10: 32 bit data segment */
	.quad	0x008F9A000000FFFF	/* 0x18: 16 bit code segment */
	.quad	0x008F92000000FFFF	/* 0x18: 16 bit data segment */
gdt_end:

gdt_desc:
	.word	gdt_end - gdt - 1
	.long	gdt
_scratch:
	.quad	0x0000000000000000
	.quad	0x0000000000000000
	.size	ProtectedModeCallRAW, .-ProtectedModeCallRAW
