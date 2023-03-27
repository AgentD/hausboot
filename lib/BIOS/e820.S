/* SPDX-License-Identifier: ISC */
/*
 * e820.S
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
	.code16
	.section ".text"
	.globl	IntCallE820
	.type	IntCallE820, @function
IntCallE820:
	pushl	%ebp
	movl	%esp, %ebp
	pushal

	movl	8(%ebp), %ebx
	movl	(%ebx), %ebx

	movl	$0x0000e820, %eax
	movl	$20, %ecx
	movl	$0x534d4150, %edx
	movl	12(%ebp), %edi
	int	$0x15

	/*
	  Check carry first, some BIOSes don't set the signature and terminate
	  the list with cary flag. If neither is set, something is fishy.
	*/
	jc	_last
	cmpl	$0x534d4150, %eax
	jne	_fail

	/* result write back */
	movl	8(%ebp), %eax
	movl	%ebx, (%eax)

	/* restore state, leave function */
	popal
	xor	%eax, %eax
_out:
	mov	%ebp, %esp
	popl	%ebp
	retl
_last:
	popal
	movl	$1, %eax
	jmp	_out
_fail:
	popal
	movl	$-1, %eax
	jmp	_out
	.size	IntCallE820, .-IntCallE820
