/* SPDX-License-Identifier: ISC */
/*
 * MBRTable.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef MBR_TABLE_H
#define MBR_TABLE_H

#include "MBREntry.h"

struct MBRTable {
	MBREntry entries[4];
	uint16_t magic;
};

static_assert(sizeof(MBRTable) == (4 * 16 + 2));

#endif /* MBR_TABLE_H */
