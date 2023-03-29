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

#endif /* IO_H */
