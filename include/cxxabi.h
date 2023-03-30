/* SPDX-License-Identifier: ISC */
/*
 * cxxabi.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef CXXABI_H
#define CXXABI_H

extern "C" {
	extern void *__dso_handle;

	int __cxa_atexit(void (*f)(void *), void *objptr, void *dso);

	void __cxa_finalize(void *f);
};

#endif /* CXXABI_H */
