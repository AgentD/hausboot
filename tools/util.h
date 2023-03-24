/* SPDX-License-Identifier: ISC */
/*
 * util.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef UTIL_H
#define UTIL_H

#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <string>
#include <list>

static void trim(std::string &line)
{
	while (!line.empty() && isspace(line.back()))
		line.pop_back();

	size_t count = 0;
	while (count < line.size() && isspace(line.at(count)))
		++count;

	if (count > 0)
		line.erase(0, count);
}

static std::list<std::string> SplitPath(std::string str)
{
	std::list<std::string> out;

	while (!str.empty()) {
		// drop leadings slashes
		size_t i = 0;
		while (i < str.size() && (str.at(i) == '/' || str.at(i) == '\\'))
			++i;
		if (i > 0) {
			str.erase(0, i);
			continue;
		}

		// find end of current component
		while (i < str.size() && str.at(i) != '/' && str.at(i) != '\\')
			++i;

		if (i == str.size()) {
			out.push_back(str);
			str.clear();
		} else {
			out.push_back(str.substr(0, i));
			str.erase(0, i + 1);
		}
	}

	return out;
}

static bool IsShortName(const std::string &entry)
{
	if (entry.empty() || entry.size() > (8 + 3))
		return false;

	for (auto &c : entry) {
		// Must be printable ASCII
		if (c < 0x20 || c > 0x7E)
			return false;

		// Must not be lower case
		if (c >= 0x61 && c <= 0x7A)
			return false;

		// exclude :;<=>?
		if (c >= 0x3A && c <= 0x3F)
			return false;

		// more special chars to exclude
		if (c == '|' || c == '\\' || c == '/' || c == '"' || c == '*')
			return false;

		if (c == '+' || c == ',' || c == '[' || c == ']')
			return false;
	}

	// Rules for the file extension
	auto pos = entry.find('.');
	if (pos == std::string::npos)
		return true;

	if (pos == 0 || pos == (entry.size() - 1))
		return false;

	if (pos > 8 || (entry.size() - pos) > 4)
		return false;

	for (size_t i = 0; i < entry.size(); ++i) {
		if (entry.at(i) == '.' && i != pos)
			return false;
	}

	return true;
}

#endif /* UTIL_H */
