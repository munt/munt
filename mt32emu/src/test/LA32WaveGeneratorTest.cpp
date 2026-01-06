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

#include "../LA32WaveGenerator.h"
#include "../LA32FloatWaveGenerator.h"
#include "../Synth.h"
#include "../Tables.h"

#include "Testing.h"

namespace MT32Emu {

namespace Test {

static const Bit16u *exp9;
static const Bit16u *logsin9;

static void initTables() {
	static bool firstRun = true;

	if (firstRun) {
		firstRun = false;
		const Tables &tables = Tables::getInstance();
		LA32IntPartialPair::initTables(tables);
		LA32FloatPartialPair::initTables(tables);
		exp9 = tables.exp9;
		logsin9 = tables.logsin9;
	}
}

namespace {

typedef LA32PartialPair::PairType PairType;

struct Context {
	LA32IntPartialPair intPair;
	LA32FloatPartialPair floatPair;

	Bit32u masterAmp;
	Bit16u masterPitch;
	Bit8u masterCutoff;

	Bit32u slaveAmp;
	Bit16u slavePitch;
	Bit8u slaveCutoff;

	const FloatSample *expectedSamples;
	size_t expectedSampleCount;

	Context() :
		masterAmp(),
		masterPitch(),
		masterCutoff(),
		slaveAmp(),
		slavePitch(),
		slaveCutoff(),
		expectedSamples(),
		expectedSampleCount()
	{}

	void initPair(const bool ringModulated, const bool mixed) {
		initTables();
		intPair.init(ringModulated, mixed);
		floatPair.init(ringModulated, mixed);
	}

	void initSynth(const PairType master, const bool sawtoothWaveform, const Bit8u pulseWidth, const Bit8u resonance) {
		intPair.initSynth(master, sawtoothWaveform, pulseWidth, resonance);
		floatPair.initSynth(master, sawtoothWaveform, pulseWidth, resonance);
	}

	void deactivate(const PairType master) {
		intPair.deactivate(master);
		floatPair.deactivate(master);
	}

	void initSingleSynth(const bool sawtoothWaveform, const Bit8u pulseWidth, const Bit8u resonance) {
		initPair(false, false);
		initSynth(LA32PartialPair::MASTER, sawtoothWaveform, pulseWidth, resonance);
		deactivate(LA32PartialPair::SLAVE);
	}

	void setExpectedSamples(Array<const FloatSample> useExpectedSamples) {
		expectedSamples = useExpectedSamples.data;
		expectedSampleCount = useExpectedSamples.size;
	}

	void generateNextSample() {
		const Bit32u masterCutoffVal = masterCutoff << 18;
		intPair.generateNextSample(LA32PartialPair::MASTER, masterAmp, masterPitch, masterCutoffVal);
		floatPair.generateNextSample(LA32PartialPair::MASTER, masterAmp, masterPitch, masterCutoffVal);

		const Bit32u slaveCutoffVal = slaveCutoff << 18;
		intPair.generateNextSample(LA32PartialPair::SLAVE, slaveAmp, slavePitch, slaveCutoffVal);
		floatPair.generateNextSample(LA32PartialPair::SLAVE, slaveAmp, slavePitch, slaveCutoffVal);
	}

	IntSample nextIntOutSample() {
		return intPair.nextOutSample();
	}

	FloatSample nextFloatOutSample() {
		return floatPair.nextOutSample();
	}
}; // struct Context

} // namespace

TEST_CASE("LA32WaveGenerator should produce various synth waveforms") {
	Context ctx;

	SUBCASE("sine") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000f, 0.048773f, 0.095671f, 0.138893f, 0.176777f, 0.207867f, 0.230970f, 0.245196f,
			0.250000f, 0.245196f, 0.230970f, 0.207867f, 0.176777f, 0.138893f, 0.095671f, 0.048773f,
			0.000000f, -0.048773f, -0.095671f, -0.138893f, -0.176777f, -0.207867f, -0.230970f, -0.245196f,
			-0.250000f, -0.245196f, -0.230970f, -0.207867f, -0.176777f, -0.138893f, -0.095671f, -0.048773f
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xb000;
		ctx.masterCutoff = 0x80;
		ctx.initSingleSynth(false, 0, 0);
	}

	SUBCASE("symmetric square") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f,
			0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f,
			0.000000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f,
			-0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xb000;
		ctx.masterCutoff = 0xf0;
		ctx.initSingleSynth(false, 0, 0);
	}

	SUBCASE("asymmetric square") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f, 0.250000f,
			0.000000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f,
			-0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f,
			-0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f, -0.250000f
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xb000;
		ctx.masterCutoff = 0xc0;
		ctx.initSingleSynth(false, 192, 0);
	}

	SUBCASE("symmetric resonant square") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000f, 0.236627f, 0.364626f, 0.327616f, 0.250000f, 0.178826f, 0.153612f, 0.184733f,
			0.250000f, 0.309850f, 0.331052f, 0.304883f, 0.250000f, 0.199672f, 0.181843f, 0.203849f,
			0.250000f, 0.292321f, 0.307313f, 0.288808f, 0.250000f, 0.214413f, 0.201806f, 0.217366f,
			0.250000f, 0.279925f, 0.290526f, 0.277441f, 0.250000f, 0.224836f, 0.215922f, 0.165239f,
			0.000000f, -0.236304f, -0.363391f, -0.326365f, -0.250000f, -0.180727f, -0.156693f, -0.187161f,
			-0.250000f, -0.307003f, -0.326780f, -0.301709f, -0.250000f, -0.203093f, -0.186819f, -0.207450f,
			-0.250000f, -0.288599f, -0.301990f, -0.285014f, -0.250000f, -0.218238f, -0.207219f, -0.221188f,
			-0.250000f, -0.276136f, -0.285204f, -0.273709f, -0.250000f, -0.228493f, -0.221032f, -0.167022f
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xa000;
		ctx.masterCutoff = 0xb0;
		ctx.initSingleSynth(false, 0, 24);
	}

	SUBCASE("asymmetric resonant square") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000f, 0.236627f, 0.364626f, 0.327616f, 0.250000f, 0.178826f, 0.153612f, 0.184733f,
			0.250000f, 0.309850f, 0.331052f, 0.304883f, 0.250000f, 0.199672f, 0.181843f, 0.153701f,
			0.000000f, -0.236304f, -0.363391f, -0.326365f, -0.250000f, -0.180727f, -0.156693f, -0.187161f,
			-0.250000f, -0.307003f, -0.326780f, -0.301709f, -0.250000f, -0.203093f, -0.186819f, -0.207450f,
			-0.250000f, -0.288599f, -0.301990f, -0.285014f, -0.250000f, -0.218238f, -0.207219f, -0.221188f,
			-0.250000f, -0.276136f, -0.285204f, -0.273709f, -0.250000f, -0.228493f, -0.221032f, -0.230491f,
			-0.250000f, -0.267697f, -0.273837f, -0.266054f, -0.250000f, -0.235437f, -0.230385f, -0.236790f,
			-0.250000f, -0.261983f, -0.266141f, -0.260870f, -0.250000f, -0.240139f, -0.236718f, -0.172304f
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xa000;
		ctx.masterCutoff = 0xb0;
		ctx.initSingleSynth(false, 192, 24);
	}

	SUBCASE("symmetric sawtooth") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000f, 0.220296f, 0.231185f, 0.207850f, 0.176747f, 0.138886f, 0.095672f, 0.048773f,
			0.000000f, -0.048773f, -0.095671f, -0.138893f, -0.176777f, -0.207867f, -0.230970f, -0.219701f,
			0.000000f, 0.220291f, 0.231182f, 0.207850f, 0.176748f, 0.138886f, 0.095672f, 0.048773f,
			0.000000f, -0.048773f, -0.095671f, -0.138893f, -0.176777f, -0.207867f, -0.230970f, -0.219700f
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xb000;
		ctx.masterCutoff = 0xa8;
		ctx.initSingleSynth(true, 0, 0);
	}

	SUBCASE("asymmetric sawtooth") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000f, 0.220296f, 0.231185f, 0.207850f, 0.176747f, 0.138886f, 0.095672f, 0.043702f,
			-0.000000f, 0.043819f, 0.095759f, 0.138881f, 0.176748f, 0.207858f, 0.230972f, 0.245199f,
			0.250000f, 0.245196f, 0.230970f, 0.207867f, 0.176777f, 0.138893f, 0.095671f, 0.048773f,
			0.000000f, -0.048773f, -0.095671f, -0.138893f, -0.176777f, -0.207867f, -0.230970f, -0.21970f
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xb000;
		ctx.masterCutoff = 0xa8;
		ctx.initSingleSynth(true, 192, 0);
	}

	SUBCASE("ring modulation") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000f, 0.023430f, -0.027314f, -0.045586f, 0.088776f, 0.009978f, -0.135770f, 0.077375f,
			0.122290f, -0.172582f, -0.035072f, 0.218686f, -0.095208f, -0.179530f, 0.208842f, 0.062590f,
			-0.249770f, 0.082996f, 0.197064f, -0.192420f, -0.076823f, 0.220287f, -0.052502f, -0.164495f,
			0.132780f, 0.065226f, -0.137780f, 0.020004f, 0.085393f, -0.050257f, -0.023750f, 0.023959f,
			0.000000f, 0.022729f, -0.030676f, -0.040579f, 0.091506f, -0.000122f, -0.132761f, 0.088955f,
			0.110901f, -0.179400f, -0.017385f, 0.215476f, -0.112891f, -0.165318f, 0.219084f, 0.041724f,
			-0.247932f, 0.102790f, 0.183835f, -0.203895f, -0.057874f, 0.220268f, -0.069545f, -0.155197f,
			0.142293f, 0.052597f, -0.138777f, 0.029884f, 0.081382f, -0.054559f, -0.020012f, 0.024311f
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xa000;
		ctx.masterCutoff = 0x80;
		ctx.slavePitch = 0xe400;
		ctx.slaveCutoff = 0x80;
		ctx.initPair(true, false);
		ctx.initSynth(LA32PartialPair::MASTER, false, 0, 0);
		ctx.initSynth(LA32PartialPair::SLAVE, false, 0, 0);
	}

	SUBCASE("ring modulation mixed with master") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000f, 0.047934f, 0.021459f, 0.026985f, 0.184447f, 0.127827f, 0.003122f, 0.235973f,
			0.299067f, 0.020670f, 0.172795f, 0.439166f, 0.135762f, 0.059706f, 0.454039f, 0.311386f,
			0.000230f, 0.331792f, 0.442260f, 0.046815f, 0.154147f, 0.440767f, 0.155365f, 0.028758f,
			0.309557f, 0.223824f, 0.001112f, 0.137854f, 0.181064f, 0.022314f, 0.025022f, 0.048463f,
			0.000000f, -0.001775f, -0.079449f, -0.113150f, -0.004165f, -0.117971f, -0.271654f, -0.069644f,
			-0.065876f, -0.372652f, -0.225252f, -0.005005f, -0.343861f, -0.404553f, -0.026112f, -0.207072f,
			-0.497932f, -0.146006f, -0.061361f, -0.443130f, -0.288844f, -0.000213f, -0.277413f, -0.348450f,
			-0.034484f, -0.106002f, -0.277669f, -0.087965f, -0.014289f, -0.127130f, -0.068785f, -0.000193f
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xa000;
		ctx.masterCutoff = 0x80;
		ctx.slavePitch = 0xe400;
		ctx.slaveCutoff = 0x80;
		ctx.initPair(true, true);
		ctx.initSynth(LA32PartialPair::MASTER, false, 0, 0);
		ctx.initSynth(LA32PartialPair::SLAVE, false, 0, 0);
	}

	SUBCASE("ring modulation with distortion") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000f, 0.047301f, 0.105126f, 0.170305f, 0.238719f, -0.194330f, -0.133685f, -0.083874f,
			-0.048683f, -0.030794f, -0.030023f, -0.036897f, -0.049482f, -0.067387f, -0.090078f, -0.116897f,
			-0.147078f, -0.179776f, -0.214079f, -0.249045f, 0.216281f, 0.182834f, 0.151510f, 0.123151f,
			0.099966f, 0.084839f, 0.078011f, 0.076965f, 0.077154f, 0.073234f, 0.060491f, 0.036113f,
			0.000000f, 0.047294f, 0.105072f, 0.170130f, 0.238332f, -0.195022f, -0.134755f, -0.085361f,
			-0.050578f, -0.033037f, -0.032513f, -0.039549f, -0.052203f, -0.070070f, -0.092607f, -0.119150f,
			-0.148936f, -0.181122f, -0.214808f, -0.249065f, 0.217040f, 0.184421f, 0.153946f, 0.126406f,
			0.103801f, 0.088877f, 0.081834f, 0.080197f, 0.079545f, 0.074715f, 0.061182f, 0.036285f
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xa000;
		ctx.masterCutoff = 0x8c;
		ctx.slavePitch = 0xa000;
		ctx.slaveCutoff = 0xf0;
		ctx.initPair(true, false);
		ctx.initSynth(LA32PartialPair::MASTER, false, 0, 28);
		ctx.initSynth(LA32PartialPair::SLAVE, false, 0, 0);
	}

	REQUIRE(ctx.expectedSamples != NULL_PTR);
	for (size_t i = 0; i < ctx.expectedSampleCount; i++) {
		CAPTURE(i);
		ctx.generateNextSample();
		IntSample intSample = ctx.nextIntOutSample();
		Approx floatSample = Approx(ctx.nextFloatOutSample());
		CHECK(floatSample == ctx.expectedSamples[i]);
		CAPTURE(intSample);
		CHECK(Synth::convertSample(intSample) == floatSample.epsilon(0.003));
	}
}

} // namespace Test

} // namespace MT32Emu
