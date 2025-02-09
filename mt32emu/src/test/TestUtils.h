/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2024 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

#ifndef MT32EMU_TESTUTILS_H
#define MT32EMU_TESTUTILS_H

#include "../Types.h"

#include "Testing.h"

namespace MT32Emu {

class Synth;

namespace Test {

class ROMSet;

template<class Event>
class TestEventHandler {
	const Array<const Event> expectedEvents;
	size_t currentEventIx;

public:
	explicit TestEventHandler(const Array<const Event> &events) : expectedEvents(events), currentEventIx()
	{}

	const size_t &getCurrentEventIx() const {
		return currentEventIx;
	}

	void checkRemainingEvents() const {
		REQUIRE(currentEventIx == expectedEvents.size);
	}

protected:
	const Event *nextExpectedEvent() {
		if (currentEventIx < expectedEvents.size) return &expectedEvents[currentEventIx++];
		currentEventIx++;
		return NULL;
	}
};

void openSynth(Synth &synth, const ROMSet &romSet);
void sendSystemResetSysex(Synth &synth);
void sendMasterVolumeSysex(Synth &synth, Bit8u volume);
Bit8u readMasterVolume(Synth &synth);
void sendAllNotesOff(Synth &synth, Bit8u channel);
void sendNoteOn(Synth &synth, Bit8u channel, Bit8u note, Bit8u velocity);
// Configures the patch & timbre temp area with a timbre that outputs a pure sine wave with a period of exactly 256 samples
// at the maximum amplitude in the right channel.
void sendSineWaveSysex(Synth &synth, Bit8u channel);
void sendDisplaySysex(Synth &synth, Array<const char>message);
void sendDisplayResetSysex(Synth &synth);

} // namespace Test

} // namespace MT32Emu

#endif // #ifndef MT32EMU_TESTUTILS_H
