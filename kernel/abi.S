/* SPDX-License-Identifier: ISC */
/*
 * abi.S
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
.global _start
.extern multiboot_main
.extern inital_stack_ptr

/*
 We use the linker script to set up the following order of sections:
  - text
  - data
  - bss

 Each with 4k alignment. We instruct the linker to export the following
 pseudo symbols for use in the code to get the addresses of the segments.
*/
.extern __start_text
.extern __stop_text

.extern __start_data
.extern __stop_data

.extern __start_bss
.extern __stop_bss

/*
 Multiboot header. Because we are using a flat binary instead of ELF, we
 use manually specify load address and section layout for the boot loader.
 */
.section .mbheader
	.align 4
	.long 0x1BADB002
	.long 0x00010003
	.long -(0x1BADB002 + 0x00010003)
	.long .mbheader
	.long __start_text
	.long __stop_data
	.long __stop_bss
	.long _start

/*
 Entry point that the boot loader jumps to.
*/
.section .text
_start:
	movl	$gdt_desc, %eax
	lgdt	(%eax)

	movl	$0x10, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %fs
	movl	%eax, %gs
	movl	%eax, %ss

	ljmp	$0x08,$.flush
.flush:
	movl	$_init_stack_ptr, %esp
	push	%ebx
	call	multiboot_main

/*
 A dummy GDT with identity mapping, replacing the one from the boot loader.
*/
gdt:
	.quad	0x0000000000000000
	.quad	0x00CF9A000000FFFF
	.quad	0x00CF92000000FFFF
gdt_end:

gdt_desc:
	.word	gdt_end - gdt - 1
	.long	gdt

/*
 Initial kernel stack
*/
.section .bss
_init_stack:
	.space	1024
_init_stack_ptr:
