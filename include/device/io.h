/* SPDX-License-Identifier: ISC */
/*
 * io.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef IO_H
#define IO_H

#include <cstdint>

static inline void IoWriteByte(uint16_t port, uint8_t val)
{
	__asm__ __volatile__("outb %0, %1"
			     : : "a"(val), "Nd"(port));
}

static inline uint8_t IoReadByte(uint16_t port)
{
	uint8_t ret;
	__asm__ __volatile__("inb %1, %0"
			     : "=a"(ret) : "Nd"(port));
	return ret;
}

static inline void IoWait()
{
	IoWriteByte(0x80, 0);
}

static inline void SetFs(uint16_t seg)
{
	__asm__ __volatile__ ("movw %0, %%fs" : : "rm"(seg));
}

static inline uint8_t Peek(uint16_t seg, uint16_t off)
{
	uint8_t *ptr = (uint8_t *)off;
	uint8_t v;
	SetFs(seg);
	__asm__ __volatile__ ("movb %%fs:%1, %0" : "=q"(v) : "m"(*ptr));
	return v;
}

static inline void Poke(uint16_t seg, uint16_t off, uint8_t v)
{
	uint8_t *ptr = (uint8_t *)off;
	SetFs(seg);
	__asm__ __volatile__ ("movb %1, %%fs:%0" : "+m"(*ptr) : "qi"(v));
}

#endif /* IO_H */
