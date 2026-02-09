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

#include "../mt32emu.h"
#include "../mmath.h"

#include "FakeROMs.h"
#include "TestReportHandler.h"
#include "TestUtils.h"
#include "Testing.h"

namespace MT32Emu {

namespace Test {

static void openSynthWithMT32NewROMSet(Synth &synth, ROMSet &romSet) {
	romSet.initMT32New();
	openSynth(synth, romSet);
}

template <class Sample>
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
	TestReportHandler<ReportHandler2> rh(expected);

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
	TestReportHandler<ReportHandler2> rh(expected);

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
	TestReportHandler<ReportHandler2> rh(expected);

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
	TestReportHandler<ReportHandler2> rh(expected);

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
	TestReportHandler<ReportHandler2> rh(expected);

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
	TestReportHandler<ReportHandler2> rh(expected);

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
	TestReportHandler<ReportHandler2> rh(expected);

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
	TestReportHandler<ReportHandler2> rh(expected);

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
	TestReportHandler<ReportHandler2> rh(expected);

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
	TestReportHandler<ReportHandler2> rh(expected);

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

TEST_CASE("When Synth lacks free partials") {
	Synth synth;
	ROMSet romSet;
	romSet.initMT32New();
	openSynth(synth, romSet, 4);
	sendSineWaveSysex(synth, 1);
	sendNoteOn(synth, 1, 36, 100);
	sendNoteOn(synth, 1, 37, 100);
	sendNoteOn(synth, 1, 38, 100);
	sendNoteOn(synth, 1, 39, 100);

	SUBCASE("and a new note is about to play, it should report about silencing currently playing polys") {
		const ReportedEvent expected[] = { ReportedEvent::playingPolySilenced(1, 0), ReportedEvent::midiMessage() };
		TestReportHandler<ReportHandler3> rh(expected);
		synth.setReportHandler3(&rh);
		sendNoteOn(synth, 1, 40, 100);

		rh.checkRemainingEvents();
	}

	SUBCASE("and a note is about to play and the same note is playing on the part in single assign mode, it should not report about silencing polys") {
		const ReportedEvent expected[] = { ReportedEvent::midiMessage() };
		TestReportHandler<ReportHandler3> rh(expected);
		synth.setReportHandler3(&rh);
		sendNoteOn(synth, 1, 38, 100);

		rh.checkRemainingEvents();
	}

	SUBCASE("it should report about skipping new notes due to insufficient free partials") {
		sendSineWaveSysex(synth, 2);

		const ReportedEvent expected[] = { ReportedEvent::noteOnIgnored(1, 0), ReportedEvent::midiMessage() };
		TestReportHandler<ReportHandler3> rh(expected);
		synth.setReportHandler3(&rh);
		// We don't have a reserve and this part has lower priority than the part all the other notes are playing on.
		sendNoteOn(synth, 2, 36, 100);

		rh.checkRemainingEvents();
	}
}

TEST_CASE("Synth disregards invalid and unsupported short MIDI messages") {
	Synth synth;
	ROMSet romSet;
	romSet.initMT32New();
	openSynth(synth, romSet);
	sendSineWaveSysex(synth, 1);

	CHECK_FALSE(synth.isActive());

	SUBCASE("Packed message lacking data bytes") { synth.playMsgNow(0xFFFFFF91); }

	SUBCASE("Unpacked message lacking data bytes") { synth.playMsgOnPart(0, 9, 36, 240); }

	SUBCASE("Unrecognised MIDI Control") { synth.playMsgOnPart(0, 11, 0x78, 2); }

	SUBCASE("Unassigned MIDI channel") { synth.playMsgNow(0x78568f); }

	SUBCASE("Invalid MIDI command") { synth.playMsgNow(0x341271); }

	CHECK_FALSE(synth.isActive());
}

} // namespace Test

} // namespace MT32Emu
