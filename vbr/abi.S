/* SPDX-License-Identifier: ISC */
/*
 * abi.S
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
	.code16
	.global main
	.section ".entry"
_start:
	xor	%ax, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %ss
	mov	$0x7c00, %sp
	pushl	%esi
	pushl	%edx
	calll	main
