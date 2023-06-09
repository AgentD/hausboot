/* SPDX-License-Identifier: ISC */
/*
 * a20.cpp
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#include "device/PS2Controller.h"
#include "device/SysCtrl.h"

static bool TestA20()
{
	auto old = Peek(0x0000, 0x0500);

	for (int i = 0; i < 100; ++i) {
		Poke(0x0000, 0x0500, 0xAA ^ i);
		Poke(0xffff, 0x0510, 0x55 ^ i);

		IoWait();

		bool success = (Peek(0x0000, 0x0500) == (0xAA ^ i) &&
				Peek(0xffff, 0x0510) == (0x55 ^ i));

		if (success) {
			Poke(0x0000, 0x0500, old);
			return true;
		}
	}

	Poke(0x0000, 0x0500, old);
	return false;
}

bool EnableA20()
{
	for (int i = 0; i < 256; ++i) {
		if (TestA20())
			return true;

		// Try using INT 15 service
		__asm__ __volatile__("pushfl\r\n"
				     "int $0x15\r\n"
				     "popfl"
				     : : "a"(0x2401));
		if (TestA20())
			return true;

		// Try via the keyboard controller
		PS2Controller ps2;

		if (ps2.EnableA20Gate()) {
			if (TestA20())
				return true;
		}

		// Try fast A20 enable via system CTRL port A
		SystemCtrlPort ctrl;

		ctrl.EnableFastA20();

		if (TestA20())
			return true;
	}

	return false;
}
