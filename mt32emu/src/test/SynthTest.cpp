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

#include "../mt32emu.h"
#include "../mmath.h"

#include "FakeROMs.h"
#include "TestUtils.h"

namespace MT32Emu {

namespace Test {

namespace {

struct ReportedEvent {
	enum Type {
		DEBUG_MESSAGE, ERROR_CONTROL_ROM, ERROR_PCM_ROM, LCD_MESSAGE, MIDI_MESSAGE, MIDI_QUEUE_OVERFLOW, MIDI_SYSTEM_REALTIME,
		DEVICE_RESET, DEVICE_RECONFIG, NEW_REVERB_MODE, NEW_REVERB_TIME, NEW_REVERB_LEVEL, POLY_STATE_CHANGED, PROGRAM_CHANGED,
		LCD_STATE_UPDATED, MIDI_MESSAGE_LED_STATE_UPDATED
	};

	static ReportedEvent debugMessage(const char *pattern) {
		ReportedEvent e(DEBUG_MESSAGE);
		e.debugMessageFormatPattern = pattern;
		return e;
	}

	static ReportedEvent errorControlROM() {
		return ReportedEvent(ERROR_CONTROL_ROM);
	}

	static ReportedEvent errorPCMROM() {
		return ReportedEvent(ERROR_PCM_ROM);
	}

	static ReportedEvent lcdMessageShown(const char *message) {
		ReportedEvent e(LCD_MESSAGE);
		e.lcdMessage = message;
		return e;
	}

	static ReportedEvent midiMessage() {
		return ReportedEvent(MIDI_MESSAGE);
	}

	static ReportedEvent midiQueueOverflow() {
		return ReportedEvent(MIDI_QUEUE_OVERFLOW);
	}

	static ReportedEvent midiSystemRealtime(Bit8u systemRealtime) {
		ReportedEvent e(MIDI_SYSTEM_REALTIME);
		e.systemRealtime = systemRealtime;
		return e;
	}

	static ReportedEvent deviceReset() {
		return ReportedEvent(DEVICE_RESET);
	}

	static ReportedEvent deviceReconfig() {
		return ReportedEvent(DEVICE_RECONFIG);
	}

	static ReportedEvent newReverbMode(Bit8u newReverbMode) {
		ReportedEvent e(NEW_REVERB_MODE);
		e.reverbMode = newReverbMode;
		return e;
	}

	static ReportedEvent newReverbTime(Bit8u newReverbTime) {
		ReportedEvent e(NEW_REVERB_TIME);
		e.reverbTime = newReverbTime;
		return e;
	}

	static ReportedEvent newReverbLevel(Bit8u newReverbLevel) {
		ReportedEvent e(NEW_REVERB_LEVEL);
		e.reverbLevel = newReverbLevel;
		return e;
	}

	static ReportedEvent polyStateChanged(Bit8u partNum) {
		ReportedEvent e(POLY_STATE_CHANGED);
		e.polyStateChangePartNum = partNum;
		return e;
	}

	static ReportedEvent programChanged(Bit8u partNum, const char *soundGroupName, const char *patchName) {
		ReportedEvent e(PROGRAM_CHANGED);
		e.programChange.partNum = partNum;
		e.programChange.soundGroupName = soundGroupName;
		e.programChange.patchName = patchName;
		return e;
	}

	static ReportedEvent lcdStateUpdated() {
		return ReportedEvent(LCD_STATE_UPDATED);
	}

	static ReportedEvent midiMessageLEDStateUpdated(bool state) {
		ReportedEvent e(MIDI_MESSAGE_LED_STATE_UPDATED);
		e.midiMessageLEDState = state;
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
	};

private:
	explicit ReportedEvent(Type t) : type(t) {};
};

MT32EMU_STRINGIFY_ENUM(ReportedEvent::Type)

struct TestReportHandler : TestEventHandler<ReportedEvent>, ReportHandler2 {
	explicit TestReportHandler(const Array<const ReportedEvent> &events) : TestEventHandler(events)
	{}

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
};

} // namespace

static void openSynthWithMT32NewROMSet(Synth &synth, ROMSet &romSet) {
	romSet.initMT32New();
	openSynth(synth, romSet);
}

template<class Sample>
static void checkSilence(Sample *buffer, Bit32u size) {
	for (Bit32u i = 0; i < size; i++) {
		CAPTURE(i);
		CHECK(Sample(0) == buffer[i]);
	}
}

TEST_CASE("Synth can be opened with ROMImages") {
	Synth synth;
	ROMSet romSet;
	openSynthWithMT32NewROMSet(synth, romSet);
	CHECK(synth.isOpen());
	CHECK_FALSE(synth.isActive());
	synth.close();
	CHECK_FALSE(synth.isOpen());
}

TEST_CASE("Synth should render silence when inactive") {
	Synth synth;
	ROMSet romSet;
	const Bit32u frameCount = 256;

	SUBCASE("16-bit integer samples") {
		openSynthWithMT32NewROMSet(synth, romSet);
		Bit16s buffer[2 * frameCount];
		synth.render(buffer, frameCount);
		checkSilence(buffer, 2 * frameCount);
	}

	SUBCASE("Float samples") {
		synth.selectRendererType(RendererType_FLOAT);
		openSynthWithMT32NewROMSet(synth, romSet);
		float buffer[2 * frameCount];
		synth.render(buffer, frameCount);
		checkSilence(buffer, 2 * frameCount);
	}
}

TEST_CASE("Synth with float samples should render sine wave") {
	Synth synth;
	synth.selectRendererType(RendererType_FLOAT);
	ROMSet romSet;
	openSynthWithMT32NewROMSet(synth, romSet);
	sendSineWaveSysex(synth, 1);
	sendNoteOn(synth, 1, 60, 127);
	REQUIRE(synth.isActive());

	const Bit32u frameCount = 512;
	float buffer[2 * frameCount];
	synth.render(buffer, frameCount);

	sendAllNotesOff(synth, 1);
	float buffer2[2 * frameCount];
	synth.render(buffer2, frameCount);

	synth.close();
	CHECK_FALSE(synth.isOpen());

	// The left channel is expected to be silent.
	for (Bit32u i = 0; i < 2 * frameCount; i += 2) {
		CAPTURE(i);
		CHECK(Approx(0) == buffer[i]);
	}

	// The right channel should match a sine wave with certain period and amplitude.
	for (Bit32u i = 0; i < 2 * frameCount; i += 2) {
		CAPTURE(i);
		float phase = FLOAT_PI * i / 256.0f;
		CAPTURE(phase);
		CHECK(Approx(0.478154f * sin(phase)) == buffer[i + 1]);
	}

	// After AllNotesOff, the right channel is expected to decay very quickly,
	// while the left channel should stay silent.
	for (Bit32u i = 0; i < 2 * frameCount; i += 2) {
		CAPTURE(i);
		CHECK(Approx(0).epsilon(1e-20) == buffer2[i]);
		CHECK(Approx(0).epsilon(1.5e-6) == buffer2[i + 1]);
	}
}

TEST_CASE("Synth with integer samples should render sine-like wave") {
	Synth synth;
	synth.selectRendererType(MT32Emu::RendererType_BIT16S);
	ROMSet romSet;
	openSynthWithMT32NewROMSet(synth, romSet);
	sendSineWaveSysex(synth, 1);
	sendNoteOn(synth, 1, 60, 127);
	REQUIRE(synth.isActive());

	const Bit32u frameCount = 512;
	float buffer[2 * frameCount];
	synth.render(buffer, frameCount);

	sendAllNotesOff(synth, 1);
	float buffer2[2 * frameCount];
	synth.render(buffer2, frameCount);

	synth.close();
	CHECK_FALSE(synth.isOpen());

	// The left channel is expected to be silent.
	for (Bit32u i = 0; i < 2 * frameCount; i += 2) {
		CAPTURE(i);
		CHECK(Approx(0) == buffer[i]);
	}

	// The right channel is expected to start with ~0f, the maximum should be near index 64.
	CHECK(0 == Approx(buffer[1]).epsilon(0.002));
	for (Bit32u i = 3; i < 2 * 64 + 1; i += 2) {
		CAPTURE(i);
		CHECK(buffer[i - 2] < buffer[i]);
	}
	for (Bit32u i = 2 * 65 + 1; i <= 2 * 128 + 1; i += 2) {
		CAPTURE(i);
		CHECK(buffer[i - 2] > buffer[i]);
	}

	// Symmetry: frames with indices [129..256] should be negated copies of frames [0..128], and
	// frames [257..512] should match frames [0..256].
	for (Bit32u i = 1; i < 2 * 128; i += 2) {
		CAPTURE(i);
		CHECK(Approx(buffer[i]) == -buffer[2 * 128 + i]);
	}
	for (Bit32u i = 1; i < 2 * 256; i += 2) {
		CAPTURE(i);
		CHECK(Approx(buffer[i]) == buffer[2 * 256 + i]);
	}

	// Some trigonometry...
	const Approx ampSquared = Approx(buffer[2 * 64 + 1] * buffer[2 * 64 + 1]).epsilon(0.0002);
	for (Bit32u i = 1; i < 2 * 64; i += 2) {
		float a = buffer[i];
		float b = buffer[2 * 64 + i];
		INFO("i=" << i << ", buffer[i]=" << a << ", buffer[2 * 64 + i]=" << b);
		CHECK(a * a + b * b == ampSquared);
	}

	// After AllNotesOff, the right channel is expected to decay very quickly,
	// while the left channel should stay silent.
	for (Bit32u i = 0; i < 2 * frameCount; i += 2) {
		CAPTURE(i);
		CHECK(Approx(0).epsilon(1e-20) == buffer2[i]);
		CHECK(Approx(0).epsilon(1.5e-6) == buffer2[i + 1]);
	}
}

TEST_CASE("Synth should report errors related to Control ROM contents when opening") {
	const ReportedEvent expected[] = {
		ReportedEvent::debugMessage("invalid Control ROM"),
		ReportedEvent::errorControlROM()
	};
	TestReportHandler rh(expected);

	Synth synth;
	synth.setReportHandler2(&rh);
	ROMSet romSet;
	romSet.initMT32Old(true);
	CHECK_FALSE(synth.open(*romSet.getPCMROMImage(), *romSet.getPCMROMImage()));

	rh.checkRemainingEvents();
}

TEST_CASE("Synth should report errors related to PCM ROM contents when opening") {
	const ReportedEvent expected[] = {
		ReportedEvent::debugMessage("Missing PCM ROM"),
		ReportedEvent::errorPCMROM()
	};
	TestReportHandler rh(expected);

	Synth synth;
	synth.setReportHandler2(&rh);
	ROMSet romSet;
	romSet.initMT32Old();
	CHECK_FALSE(synth.open(*romSet.getControlROMImage(), *romSet.getControlROMImage()));

	rh.checkRemainingEvents();
}

TEST_CASE("Synth should report about displaying custom LCD messages") {
	const char message[] = "Hello!";
	const ReportedEvent expected[] = {
		ReportedEvent::midiMessage(),
		ReportedEvent::lcdMessageShown(message)
	};
	TestReportHandler rh(expected);

	Synth synth;
	ROMSet romSet;
	openSynthWithMT32NewROMSet(synth, romSet);
	synth.setReportHandler2(&rh);
	sendDisplaySysex(synth, message);

	rh.checkRemainingEvents();
}

TEST_CASE("Synth should report about playing short MIDI messages") {
	const ReportedEvent expected[] = {
		ReportedEvent::midiMessage(),
		ReportedEvent::midiMessage()
	};
	TestReportHandler rh(expected);

	Synth synth;
	ROMSet romSet;
	openSynthWithMT32NewROMSet(synth, romSet);
	synth.setReportHandler2(&rh);
	sendNoteOn(synth, 1, 0, 0);
	sendAllNotesOff(synth, 1);

	rh.checkRemainingEvents();
}

TEST_CASE("Synth should report about MIDI queue overflow") {
	const Bit32u message = 0x121381;
	const ReportedEvent expected[] = { ReportedEvent::midiQueueOverflow() };
	TestReportHandler rh(expected);

	Synth synth;
	synth.setMIDIEventQueueSize(4);
	ROMSet romSet;
	openSynthWithMT32NewROMSet(synth, romSet);
	synth.setReportHandler2(&rh);
	CHECK(synth.playMsg(message));
	CHECK(synth.playMsg(message));
	CHECK(synth.playMsg(message));
	CHECK_FALSE(synth.playMsg(message));

	rh.checkRemainingEvents();
}

TEST_CASE("Synth should report about System Realtime MIDI messages") {
	const Bit8u message = 0xF8;
	const ReportedEvent expected[] = { ReportedEvent::midiSystemRealtime(message) };
	TestReportHandler rh(expected);

	Synth synth;
	ROMSet romSet;
	openSynthWithMT32NewROMSet(synth, romSet);
	synth.setReportHandler2(&rh);
	synth.playMsg(message);

	rh.checkRemainingEvents();
}

TEST_CASE("Synth should report about device reset") {
	const ReportedEvent expected[] = {
		ReportedEvent::deviceReset(),
		ReportedEvent::programChanged(0, "Group 1|", ""),
		ReportedEvent::programChanged(1, "Group 1|", ""),
		ReportedEvent::programChanged(2, "Group 1|", ""),
		ReportedEvent::programChanged(3, "Group 1|", ""),
		ReportedEvent::programChanged(4, "Group 1|", ""),
		ReportedEvent::programChanged(5, "Group 1|", ""),
		ReportedEvent::programChanged(6, "Group 1|", ""),
		ReportedEvent::programChanged(7, "Group 1|", ""),
		ReportedEvent::newReverbMode(0),
		ReportedEvent::newReverbTime(5),
		ReportedEvent::newReverbLevel(3)
	};
	TestReportHandler rh(expected);

	Synth synth;
	ROMSet romSet;
	openSynthWithMT32NewROMSet(synth, romSet);
	synth.setReportHandler2(&rh);
	sendSystemResetSysex(synth);

	rh.checkRemainingEvents();
}

TEST_CASE("Synth should report about device reconfiguration") {
	const ReportedEvent expected[] = {
		ReportedEvent::midiMessage(),
		ReportedEvent::deviceReconfig()
	};
	TestReportHandler rh(expected);

	Synth synth;
	ROMSet romSet;
	openSynthWithMT32NewROMSet(synth, romSet);
	synth.setReportHandler2(&rh);

	CHECK(readMasterVolume(synth) == 100);
	sendMasterVolumeSysex(synth, 23);
	CHECK(readMasterVolume(synth) == 23);

	rh.checkRemainingEvents();
}

TEST_CASE("Synth should report about changes in states of polys") {
	const ReportedEvent expected[] = {
		ReportedEvent::polyStateChanged(0),
		ReportedEvent::midiMessage(),
		ReportedEvent::midiMessageLEDStateUpdated(true),
		ReportedEvent::midiMessage(),
		ReportedEvent::polyStateChanged(0)
	};
	TestReportHandler rh(expected);

	Synth synth;
	ROMSet romSet;
	openSynthWithMT32NewROMSet(synth, romSet);
	sendSineWaveSysex(synth, 1);
	synth.setReportHandler2(&rh);

	sendNoteOn(synth, 1, 36, 100);
	CHECK(rh.getCurrentEventIx() == 2);

	skipRenderedFrames(synth, 16);
	CHECK(rh.getCurrentEventIx() == 3);

	sendNoteOn(synth, 1, 36, 0);
	CHECK(rh.getCurrentEventIx() == 4);

	skipRenderedFrames(synth, 16);

	rh.checkRemainingEvents();
}

TEST_CASE("Synth should report about changes in display state") {
	const ReportedEvent expected[] = {
		ReportedEvent::midiMessage(),
		ReportedEvent::midiMessageLEDStateUpdated(true),
		ReportedEvent::lcdStateUpdated(),

		ReportedEvent::midiMessageLEDStateUpdated(false),

		ReportedEvent::midiMessage(),
		ReportedEvent::programChanged(0, "Group 1|", ""),
		ReportedEvent::midiMessage(),
		ReportedEvent::programChanged(0, "Group 1|", "Test-sine."),

		ReportedEvent::polyStateChanged(0),
		ReportedEvent::midiMessage(),

		ReportedEvent::midiMessageLEDStateUpdated(true),
		ReportedEvent::lcdStateUpdated()
	};
	TestReportHandler rh(expected);

	Synth synth;
	ROMSet romSet;
	openSynthWithMT32NewROMSet(synth, romSet);
	synth.setReportHandler2(&rh);

	char display[21];
	synth.getDisplayState(display);
	CHECK(display == "Starting up...      ");

	sendDisplayResetSysex(synth);
	CHECK(rh.getCurrentEventIx() == 1);
	skipRenderedFrames(synth, 1);
	CHECK(rh.getCurrentEventIx() == 3);

	synth.getDisplayState(display);
	CHECK(display == "1 2 3 4 5 R |vol:100");

	skipRenderedFrames(synth, 2560);
	CHECK(rh.getCurrentEventIx() == 3);
	skipRenderedFrames(synth, 1);
	CHECK(rh.getCurrentEventIx() == 4);

	sendSineWaveSysex(synth, 1);
	CHECK(rh.getCurrentEventIx() == 8);

	sendNoteOn(synth, 1, 36, 100);
	CHECK(rh.getCurrentEventIx() == 10);

	skipRenderedFrames(synth, 1);
	CHECK(rh.getCurrentEventIx() == 12);

	synth.getDisplayState(display);
	CHECK(display == "\001 2 3 4 5 R |vol:100");

	rh.checkRemainingEvents();
}

} // namespace Test

} // namespace MT32Emu
