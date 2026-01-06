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

#include "../Part.h"

#include "../Synth.h"

#include "FakeROMs.h"
#include "TestUtils.h"
#include "Testing.h"

namespace MT32Emu {

namespace Test {

namespace {

struct Context {
	Synth synth;
	ROMSet romSet;
	const Part *part;

	void prepareMT32New() {
		romSet.initMT32New();
		openSynth(synth, romSet);

		part = Part::getPart(synth, 0);
		REQUIRE(part != NULL_PTR);

		sendSineWaveSysex(synth, 1);
	}
};

} // namespace

TEST_CASE("Part stops all notes playing on it") {
	Context ctx;
	ctx.prepareMT32New();

	sendNoteOn(ctx.synth, 1, 36, 80);
	sendNoteOn(ctx.synth, 1, 48, 50);
	CHECK(ctx.part->getActivePartialCount() == 2);
	CHECK(ctx.part->getActiveNonReleasingPartialCount() == 2);
	CHECK(ctx.part->getFirstActivePoly() != NULL_PTR);

	SUBCASE("once AllNotesOff is received") {
		sendAllNotesOff(ctx.synth, 1);
	}

	SUBCASE("once ProgramChange is received") {
		sendProgramChange(ctx.synth, 1, 0);
	}

	SUBCASE("once its Patch Temp area is updated") {
		sendSineWaveSysex(ctx.synth, 1);
	}

	CHECK(ctx.part->getActivePartialCount() == 2);
	CHECK(ctx.part->getActiveNonReleasingPartialCount() == 0);
	CHECK(ctx.part->getFirstActivePoly() != NULL_PTR);

	skipRenderedFrames(ctx.synth, 12);
	CHECK(ctx.part->getActivePartialCount() == 0);
	CHECK(ctx.part->getFirstActivePoly() == NULL_PTR);
}

} // namespace Test

} // namespace MT32Emu
