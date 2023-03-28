/* SPDX-License-Identifier: ISC */
/*
 * Memory.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef MEMORY_H
#define MEMORY_H

#ifdef __STDC_HOSTED__
#include <cstdlib>
#else
#include <cstddef>

extern void *malloc(size_t count);
extern void free(void *ptr);
#endif

#endif /* MEMORY_H */

