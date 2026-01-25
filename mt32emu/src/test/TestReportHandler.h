/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2026 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

#ifndef MT32EMU_TESTREPORTHANDLER_H
#define MT32EMU_TESTREPORTHANDLER_H

#include <cstdarg>

#include "../Types.h"

#include "TestUtils.h"
#include "Testing.h"

namespace MT32Emu {

namespace Test {

struct ReportedEvent {
	enum Type {
		DEBUG_MESSAGE, ERROR_CONTROL_ROM, ERROR_PCM_ROM, LCD_MESSAGE, MIDI_MESSAGE, MIDI_QUEUE_OVERFLOW, MIDI_SYSTEM_REALTIME,
		DEVICE_RESET, DEVICE_RECONFIG, NEW_REVERB_MODE, NEW_REVERB_TIME, NEW_REVERB_LEVEL, POLY_STATE_CHANGED, PROGRAM_CHANGED,
		LCD_STATE_UPDATED, MIDI_MESSAGE_LED_STATE_UPDATED,
		NOTE_ON_IGNORED, PLAYING_POLY_SILENCED
	};

	static const ReportedEvent debugMessage(const char *pattern) {
		ReportedEvent e(DEBUG_MESSAGE);
		e.debugMessageFormatPattern = pattern;
		return e;
	}

	static const ReportedEvent errorControlROM() {
		return ReportedEvent(ERROR_CONTROL_ROM);
	}

	static const ReportedEvent errorPCMROM() {
		return ReportedEvent(ERROR_PCM_ROM);
	}

	static const ReportedEvent lcdMessageShown(const char *message) {
		ReportedEvent e(LCD_MESSAGE);
		e.lcdMessage = message;
		return e;
	}

	static const ReportedEvent midiMessage() {
		return ReportedEvent(MIDI_MESSAGE);
	}

	static const ReportedEvent midiQueueOverflow() {
		return ReportedEvent(MIDI_QUEUE_OVERFLOW);
	}

	static const ReportedEvent midiSystemRealtime(Bit8u systemRealtime) {
		ReportedEvent e(MIDI_SYSTEM_REALTIME);
		e.systemRealtime = systemRealtime;
		return e;
	}

	static const ReportedEvent deviceReset() {
		return ReportedEvent(DEVICE_RESET);
	}

	static const ReportedEvent deviceReconfig() {
		return ReportedEvent(DEVICE_RECONFIG);
	}

	static const ReportedEvent newReverbMode(Bit8u newReverbMode) {
		ReportedEvent e(NEW_REVERB_MODE);
		e.reverbMode = newReverbMode;
		return e;
	}

	static const ReportedEvent newReverbTime(Bit8u newReverbTime) {
		ReportedEvent e(NEW_REVERB_TIME);
		e.reverbTime = newReverbTime;
		return e;
	}

	static const ReportedEvent newReverbLevel(Bit8u newReverbLevel) {
		ReportedEvent e(NEW_REVERB_LEVEL);
		e.reverbLevel = newReverbLevel;
		return e;
	}

	static const ReportedEvent polyStateChanged(Bit8u partNum) {
		ReportedEvent e(POLY_STATE_CHANGED);
		e.polyStateChangePartNum = partNum;
		return e;
	}

	static const ReportedEvent programChanged(Bit8u partNum, const char *soundGroupName, const char *patchName) {
		ReportedEvent e(PROGRAM_CHANGED);
		e.programChange.partNum = partNum;
		e.programChange.soundGroupName = soundGroupName;
		e.programChange.patchName = patchName;
		return e;
	}

	static const ReportedEvent lcdStateUpdated() {
		return ReportedEvent(LCD_STATE_UPDATED);
	}

	static const ReportedEvent midiMessageLEDStateUpdated(bool state) {
		ReportedEvent e(MIDI_MESSAGE_LED_STATE_UPDATED);
		e.midiMessageLEDState = state;
		return e;
	}

	static const ReportedEvent noteOnIgnored(Bit32u partialsNeeded, Bit32u partialsFree) {
		ReportedEvent e(NOTE_ON_IGNORED);
		e.partialAllocationInfo.partialsNeeded = partialsNeeded;
		e.partialAllocationInfo.partialsFree = partialsFree;
		return e;
	}

	static const ReportedEvent playingPolySilenced(Bit32u partialsNeeded, Bit32u partialsFree) {
		ReportedEvent e(PLAYING_POLY_SILENCED);
		e.partialAllocationInfo.partialsNeeded = partialsNeeded;
		e.partialAllocationInfo.partialsFree = partialsFree;
		return e;
	}

	const Type type;
	union {
		const char *debugMessageFormatPattern;
		const char *lcdMessage;
		Bit8u systemRealtime;
		Bit8u reverbMode;
		Bit8u reverbTime;
		Bit8u reverbLevel;
		Bit8u polyStateChangePartNum;
		struct {
			Bit8u partNum;
			const char *soundGroupName;
			const char *patchName;
		} programChange;
		bool midiMessageLEDState;
		struct {
			Bit32u partialsNeeded;
			Bit32u partialsFree;
		} partialAllocationInfo;
	};

private:
	explicit ReportedEvent(Type t) : type(t) {};
}; // struct ReportedEvent

inline MT32EMU_STRINGIFY_ENUM(ReportedEvent::Type)

template <class BaseReportHandler>
class TestReportHandler : public TestEventHandler<ReportedEvent>, public BaseReportHandler {
public:
	explicit TestReportHandler(const Array<const ReportedEvent> &events) : TestEventHandler(events) {}
	virtual ~TestReportHandler() {}

private:
	void printDebug(const char *format, va_list) {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(format);
		if (expectedEvent == NULL) {
			FAIL("Unexpected debug message");
		}
		REQUIRE(ReportedEvent::DEBUG_MESSAGE == expectedEvent->type);
		MT32EMU_CHECK_STRING_CONTAINS(format, expectedEvent->debugMessageFormatPattern);
	}

	void onErrorControlROM() {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		if (expectedEvent == NULL) {
			FAIL("Unexpected control ROM error");
		}
		REQUIRE(ReportedEvent::ERROR_CONTROL_ROM == expectedEvent->type);
	}

	void onErrorPCMROM() {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		if (expectedEvent == NULL) {
			FAIL("Unexpected PCM ROM error");
		}
		REQUIRE(ReportedEvent::ERROR_PCM_ROM == expectedEvent->type);
	}

	void showLCDMessage(const char *message) {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(message);
		if (expectedEvent == NULL) {
			FAIL("Unexpected LCD message");
		}
		REQUIRE(ReportedEvent::LCD_MESSAGE == expectedEvent->type);
		CHECK(message == expectedEvent->lcdMessage);
	}

	void onMIDIMessagePlayed() {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		if (expectedEvent == NULL) {
			FAIL("Unexpected MIDI message");
		}
		REQUIRE(ReportedEvent::MIDI_MESSAGE == expectedEvent->type);
	}

	bool onMIDIQueueOverflow() {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		if (expectedEvent == NULL) {
			FAIL("Unexpected MIDI queue overflow");
		}
		REQUIRE(ReportedEvent::MIDI_QUEUE_OVERFLOW == expectedEvent->type);
		return false;
	}

	void onMIDISystemRealtime(Bit8u systemRealtime) {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(systemRealtime);
		if (expectedEvent == NULL) {
			FAIL("Unexpected MIDI System Realtime");
		}
		REQUIRE(ReportedEvent::MIDI_SYSTEM_REALTIME == expectedEvent->type);
		CHECK(systemRealtime == expectedEvent->systemRealtime);
	}

	void onDeviceReset() {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		if (expectedEvent == NULL) {
			FAIL("Unexpected device reset");
		}
		REQUIRE(ReportedEvent::DEVICE_RESET == expectedEvent->type);
	}

	void onDeviceReconfig() {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		if (expectedEvent == NULL) {
			FAIL("Unexpected device reconfig");
		}
		REQUIRE(ReportedEvent::DEVICE_RECONFIG == expectedEvent->type);
	}

	void onNewReverbMode(Bit8u mode) {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(mode);
		if (expectedEvent == NULL) {
			FAIL("Unexpected reverb mode update");
		}
		REQUIRE(ReportedEvent::NEW_REVERB_MODE == expectedEvent->type);
		CHECK(mode == expectedEvent->reverbMode);
	}

	void onNewReverbTime(Bit8u time) {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(time);
		if (expectedEvent == NULL) {
			FAIL("Unexpected reverb time update");
		}
		REQUIRE(ReportedEvent::NEW_REVERB_TIME == expectedEvent->type);
		CHECK(time == expectedEvent->reverbTime);
	}

	void onNewReverbLevel(Bit8u level) {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(level);
		if (expectedEvent == NULL) {
			FAIL("Unexpected reverb level update");
		}
		REQUIRE(ReportedEvent::NEW_REVERB_LEVEL == expectedEvent->type);
		CHECK(level == expectedEvent->reverbLevel);
	}

	void onPolyStateChanged(Bit8u partNum) {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(partNum);
		if (expectedEvent == NULL) {
			FAIL("Unexpected poly state change");
		}
		REQUIRE(ReportedEvent::POLY_STATE_CHANGED == expectedEvent->type);
		CHECK(partNum == expectedEvent->polyStateChangePartNum);
	}

	void onProgramChanged(Bit8u partNum, const char *soundGroupName, const char *patchName) {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(partNum);
		CAPTURE(soundGroupName);
		CAPTURE(patchName);
		if (expectedEvent == NULL) {
			FAIL("Unexpected program change");
		}
		REQUIRE(ReportedEvent::PROGRAM_CHANGED == expectedEvent->type);
		CHECK(partNum == expectedEvent->programChange.partNum);
		CHECK(soundGroupName == expectedEvent->programChange.soundGroupName);
		CHECK(patchName == expectedEvent->programChange.patchName);
	}

	void onLCDStateUpdated() {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		if (expectedEvent == NULL) {
			FAIL("Unexpected LCD state update");
		}
		REQUIRE(ReportedEvent::LCD_STATE_UPDATED == expectedEvent->type);
	}

	void onMidiMessageLEDStateUpdated(bool ledState) {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(ledState);
		if (expectedEvent == NULL) {
			FAIL("Unexpected MIDI MESSAGE LED state update");
		}
		REQUIRE(ReportedEvent::MIDI_MESSAGE_LED_STATE_UPDATED == expectedEvent->type);
		CHECK(ledState == expectedEvent->midiMessageLEDState);
	}

	void onNoteOnIgnored(Bit32u partialsNeeded, Bit32u partialsFree) {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(partialsNeeded);
		CAPTURE(partialsFree);
		if (expectedEvent == NULL) {
			FAIL("Unexpected NoteOn ignored");
		}
		REQUIRE(ReportedEvent::NOTE_ON_IGNORED == expectedEvent->type);
		CHECK(partialsNeeded == expectedEvent->partialAllocationInfo.partialsNeeded);
		CHECK(partialsFree == expectedEvent->partialAllocationInfo.partialsFree);
	}

	void onPlayingPolySilenced(Bit32u partialsNeeded, Bit32u partialsFree) {
		const ReportedEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(partialsNeeded);
		CAPTURE(partialsFree);
		if (expectedEvent == NULL) {
			FAIL("Unexpected playing poly silenced");
		}
		REQUIRE(ReportedEvent::PLAYING_POLY_SILENCED == expectedEvent->type);
		CHECK(partialsNeeded == expectedEvent->partialAllocationInfo.partialsNeeded);
		CHECK(partialsFree == expectedEvent->partialAllocationInfo.partialsFree);
	}
}; // class TestReportHandler

} // namespace Test

} // namespace MT32Emu

#endif // #ifndef MT32EMU_TESTREPORTHANDLER_H
