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

#include "../Display.h"
#include "../Synth.h"

#include "FakeROMs.h"
#include "TestUtils.h"
#include "Testing.h"

namespace MT32Emu {

namespace Test {

static const Bit32u DISPLAY_BUFFER_SIZE = Display::LCD_TEXT_SIZE + 1;

TEST_CASE("Display shows startup message") {
	ROMSet romSet;
	romSet.initMT32New();

	Synth synth;
	openSynth(synth, romSet);
	Display display(synth);

	bool midiMessageLEDState;
	bool midiMessageLEDUpdated;
	bool lcdUpdated;
	display.checkDisplayStateUpdated(midiMessageLEDState, midiMessageLEDUpdated, lcdUpdated);
	CHECK_FALSE(midiMessageLEDState);
	CHECK_FALSE(midiMessageLEDUpdated);
	CHECK_FALSE(lcdUpdated);

	char targetBuffer[DISPLAY_BUFFER_SIZE];
	display.getDisplayState(targetBuffer, false);
	// TODO: Set a non-trivial startup message in the fake ROM.
	REQUIRE(targetBuffer[0] == 0);
}

TEST_CASE("Display shows channel status in main mode") {
	ROMSet romSet;

	SUBCASE("Old-gen MT-32") {
		romSet.initMT32Old();
	}

	SUBCASE("New-gen MT-32") {
		romSet.initMT32New();
	}

	Synth synth;
	openSynth(synth, romSet);
	Display display(synth);

	bool midiMessageLEDState;
	bool midiMessageLEDUpdated;
	bool lcdUpdated;
	char targetBuffer[DISPLAY_BUFFER_SIZE];
	char expectedBuffer[DISPLAY_BUFFER_SIZE] = "1 2 3 4 5 R |vol:100";

	display.setMainDisplayMode();
	display.checkDisplayStateUpdated(midiMessageLEDState, midiMessageLEDUpdated, lcdUpdated);
	CHECK_FALSE(midiMessageLEDState);
	CHECK_FALSE(midiMessageLEDUpdated);
	CHECK(lcdUpdated);
	display.getDisplayState(targetBuffer, false);
	MT32EMU_CHECK_MEMORY_EQUAL(targetBuffer, expectedBuffer, DISPLAY_BUFFER_SIZE);

	display.voicePartStateChanged(0, true);
	display.checkDisplayStateUpdated(midiMessageLEDState, midiMessageLEDUpdated, lcdUpdated);
	CHECK(midiMessageLEDState);
	CHECK(midiMessageLEDUpdated);
	CHECK(lcdUpdated);
	display.getDisplayState(targetBuffer, false);
	expectedBuffer[0] = 1;
	MT32EMU_CHECK_MEMORY_EQUAL(targetBuffer, expectedBuffer, DISPLAY_BUFFER_SIZE);
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
		expectedBuffer = "Hello there!\x0\x0\x0\x0\x0\x0\x0\x0";
	}

	Synth synth;
	openSynth(synth, romSet);
	Display display(synth);

	bool midiMessageLEDState;
	bool midiMessageLEDUpdated;
	bool lcdUpdated;
	char targetBuffer[DISPLAY_BUFFER_SIZE];

	const char customMessage[] = "Hello there!";
	display.customDisplayMessageReceived(reinterpret_cast<const Bit8u *>(customMessage), 0, sizeof customMessage);
	display.checkDisplayStateUpdated(midiMessageLEDState, midiMessageLEDUpdated, lcdUpdated);
	CHECK_FALSE(midiMessageLEDState);
	CHECK_FALSE(midiMessageLEDUpdated);
	CHECK(lcdUpdated);
	display.getDisplayState(targetBuffer, false);
	MT32EMU_CHECK_MEMORY_EQUAL(targetBuffer, expectedBuffer, DISPLAY_BUFFER_SIZE);
}

} // namespace Test

} // namespace MT32Emu
