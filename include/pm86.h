/* SPDX-License-Identifier: ISC */
/*
 * pm86.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef PM86_H
#define PM86_H

#include <type_traits>

// Try to enable the A20 line by various means and test if it worked
extern bool EnableA20();

extern "C" {
	// Assembly implementation of ProtectedModeCall
	void ProtectedModeCallRAW(...);
}

/*
  Switch to protected mode, call a 32 bit function with possible
  arguments and switch back into 16 bit real-mode before returning.
*/
template<typename... Ts>
void ProtectedModeCall(void(*fun)(Ts...), Ts... args)
{
	ProtectedModeCallRAW(fun, args...);
}

#endif /* PM86_H */
