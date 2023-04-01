/* SPDX-License-Identifier: ISC */
/*
 * SysCtrl.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef SYSCTRL_H
#define SYSCTRL_H

#include "types/FlagField.h"
#include "device/io.h"

class SystemCtrlPort {
public:
	enum class PortA : uint8_t {
		FastReset = 0x01,
		EnableA20 = 0x02,
		SecurityLock = 0x08,
		WatchdogStatus = 0x10,
		HDD2DriveActive = 0x40,
		HDD1DriveActive = 0x80,
	};

	enum class PortB : uint8_t {
		ConnectTimer2ToSPKR = 0x01,
		SpeakerEnable = 0x02,
		ParityCeckEnable = 0x04,
		ChannelCheckEnable = 0x08,
		RequestRefresh = 0x10,
		Timer2Output = 0x20,
		ChannelCheck = 0x40,
		ParityCheck = 0x80,
	};

	void EnableFastA20() {
		auto ctrlA = ReadPortA();
		ctrlA.Set(PortA::EnableA20);
		ctrlA.Clear(PortA::FastReset);
		SetPortA(ctrlA);
	}
private:
	FlagField<PortA, uint8_t> ReadPortA() {
		return FlagField<PortA, uint8_t>(IoReadByte(0x92));
	}

	FlagField<PortB, uint8_t> ReadPortB() {
		return FlagField<PortB, uint8_t>(IoReadByte(0x61));
	}

	void SetPortA(FlagField<PortA, uint8_t> flags) {
		IoWriteByte(0x92, flags.RawValue());
	}

	void SetPortB(FlagField<PortB, uint8_t> flags) {
		IoWriteByte(0x61, flags.RawValue());
	}
};

#endif /* SYSCTRL_H */
