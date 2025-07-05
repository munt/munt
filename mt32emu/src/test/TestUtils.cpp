/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2025 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../Synth.h"

#include "FakeROMs.h"
#include "TestUtils.h"

namespace MT32Emu {

namespace Test {

void openSynth(Synth &synth, const ROMSet &romSet) {
	REQUIRE(romSet.getControlROMImage() != NULL_PTR);
	REQUIRE(romSet.getPCMROMImage() != NULL_PTR);
	REQUIRE(synth.open(*romSet.getControlROMImage(), *romSet.getPCMROMImage(), AnalogOutputMode_DIGITAL_ONLY));
}

void sendSystemResetSysex(Synth &synth) {
	static const Bit8u sysex[] = { 0x7f };
	synth.writeSysex(16, sysex, sizeof sysex);
}

void sendMasterVolumeSysex(Synth &synth, Bit8u volume) {
	const Bit8u sysex[] = { 0x10, 0x00, 0x16, volume };
	synth.writeSysex(16, sysex, sizeof sysex);
}

Bit8u readMasterVolume(Synth &synth) {
	Bit8u volume = 0;
	synth.readMemory(0x40016, sizeof volume, &volume);
	return volume;
}

void sendAllNotesOff(Synth &synth, Bit8u channel) {
	synth.playMsgNow(0x7BB0 | channel);
}

void sendNoteOn(Synth &synth, Bit8u channel, Bit8u note, Bit8u velocity) {
	synth.playMsgNow(0x90 | channel | note << 8 | velocity << 16);
}

void sendSineWaveSysex(Synth &synth, Bit8u channel) {
	static const Bit8u patchSysex[] = {
		0x00, 0x00, 0x00,
		0x00, 0x00, 0x18, 0x19, 0x0c, 0x00, 0x00, 0x00,
		0x64, 0x00
	};
	synth.writeSysex(channel, patchSysex, sizeof patchSysex);
	static const Bit8u timbreSysex[] = {
		0x02, 0x00, 0x00,
		'T', 'e', 's', 't', '-', 's', 'i', 'n', 'e', '.', 0x00, 0x00, 0x01, 0x00,
		0x18, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x07,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32,
		0x00, 0x0b, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x64, 0x32, 0x00, 0x0c, 0x00, 0x0c, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x64,
		0x64, 0x64
	};
	synth.writeSysex(channel, timbreSysex, sizeof timbreSysex);
}

void sendAssignModeSysex(Synth &synth, Bit8u channel, Bit8u assignMode) {
	const Bit8u sysex[] = { 0x00, 0x00, 0x05, assignMode };
	synth.writeSysex(channel, sysex, sizeof sysex);
}

void sendDisplaySysex(Synth &synth, Array<const char>message) {
	static const Bit8u sysexAddress[] = { 0x20, 0x00, 0x00 };
	static const Bit8u maxMessageLength = 20;

	Bit8u sysex[sizeof sysexAddress + maxMessageLength];
	size_t messageLength = maxMessageLength < message.size ? maxMessageLength : message.size;
	memcpy(sysex, sysexAddress, sizeof sysexAddress);
	memcpy(sysex + sizeof sysexAddress, message.data, messageLength);
	synth.writeSysex(16, sysex, sizeof sysex);
}

void sendDisplayResetSysex(Synth &synth) {
	static const Bit8u sysex[] = { 0x20, 0x01, 0x00 };
	synth.writeSysex(16, sysex, sizeof sysex);
}

void skipRenderedFrames(Synth &synth, Bit32u framesToSkip) {
	Bit16s buffer[2 * MAX_SAMPLES_PER_RUN];
	while (framesToSkip > 0) {
		Bit32u frameCount = framesToSkip > MAX_SAMPLES_PER_RUN ? MAX_SAMPLES_PER_RUN : framesToSkip;
		synth.render(buffer, frameCount);
		framesToSkip -= frameCount;
	}
}

} // namespace Test

} // namespace MT32Emu
