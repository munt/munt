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
			0.000000, 0.048773, 0.095671, 0.138893, 0.176777, 0.207867, 0.230970, 0.245196,
			0.250000, 0.245196, 0.230970, 0.207867, 0.176777, 0.138893, 0.095671, 0.048773,
			0.000000, -0.048773, -0.095671, -0.138893, -0.176777, -0.207867, -0.230970, -0.245196,
			-0.250000, -0.245196, -0.230970, -0.207867, -0.176777, -0.138893, -0.095671, -0.048773
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xb000;
		ctx.masterCutoff = 0x80;
		ctx.initSingleSynth(false, 0, 0);
	}

	SUBCASE("symmetric square") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000, 0.250000, 0.250000, 0.250000, 0.250000, 0.250000, 0.250000, 0.250000,
			0.250000, 0.250000, 0.250000, 0.250000, 0.250000, 0.250000, 0.250000, 0.250000,
			0.000000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000,
			-0.250000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xb000;
		ctx.masterCutoff = 0xf0;
		ctx.initSingleSynth(false, 0, 0);
	}

	SUBCASE("asymmetric square") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000, 0.250000, 0.250000, 0.250000, 0.250000, 0.250000, 0.250000, 0.250000,
			0.000000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000,
			-0.250000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000,
			-0.250000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000, -0.250000
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xb000;
		ctx.masterCutoff = 0xc0;
		ctx.initSingleSynth(false, 192, 0);
	}

	SUBCASE("symmetric resonant square") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000, 0.236627, 0.364626, 0.327616, 0.250000, 0.178826, 0.153612, 0.184733,
			0.250000, 0.309850, 0.331052, 0.304883, 0.250000, 0.199672, 0.181843, 0.203849,
			0.250000, 0.292321, 0.307313, 0.288808, 0.250000, 0.214413, 0.201806, 0.217366,
			0.250000, 0.279925, 0.290526, 0.277441, 0.250000, 0.224836, 0.215922, 0.165239,
			0.000000, -0.236304, -0.363391, -0.326365, -0.250000, -0.180727, -0.156693, -0.187161,
			-0.250000, -0.307003, -0.326780, -0.301709, -0.250000, -0.203093, -0.186819, -0.207450,
			-0.250000, -0.288599, -0.301990, -0.285014, -0.250000, -0.218238, -0.207219, -0.221188,
			-0.250000, -0.276136, -0.285204, -0.273709, -0.250000, -0.228493, -0.221032, -0.167022
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xa000;
		ctx.masterCutoff = 0xb0;
		ctx.initSingleSynth(false, 0, 24);
	}

	SUBCASE("asymmetric resonant square") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000, 0.236627, 0.364626, 0.327616, 0.250000, 0.178826, 0.153612, 0.184733,
			0.250000, 0.309850, 0.331052, 0.304883, 0.250000, 0.199672, 0.181843, 0.153701,
			0.000000, -0.236304, -0.363391, -0.326365, -0.250000, -0.180727, -0.156693, -0.187161,
			-0.250000, -0.307003, -0.326780, -0.301709, -0.250000, -0.203093, -0.186819, -0.207450,
			-0.250000, -0.288599, -0.301990, -0.285014, -0.250000, -0.218238, -0.207219, -0.221188,
			-0.250000, -0.276136, -0.285204, -0.273709, -0.250000, -0.228493, -0.221032, -0.230491,
			-0.250000, -0.267697, -0.273837, -0.266054, -0.250000, -0.235437, -0.230385, -0.236790,
			-0.250000, -0.261983, -0.266141, -0.260870, -0.250000, -0.240139, -0.236718, -0.172304
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xa000;
		ctx.masterCutoff = 0xb0;
		ctx.initSingleSynth(false, 192, 24);
	}

	SUBCASE("symmetric sawtooth") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000, 0.220296, 0.231185, 0.207850, 0.176747, 0.138886, 0.095672, 0.048773,
			0.000000, -0.048773, -0.095671, -0.138893, -0.176777, -0.207867, -0.230970, -0.219701,
			0.000000, 0.220291, 0.231182, 0.207850, 0.176748, 0.138886, 0.095672, 0.048773,
			0.000000, -0.048773, -0.095671, -0.138893, -0.176777, -0.207867, -0.230970, -0.219700
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xb000;
		ctx.masterCutoff = 0xa8;
		ctx.initSingleSynth(true, 0, 0);
	}

	SUBCASE("asymmetric sawtooth") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000, 0.220296, 0.231185, 0.207850, 0.176747, 0.138886, 0.095672, 0.043702,
			-0.000000, 0.043819, 0.095759, 0.138881, 0.176748, 0.207858, 0.230972, 0.245199,
			0.250000, 0.245196, 0.230970, 0.207867, 0.176777, 0.138893, 0.095671, 0.048773,
			0.000000, -0.048773, -0.095671, -0.138893, -0.176777, -0.207867, -0.230970, -0.21970
		};
		ctx.setExpectedSamples(EXPECTED_SAMPLES);
		ctx.masterPitch = 0xb000;
		ctx.masterCutoff = 0xa8;
		ctx.initSingleSynth(true, 192, 0);
	}

	SUBCASE("ring modulation") {
		static const FloatSample EXPECTED_SAMPLES[] = {
			0.000000, 0.023430, -0.027314, -0.045586, 0.088776, 0.009978, -0.135770, 0.077375,
			0.122290, -0.172582, -0.035072, 0.218686, -0.095208, -0.179530, 0.208842, 0.062590,
			-0.249770, 0.082996, 0.197064, -0.192420, -0.076823, 0.220287, -0.052502, -0.164495,
			0.132780, 0.065226, -0.137780, 0.020004, 0.085393, -0.050257, -0.023750, 0.023959,
			0.000000, 0.022729, -0.030676, -0.040579, 0.091506, -0.000122, -0.132761, 0.088955,
			0.110901, -0.179400, -0.017385, 0.215476, -0.112891, -0.165318, 0.219084, 0.041724,
			-0.247932, 0.102790, 0.183835, -0.203895, -0.057874, 0.220268, -0.069545, -0.155197,
			0.142293, 0.052597, -0.138777, 0.029884, 0.081382, -0.054559, -0.020012, 0.024311
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
			0.000000, 0.047934, 0.021459, 0.026985, 0.184447, 0.127827, 0.003122, 0.235973,
			0.299067, 0.020670, 0.172795, 0.439166, 0.135762, 0.059706, 0.454039, 0.311386,
			0.000230, 0.331792, 0.442260, 0.046815, 0.154147, 0.440767, 0.155365, 0.028758,
			0.309557, 0.223824, 0.001112, 0.137854, 0.181064, 0.022314, 0.025022, 0.048463,
			0.000000, -0.001775, -0.079449, -0.113150, -0.004165, -0.117971, -0.271654, -0.069644,
			-0.065876, -0.372652, -0.225252, -0.005005, -0.343861, -0.404553, -0.026112, -0.207072,
			-0.497932, -0.146006, -0.061361, -0.443130, -0.288844, -0.000213, -0.277413, -0.348450,
			-0.034484, -0.106002, -0.277669, -0.087965, -0.014289, -0.127130, -0.068785, -0.000193
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
			0.000000, 0.047301, 0.105126, 0.170305, 0.238719, -0.194330, -0.133685, -0.083874,
			-0.048683, -0.030794, -0.030023, -0.036897, -0.049482, -0.067387, -0.090078, -0.116897,
			-0.147078, -0.179776, -0.214079, -0.249045, 0.216281, 0.182834, 0.151510, 0.123151,
			0.099966, 0.084839, 0.078011, 0.076965, 0.077154, 0.073234, 0.060491, 0.036113,
			0.000000, 0.047294, 0.105072, 0.170130, 0.238332, -0.195022, -0.134755, -0.085361,
			-0.050578, -0.033037, -0.032513, -0.039549, -0.052203, -0.070070, -0.092607, -0.119150,
			-0.148936, -0.181122, -0.214808, -0.249065, 0.217040, 0.184421, 0.153946, 0.126406,
			0.103801, 0.088877, 0.081834, 0.080197, 0.079545, 0.074715, 0.061182, 0.036285
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
