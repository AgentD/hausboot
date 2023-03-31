/* SPDX-License-Identifier: ISC */
/*
 * abi.S
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
	.code16
	.global main
	.section ".entry"
	.extern __start_stage2
	.extern __stop_stage2
_start:
	xor	%ax, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss
	mov	$__stop_stage2, %esp
	add	$1024, %esp
	pushl	%esp
	calll	main


	.code32
	.section ".text"
	.globl	MBTrampoline
	.type	MBTrampoline, @function
MBTrampoline:
	/* first pop the return address, then the kernel entry point */
	popl	%ecx
	popl	%ecx

	/* get the info pointer */
	popl	%ebx

	movl	$0x2BADB002, %eax
	jmp	*%ecx
	.size	MBTrampoline, .-MBTrampoline
