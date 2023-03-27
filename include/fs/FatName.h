/* SPDX-License-Identifier: ISC */
/*
 * FatName.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef FAT_CHARSET_H
#define FAT_CHARSET_H

[[maybe_unused]] static bool IsValidFatChar(int c)
{
	// Must be printable ASCII
	if (c < 0x20 || c > 0x7E)
		return false;

	// Must be shouty case
	if (c >= 0x61 && c <= 0x7A)
		return false;

	// Must not be :;<=>?
	if (c >= 0x3A && c <= 0x3F)
		return false;

	// more special chars to exclude
	if (c == '|' || c == '\\' || c == '/' || c == '"' || c == '*')
		return false;

	if (c == '+' || c == ',' || c == '[' || c == ']')
		return false;

	return true;
}

[[maybe_unused]] static bool IsShortName(const char *entry)
{
	size_t len = 0;
	while (entry[len] != '\0')
		++len;

	if (len == 0 || len > (8 + 1 + 3))
		return false;

	int dotPos = -1;

	for (size_t i = 0; i < len; ++i) {
		if (!IsValidFatChar(entry[i]))
			return false;

		if (entry[i] == '.') {
			if (dotPos >= 0)
				return false;
			if (dotPos < 0)
				dotPos = i;
		}
	}

	if (dotPos < 0)
		return len <= 8;

	if (dotPos == 0 || dotPos == (int)(len - 1))
		return false;

	if (dotPos > 8 || (len - dotPos) > 4)
		return false;

	return true;
}

#endif /* FAT_CHARSET_H */
