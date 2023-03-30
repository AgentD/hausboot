/* SPDX-License-Identifier: ISC */
/*
 * cxxabi.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "cxxabi.h"

#define ATEXIT_MAX_FUNCS (32)

struct atexit_func_entry_t {
	void (*destructor)(void *);
	void *obj;
};

static_assert(sizeof(atexit_func_entry_t) == 8);

static atexit_func_entry_t atexitFuncs[ATEXIT_MAX_FUNCS];
static unsigned int atexitCount = 0;

void *__dso_handle = nullptr;

int __cxa_atexit(void (*f)(void *), void *objptr, void *dso)
{
	(void)dso;

	if (atexitCount >= ATEXIT_MAX_FUNCS)
		return -1;

	atexitFuncs[atexitCount].destructor = f;
	atexitFuncs[atexitCount].obj = objptr;
	atexitCount += 1;
	return 0;
}

void __cxa_finalize(void *f)
{
	auto i = atexitCount;

	if (f == nullptr) {
		while (i--) {
			if (atexitFuncs[i].destructor != nullptr)
				atexitFuncs[i].destructor(atexitFuncs[i].obj);
		}
	} else {
		while (i--) {
			if (atexitFuncs[i].destructor == f) {
				atexitFuncs[i].destructor(atexitFuncs[i].obj);
				atexitFuncs[i].destructor = nullptr;
			}
		}
	}
}
