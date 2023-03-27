/* SPDX-License-Identifier: ISC */
/*
 * util.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef UTIL_H
#define UTIL_H

#include <iostream>
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

#endif /* UTIL_H */
