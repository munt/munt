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

#include "TestUtils.h"
#include "Testing.h"

namespace MT32Emu {

namespace Test {

namespace {

struct ParserEvent {
	enum Type { SHORT_MESSAGE, SYSEX, REALTIME, DEBUG_MESSAGE } type;
	union {
		Bit32u shortMessage;
		struct {
			const Bit8u *bytes;
			Bit32u length;
		} sysex;
		Bit8u realtime;
		const char *debugMessagePattern;
	};

	ParserEvent(Bit32u message) {
		if (0xF8 <= message && message <= 0xFF) {
			type = REALTIME;
			realtime = Bit8u(message);
		} else {
			type = SHORT_MESSAGE;
			shortMessage = message;
		}
	}

	ParserEvent(const Bit8u *sysexBytes, Bit32u sysexLength) : type(SYSEX) {
		sysex.bytes = sysexBytes;
		sysex.length = sysexLength;
	}

	ParserEvent(const char *pattern) : type(DEBUG_MESSAGE), debugMessagePattern(pattern) {}
}; // struct ParserEvent

MT32EMU_STRINGIFY_ENUM(ParserEvent::Type)

struct TestMidiStreamParser : TestEventHandler<ParserEvent>, MidiStreamParser {
	explicit TestMidiStreamParser(const Array<const ParserEvent> &events) : TestEventHandler(events)
	{}

private:
	void handleShortMessage(const Bit32u shortMessage) {
		const ParserEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(shortMessage);
		if (expectedEvent == NULL) {
			FAIL("Unexpected short message");
		}
		REQUIRE(ParserEvent::SHORT_MESSAGE == expectedEvent->type);
		CHECK(shortMessage == expectedEvent->shortMessage);
	}

	void handleSysex(const Bit8u *sysexBytes, const Bit32u sysexLength) {
		const ParserEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(sysexBytes);
		CAPTURE(sysexLength);
		if (expectedEvent == NULL) {
			FAIL("Unexpected System Exclusive message");
		}
		REQUIRE(ParserEvent::SYSEX == expectedEvent->type);
		REQUIRE(sysexLength == expectedEvent->sysex.length);
		MT32EMU_CHECK_MEMORY_EQUAL(sysexBytes, expectedEvent->sysex.bytes, sysexLength);
	}

	void handleSystemRealtimeMessage(const Bit8u realtime) {
		const ParserEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(realtime);
		if (expectedEvent == NULL) {
			FAIL("Unexpected System Realtime message");
		}
		REQUIRE(ParserEvent::REALTIME == expectedEvent->type);
		CHECK(realtime == expectedEvent->realtime);
	}

	void printDebug(const char *debugMessage) {
		const ParserEvent *expectedEvent = nextExpectedEvent();
		CAPTURE(getCurrentEventIx());
		CAPTURE(debugMessage);
		if (expectedEvent == NULL) {
			FAIL("Unexpected debug message");
		}
		REQUIRE(ParserEvent::DEBUG_MESSAGE == expectedEvent->type);
		MT32EMU_CHECK_STRING_CONTAINS(debugMessage, expectedEvent->debugMessagePattern);
	}
}; // struct TestMidiStreamParser

} // namespace

TEST_CASE("MidiStreamParser should handle Voice messages with restored running status") {
	const ParserEvent expected[] = { 0x231291, 0x563491, 0x001291, 0x003481 };
	TestMidiStreamParser parser(expected);

	const Bit8u stream[] = { 0x91, 0x12, 0x23, 0x34, 0x56, 0x12, 0, 0x81, 0x34, 0 };
	SUBCASE("From stream of bytes processed entirely") {
		parser.parseStream(stream, sizeof stream);
	}

	SUBCASE("From stream of bytes processed individually") {
		for (Bit32u i = 0; i < sizeof stream; i++) {
			parser.parseStream(stream + i, 1);
		}
	}

	SUBCASE("From stream of bytes processed in pairs") {
		for (Bit32u i = 0; i < sizeof stream; i += 2) {
			parser.parseStream(stream + i, 2);
		}
	}

	SUBCASE("From individual short messages") {
		const Bit32u sourceMessages[] = { 0x231291, 0x5634, 0x0012, 0x003481 };
		for (Bit32u i = 0; i < getArraySize(sourceMessages); i++) {
			parser.processShortMessage(sourceMessages[i]);
		}
	}

	parser.checkRemainingEvents();
}

TEST_CASE("MidiStreamParser should handle System Realtime and Voice messages interspersed") {
	const ParserEvent expected[] = { 0x231291, 0xFE, 0x001291 };
	TestMidiStreamParser parser(expected);

	SUBCASE("From byte stream separated") {
		const Bit8u stream[] = { 0x91, 0x12, 0x23, 0xFE, 0x12, 0 };
		parser.parseStream(stream, sizeof stream);
	}

	SUBCASE("From byte stream interleaved") {
		const Bit8u stream[] = { 0x91, 0x12, 0x23, 0x12, 0xFE, 0 };
		parser.parseStream(stream, sizeof stream);
	}

	SUBCASE("As individual short messages") {
		const Bit32u sourceMessages[] = { 0x231291, 0xFE, 0x001291 };
		for (Bit32u i = 0; i < getArraySize(sourceMessages); i++) {
			parser.processShortMessage(sourceMessages[i]);
		}
	}

	parser.checkRemainingEvents();
}

TEST_CASE("MidiStreamParser should ignore Voice messages if running status cannot be restored") {
	const char *messageIgnoredText = "No valid running status yet, MIDI message ignored";
	const ParserEvent expected[] = {
		messageIgnoredText, messageIgnoredText, 0x131291, 0xF6, messageIgnoredText, messageIgnoredText, 0x001291
	};
	TestMidiStreamParser parser(expected);

	SUBCASE("Parsed from byte stream") {
		const Bit8u stream[] = { 0x12, 0x13, 0x91, 0x12, 0x13, 0xF6, 0x12, 0, 0x91, 0x12, 0 };
		parser.parseStream(stream, sizeof stream);
	}

	SUBCASE("As individual short messages") {
		const Bit32u sourceMessages[] = { 0x0011, 0x1312, 0x131291, 0xF6, 0x0012, 0x0011, 0x001291 };
		for (Bit32u i = 0; i < getArraySize(sourceMessages); i++) {
			parser.processShortMessage(sourceMessages[i]);
		}
	}

	parser.checkRemainingEvents();
}

TEST_CASE("MidiStreamParser should ignore incomplete Voice messages") {
	const ParserEvent expected[] = { 0x131291, "Invalid short message", 0x001291 };
	TestMidiStreamParser parser(expected);

	const Bit8u stream[] = { 0x91, 0x12, 0x13, 0x91, 0x12, 0x91, 0x12, 0 };
	parser.parseStream(stream, sizeof stream);

	parser.checkRemainingEvents();
}

TEST_CASE("MidiStreamParser should handle complete System Exclusive messages") {
	const Bit8u sysex[] = { 0xF0, 0x3C, 0x3D, 0x3E, 0xF7 };
	const Bit32u sysexLength = sizeof sysex;
	const ParserEvent expected[] = { ParserEvent(sysex, sysexLength) };
	TestMidiStreamParser parser(expected);
	parser.parseStream(sysex, sysexLength);

	parser.checkRemainingEvents();
}

TEST_CASE("MidiStreamParser should handle System Exclusive messages") {
	const Bit8u stream[] = { 0xF0, 0x3C, 0x3D, 0xFE, 0xF7, 0xF8 };
	const Bit32u streamLength = sizeof stream;
	const Bit8u sysex[] = { 0xF0, 0x3C, 0x3D, 0xF7 };
	const ParserEvent expected[] = { 0xFE, ParserEvent(sysex, Bit32u(sizeof sysex)), 0xF8 };
	TestMidiStreamParser parser(expected);

	SUBCASE("From stream of bytes processed entirely") {
		parser.parseStream(stream, streamLength);
	}

	SUBCASE("From stream of bytes processed individually") {
		for (Bit32u i = 0; i < streamLength; i++) {
			parser.parseStream(stream + i, 1);
		}
	}

	SUBCASE("From stream of bytes processed in pairs") {
		for (Bit32u i = 0; i < streamLength; i += 2) {
			parser.parseStream(stream + i, 2);
		}
	}

	parser.checkRemainingEvents();
}

TEST_CASE("MidiStreamParser should ignore incomplete System Exclusive messages") {
	const ParserEvent expected[] = { 0xF8, "lacks end-of-sysex", 0x3D3C91 };
	TestMidiStreamParser parser(expected);

	SUBCASE("From stream of bytes processed entirely") {
		const Bit8u stream[] = { 0xF8, 0xF0, 0x3C, 0x3D, 0x91, 0x3C, 0x3D };
		parser.parseStream(stream, sizeof stream);
	}

	SUBCASE("From stream of bytes processed individually") {
		const Bit8u stream[] = { 0xF0, 0x3C, 0xF8, 0x3D, 0x91, 0x3C, 0x3D };
		for (Bit32u i = 0; i < sizeof stream; i++) {
			parser.parseStream(stream + i, 1);
		}
	}

	parser.checkRemainingEvents();
}

} // namespace Test

} // namespace MT32Emu
