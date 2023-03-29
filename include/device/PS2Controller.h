/* SPDX-License-Identifier: ISC */
/*
 * PS2Controller.h
 *
 * Copyright (C) 2023 David Oberhollenzer <goliath@infraroot.at>
 */
#ifndef PS2CONTROLLER_H
#define PS2CONTROLLER_H

#include "FlagField.h"
#include "device/io.h"

class PS2Controller {
public:
	enum class PS2Status : uint8_t {
		OutputBufferFull = 0x01,
		InputBufferFull = 0x02,
		SystemFlag = 0x04,
		CommandFlag = 0x08,
		Vendor1 = 0x10,
		Vendor2 = 0x20,
		Timeout = 0x40,
		ParityError = 0x80
	};

	auto Status() {
		IoWait();
		return FlagField<PS2Status, uint8_t>(IoReadByte(0x64));
	}

	bool EnableA20Gate() {
		if (!DrainBuffers())
			return false;

		// Command write
		SendCommand(0xd1);
		DrainBuffers();

		// Turn A20 on
		WriteData(0xdf);
		DrainBuffers();

		// Null command, required by UHCI
		SendCommand(0xff);
		DrainBuffers();
		return true;
	}
private:
	auto ReadData() {
		IoWait();
		return IoReadByte(0x60);
	}

	void SendCommand(uint8_t cmd) {
		IoWriteByte(0x64, cmd);
	}

	void WriteData(uint8_t cmd) {
		IoWriteByte(0x60, cmd);
	}

	bool DrainBuffers() {
		int ffCount = 0;

		for (int i = 0; i < 100000; ++i) {
			auto status = Status();

			if (status.RawValue() == 0xFF) {
				// probably no controller present?
				if (ffCount++ > 32)
					return false;
				continue;
			}

			ffCount = 0;

			if (status.IsSet(PS2Status::OutputBufferFull)) {
				(void)ReadData();
				continue;
			}

			if (!status.IsSet(PS2Status::InputBufferFull))
				return true;
		}

		return false;
	}
};

#endif /* PS2CONTROLLER_H */
