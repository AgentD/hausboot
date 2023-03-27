/* SPDX-License-Identifier: ISC */
/*
 * StringUtil.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef STRING_UTIL_H
#define STRING_UTIL_H

static bool StrEqual(const char *a, const char *b)
{
	for (;;) {
		if (*a != *b)
			return false;
		if (*a == '\0')
			break;
		++a;
		++b;
	}
	return true;
}

static bool IsSpace(int x)
{
	return x == ' ' || x == '\t';
}

static bool IsAlnum(int x)
{
	return (x >= 'A' && x <= 'Z') || (x >= 'a' && x <= 'z') ||
		(x >= '0' && x <= '9');
}

#endif /* STRING_UTIL_H */
