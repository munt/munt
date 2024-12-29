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

#include "../mt32emu.h"
#include "../mmath.h"

#include "FakeROMs.h"
#include "TestUtils.h"
#include "Testing.h"

namespace MT32Emu {

namespace Test {

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

} // namespace Test

} // namespace MT32Emu
