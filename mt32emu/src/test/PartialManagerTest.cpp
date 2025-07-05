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

#include "../PartialManager.h"

#include "../Part.h"
#include "../Poly.h"
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
	PartialManager *partialManager;

	void prepareMT32New() {
		romSet.initMT32New();
		openSynth(synth, romSet);
		CHECK(DEFAULT_MAX_PARTIALS == synth.getPartialCount());

		partialManager = PartialManager::getPartialManager(synth);
		REQUIRE(partialManager != NULL_PTR);
		CHECK(DEFAULT_MAX_PARTIALS == partialManager->getFreePartialCount());

		sendSineWaveSysex(synth, 1);
	}
};

} // namespace

TEST_CASE("PartialManager allocates partials for playing a Note and frees when finished") {
	Context ctx;
	ctx.prepareMT32New();

	sendNoteOn(ctx.synth, 1, 36, 80);
	CHECK(DEFAULT_MAX_PARTIALS - 1 == ctx.partialManager->getFreePartialCount());

	sendAllNotesOff(ctx.synth, 1);
	CHECK(ctx.partialManager->getAbortingPoly() == NULL_PTR);
	CHECK(DEFAULT_MAX_PARTIALS - 1 == ctx.partialManager->getFreePartialCount());

	skipRenderedFrames(ctx.synth, 12);
	CHECK(DEFAULT_MAX_PARTIALS == ctx.partialManager->getFreePartialCount());
}

TEST_CASE("PartialManager::freePartials") {
	Context ctx;
	ctx.prepareMT32New();

	sendNoteOn(ctx.synth, 1, 36, 80);
	CHECK(DEFAULT_MAX_PARTIALS - 1 == ctx.partialManager->getFreePartialCount());

	SUBCASE("does not initiate abortion and permits allocation of partials") {
		SUBCASE("if no free partials needed") {
			CHECK(ctx.partialManager->freePartials(0, 0));
		}

		SUBCASE("while there are sufficient free partials") {
			CHECK(ctx.partialManager->freePartials(DEFAULT_MAX_PARTIALS - 1, 0));
		}

		CHECK(ctx.partialManager->getAbortingPoly() == NULL_PTR);
	}

	SUBCASE("initiates abortion of playing Note") {
		SUBCASE("on the same part where reserve exceeded") {
			CHECK(ctx.partialManager->freePartials(DEFAULT_MAX_PARTIALS, 0));
		}

		SUBCASE("on a part with lower priority where reserve exceeded") {
			CHECK(ctx.partialManager->freePartials(DEFAULT_MAX_PARTIALS, 8));
		}

		SUBCASE("in releasing phase on a part with higher priority where reserve exceeded") {
			const Poly *playingPoly = Part::getPart(ctx.synth, 0)->getFirstActivePoly();
			CHECK(playingPoly->getState() == POLY_Playing);
			sendAllNotesOff(ctx.synth, 1);
			CHECK(playingPoly->getState() == POLY_Releasing);
			CHECK(ctx.partialManager->freePartials(DEFAULT_MAX_PARTIALS, 1));
		}

		SUBCASE("on a part with higher priority where reserve exceeded if target part is within the reserve") {
			Bit8u reserve[] = { 0, DEFAULT_MAX_PARTIALS, 0, 0, 0, 0, 0, 0, 0 };
			ctx.partialManager->setReserve(reserve);
			CHECK(ctx.partialManager->freePartials(DEFAULT_MAX_PARTIALS, 1));
		}

		SUBCASE("on the same part within the reserve") {
			Bit8u reserve[] = { DEFAULT_MAX_PARTIALS, 0, 0, 0, 0, 0, 0, 0, 0 };
			ctx.partialManager->setReserve(reserve);
			CHECK(ctx.partialManager->freePartials(DEFAULT_MAX_PARTIALS, 0));
		}

		CHECK(ctx.partialManager->getAbortingPoly() != NULL_PTR);
	}

	SUBCASE("reports insufficient free partials when attempting to play Note") {
		SUBCASE("on a part with lower priority where reserve exceeded and nothing to abort on target part and below") {
			CHECK_FALSE(ctx.partialManager->freePartials(DEFAULT_MAX_PARTIALS, 1));
		}

		SUBCASE("on a part with priority given to earlier Notes where reserve exceeded") {
			sendAssignModeSysex(ctx.synth, 1, 1);
			CHECK_FALSE(ctx.partialManager->freePartials(DEFAULT_MAX_PARTIALS, 0));
		}

		SUBCASE("on another part within the reserve and nothing to abort") {
			Bit8u reserve[] = { 1, DEFAULT_MAX_PARTIALS, 0, 0, 0, 0, 0, 0, 0 };
			ctx.partialManager->setReserve(reserve);
			CHECK_FALSE(ctx.partialManager->freePartials(DEFAULT_MAX_PARTIALS, 1));
		}

		CHECK(ctx.partialManager->getAbortingPoly() == NULL_PTR);
	}

	CHECK(DEFAULT_MAX_PARTIALS - 1 == ctx.partialManager->getFreePartialCount());
}

} // namespace Test

} // namespace MT32Emu
