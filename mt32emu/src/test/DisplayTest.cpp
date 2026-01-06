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

#include <cstring>

#include "../Display.h"
#include "../Synth.h"

#include "FakeROMs.h"
#include "TestUtils.h"
#include "Testing.h"

namespace MT32Emu {

namespace Test {

static const Bit32u DISPLAY_BUFFER_SIZE = Display::LCD_TEXT_SIZE + 1;

namespace {

struct DisplayState {
	bool midiMessageLEDState;
	bool midiMessageLEDUpdated;
	bool lcdUpdated;
	char buffer[DISPLAY_BUFFER_SIZE];

	void poll(Display &display) {
		display.checkDisplayStateUpdated(midiMessageLEDState, midiMessageLEDUpdated, lcdUpdated);
		if (lcdUpdated) display.getDisplayState(buffer, false);
	}
};

} // namespace

// In old doctest versions, defining subcases in a function didn't work.
#define MT32EMU_createSubcasesForGenerations() \
SUBCASE("Old-gen MT-32") { \
	romSet.initMT32Old(); \
} \
\
SUBCASE("New-gen MT-32") { \
	romSet.initMT32New(); \
}

TEST_CASE("Display shows startup message") {
	ROMSet romSet;

	MT32EMU_createSubcasesForGenerations();

	Synth synth;
	openSynth(synth, romSet);
	Display display(synth);
	DisplayState displayState;

	displayState.poll(display);
	CHECK_FALSE(displayState.midiMessageLEDState);
	CHECK_FALSE(displayState.midiMessageLEDUpdated);
	CHECK_FALSE(displayState.lcdUpdated);

	char targetBuffer[DISPLAY_BUFFER_SIZE];
	display.getDisplayState(targetBuffer, false);
	MT32EMU_CHECK_MEMORY_EQUAL(targetBuffer, STARTUP_DISPLAY_MESSAGE, DISPLAY_BUFFER_SIZE);
}

TEST_CASE("Display shows channel status in main mode") {
	ROMSet romSet;

	MT32EMU_createSubcasesForGenerations();

	Synth synth;
	openSynth(synth, romSet);
	Display display(synth);
	DisplayState displayState;

	char expectedBuffer[DISPLAY_BUFFER_SIZE] = "1 2 3 4 5 R |vol:100";

	display.setMainDisplayMode();
	displayState.poll(display);
	CHECK_FALSE(displayState.midiMessageLEDState);
	CHECK_FALSE(displayState.midiMessageLEDUpdated);
	CHECK(displayState.lcdUpdated);
	MT32EMU_CHECK_MEMORY_EQUAL(displayState.buffer, expectedBuffer, DISPLAY_BUFFER_SIZE);

	display.voicePartStateChanged(0, true);
	expectedBuffer[0] = 1;
	displayState.poll(display);
	CHECK(displayState.midiMessageLEDState);
	CHECK(displayState.midiMessageLEDUpdated);
	CHECK(displayState.lcdUpdated);
	MT32EMU_CHECK_MEMORY_EQUAL(displayState.buffer, expectedBuffer, DISPLAY_BUFFER_SIZE);

	display.rhythmNotePlayed();
	expectedBuffer[10] = 1;
	displayState.poll(display);
	CHECK(displayState.midiMessageLEDState);
	CHECK_FALSE(displayState.midiMessageLEDUpdated);
	CHECK(displayState.lcdUpdated);
	MT32EMU_CHECK_MEMORY_EQUAL(displayState.buffer, expectedBuffer, DISPLAY_BUFFER_SIZE);

	sendMasterVolumeSysex(synth, 74);
	display.masterVolumeChanged();
	memcpy(&expectedBuffer[Display::LCD_TEXT_SIZE - 3]," 74", 3);
	displayState.poll(display);
	CHECK(displayState.midiMessageLEDState);
	CHECK_FALSE(displayState.midiMessageLEDUpdated);
	CHECK(displayState.lcdUpdated);
	MT32EMU_CHECK_MEMORY_EQUAL(displayState.buffer, expectedBuffer, DISPLAY_BUFFER_SIZE);
}

TEST_CASE("Display shows custom messages") {
	ROMSet romSet;
	const char *expectedBuffer;

	SUBCASE("Old-gen MT-32") {
		romSet.initMT32Old();
		expectedBuffer = "Hello there!        ";
	}

	SUBCASE("New-gen MT-32") {
		romSet.initMT32New();
		expectedBuffer = "Hello there!..      ";
	}

	Synth synth;
	openSynth(synth, romSet);
	Display display(synth);
	DisplayState displayState;

	const char customMessage[] = "Hello there!";
	display.customDisplayMessageReceived(reinterpret_cast<const Bit8u *>(customMessage), 0, sizeof customMessage);
	displayState.poll(display);
	CHECK_FALSE(displayState.midiMessageLEDState);
	CHECK_FALSE(displayState.midiMessageLEDUpdated);
	CHECK(displayState.lcdUpdated);
	MT32EMU_CHECK_MEMORY_EQUAL(displayState.buffer, expectedBuffer, DISPLAY_BUFFER_SIZE);
}

TEST_CASE("Display shows SysEx error messages") {
	ROMSet romSet;

	MT32EMU_createSubcasesForGenerations();

	Synth synth;
	openSynth(synth, romSet);
	Display display(synth);
	DisplayState displayState;

	display.checksumErrorOccurred();
	displayState.poll(display);
	CHECK_FALSE(displayState.midiMessageLEDState);
	CHECK_FALSE(displayState.midiMessageLEDUpdated);
	CHECK(displayState.lcdUpdated);
	MT32EMU_CHECK_MEMORY_EQUAL(displayState.buffer, ERROR_DISPLAY_MESSAGE, DISPLAY_BUFFER_SIZE);
}

TEST_CASE("Display shows program change messages") {
	ROMSet romSet;

	MT32EMU_createSubcasesForGenerations();

	Synth synth;
	openSynth(synth, romSet);
	Display display(synth);
	DisplayState displayState;

	sendSineWaveSysex(synth, 3);
	display.programChanged(2);
	const char expectedBuffer[] = "3|Group 1|Test-sine.";
	displayState.poll(display);
	CHECK_FALSE(displayState.midiMessageLEDState);
	CHECK_FALSE(displayState.midiMessageLEDUpdated);
	CHECK(displayState.lcdUpdated);
	MT32EMU_CHECK_MEMORY_EQUAL(displayState.buffer, expectedBuffer, DISPLAY_BUFFER_SIZE);
}

} // namespace Test

} // namespace MT32Emu
