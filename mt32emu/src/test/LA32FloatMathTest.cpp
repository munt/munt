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

#include <cfloat>

#include "../LA32FloatMath.h"

#include "Testing.h"

namespace MT32Emu {

namespace Test {

using namespace LA32FloatMath;
using ::std::fabs;
using ::std::sqrt;

namespace {

static const Bit32u SMALL_EXP2_LUT_BITS = 6;
static const Bit32u BIG_EXP2_LUT_BITS = 12;
static const Bit32u SMALL_SINE_LUT_BITS = 9;
static const Bit32u BIG_SINE_LUT_BITS = 12;

// With IEEE-754 floats, we normally don't go above 4 * FLT_EPSILON = ~4.768e-7.
// A higher value allows surprises, hence the tests should be more stable.
static const float EPS8 = 8 * FLT_EPSILON;
static const float MAX_ERROR = EPS8 < 1e-6f ? 1e-6f : EPS8;

struct SweepStats {
	Bit32u steps;

	float totalSquareError;
	float maxError;
	Bit32u maxErrorStep;
	float maxErrorValue;

	SweepStats() : steps(), totalSquareError(),
		maxError(), maxErrorStep(), maxErrorValue()
	{}

	void applyMeasurement(Bit32u step, float expected, float actual) {
		float absError = fabs(expected - actual);
		totalSquareError += absError * absError;
		if (maxError < absError) {
			maxError = absError;
			maxErrorStep = step;
			maxErrorValue = expected;
		}
	}
};

struct Exp2DataPoint {
	Bit32u fixedPointArg;
	float floatArg;
	float expected;
};

struct SineDataPoint {
	float arg;
	float expected;
};

template<class Table>
static void ensureTableInitialised() {
	static bool firstRun = true;
	if (firstRun) {
		firstRun = false;
		Table::init();
	}
}

} // namespace

TEST_CASE("LA32FloatMath table interpolation should provide for tolerable results") {
	SweepStats stats;
	float(*fixedPointExp)(Bit32u) = NULL;
	float(*floatFunc)(float) = NULL;
	float(*refFloatFunc)(float) = NULL;

	SUBCASE("using small exp2 LUT, quadratic interpolation, float") {
		typedef Exp2Table<QuadraticInterpolator, SMALL_EXP2_LUT_BITS> Table;
		ensureTableInitialised<Table>();
		stats.steps = 1 << (SMALL_EXP2_LUT_BITS + 4);
		floatFunc = Table::fexp2;
		refFloatFunc = EXP2F;
	}

	SUBCASE("using small exp2 LUT, quadratic interpolation, fixed point") {
		typedef Exp2Table<QuadraticInterpolator, SMALL_EXP2_LUT_BITS> Table;
		ensureTableInitialised<Table>();
		stats.steps = 1 << BIG_EXP2_LUT_BITS;
		fixedPointExp = Table::fexp2i;
		refFloatFunc = EXP2F;
	}

	SUBCASE("using small sine LUT, quadratic interpolation, float") {
		typedef SineTable<QuadraticInterpolator, SMALL_SINE_LUT_BITS> Table;
		ensureTableInitialised<Table>();
		stats.steps = 1 << (SMALL_SINE_LUT_BITS + 4);
		floatFunc = Table::fsin;
		refFloatFunc = VanillaMath::fsin;
	}

	SUBCASE("using sufficient exp2 LUT, linear interpolation, float") {
		typedef Exp2Table<LinearInterpolator, BIG_EXP2_LUT_BITS - 2> Table;
		ensureTableInitialised<Table>();
		stats.steps = 1 << (BIG_EXP2_LUT_BITS + 2);
		floatFunc = Table::fexp2;
		refFloatFunc = EXP2F;
	}

	SUBCASE("using bigger exp2 LUT, linear interpolation, fixed point") {
		typedef Exp2Table<LinearInterpolator, BIG_EXP2_LUT_BITS - 2> Table;
		ensureTableInitialised<Table>();
		stats.steps = 1 << BIG_EXP2_LUT_BITS;
		fixedPointExp = Table::fexp2i;
		refFloatFunc = EXP2F;
	}

	SUBCASE("using big sine LUT, linear interpolation, float") {
		typedef SineTable<LinearInterpolator, BIG_SINE_LUT_BITS> Table;
		ensureTableInitialised<Table>();
		stats.steps = 1 << (BIG_SINE_LUT_BITS + 3);
		floatFunc = Table::fsin;
		refFloatFunc = VanillaMath::fsin;
	}

	SUBCASE("using big exp2 LUT, linear interpolation, float") {
		typedef Exp2Table<LinearInterpolator, BIG_EXP2_LUT_BITS> Table;
		ensureTableInitialised<Table>();
		stats.steps = 1 << (BIG_EXP2_LUT_BITS + 3);
		floatFunc = Table::fexp2;
		refFloatFunc = EXP2F;
	}

	bool fixedPointFuncDefined = fixedPointExp != NULL;
	bool floatFuncDefined = floatFunc != NULL;
	bool refFuncDefined = refFloatFunc != NULL;
	REQUIRE(fixedPointFuncDefined != floatFuncDefined);
	REQUIRE(refFuncDefined);

	for (Bit32u step = 0; step < stats.steps; step++) {
		float x = float(step) / stats.steps;
		float actual = floatFuncDefined ? floatFunc(x) : fixedPointExp(step);
		stats.applyMeasurement(step, refFloatFunc(x), actual);
	}
	float stdDeviation = sqrt(stats.totalSquareError / stats.steps);
	INFO("\n\tstep #" << stats.maxErrorStep << "/" << stats.steps << "; stdDeviation=" << stdDeviation
		<< "; value=" << stats.maxErrorValue);
	CHECK(stats.maxError < MAX_ERROR);
}

TEST_CASE("LA32FloatMath::Exp2Table should give consistent results for arguments within supported range") {
	static const Exp2DataPoint dataPoints[] = {
		{  4096,        1.0f,      2.0f },
		{  8753,  2.1369629f, 4.398351f },
		{ 14963,  3.6530762f, 12.58014f },
		{ 55731, 13.6062012f, 12470.24f },
		{ 65535, 15.9997559f, 65524.91f }
	};

	float(*fexp2i)(Bit32u);
	float(*fexp2)(float);

	SUBCASE("using small LUT, quadratic interpolation") {
		typedef Exp2Table<QuadraticInterpolator, SMALL_EXP2_LUT_BITS> Table;
		ensureTableInitialised<Table>();
		fexp2i = Table::fexp2i;
		fexp2 = Table::fexp2;
	}

	SUBCASE("using sufficient LUT, linear interpolation") {
		typedef Exp2Table<LinearInterpolator, BIG_EXP2_LUT_BITS - 2> Table;
		ensureTableInitialised<Table>();
		fexp2i = Table::fexp2i;
		fexp2 = Table::fexp2;
	}

	SUBCASE("using big LUT, linear / no interpolation") {
		typedef Exp2Table<LinearInterpolator, BIG_EXP2_LUT_BITS> Table;
		ensureTableInitialised<Table>();
		fexp2i = Table::fexp2i;
		fexp2 = Table::fexp2;
	}

	SUBCASE("using extra big LUT, linear / no interpolation") {
		typedef Exp2Table<LinearInterpolator, BIG_EXP2_LUT_BITS + 1> Table;
		ensureTableInitialised<Table>();
		fexp2i = Table::fexp2i;
		fexp2 = Table::fexp2;
	}

	for (Bit32u pointIx = 0; pointIx < getArraySize(dataPoints); pointIx++) {
		Exp2DataPoint dataPoint = dataPoints[pointIx];
		float fixedPointActual = fexp2i(dataPoint.fixedPointArg);
		float floatActual = fexp2(dataPoint.floatArg);
		CAPTURE(pointIx);
		CAPTURE(dataPoint.fixedPointArg);
		CAPTURE(dataPoint.floatArg);
		CHECK(Approx(fixedPointActual).epsilon(MAX_ERROR) == dataPoint.expected);
		CHECK(Approx(floatActual).epsilon(MAX_ERROR) == dataPoint.expected);
	}
}

TEST_CASE("LA32FloatMath::SineTable should give consistent results for arguments within supported range") {
	static const SineDataPoint dataPoints[] = {
		{   0.25384f,  0.71558546f },
		{       0.5f,         1.0f },
		{   0.75139f,  0.70401225f },
		{       1.0f,         0.0f },
		{  1.398351f, -0.94944290f },
		{       1.5f,        -1.0f },
		{  1.801477f, -0.58402499f },
		{       2.0f,         0.0f },
		{  16.57193f,  0.97457617f },
		{ 314.15926f,  0.47974614f }
	};

	float(*fsin)(float);

	SUBCASE("using small LUT, quadratic interpolation") {
		typedef SineTable<QuadraticInterpolator, SMALL_SINE_LUT_BITS> Table;
		ensureTableInitialised<Table>();
		fsin = Table::fsin;
	}

	SUBCASE("using big LUT, linear interpolation") {
		typedef SineTable<LinearInterpolator, BIG_SINE_LUT_BITS> Table;
		ensureTableInitialised<Table>();
		fsin = Table::fsin;
	}

	for (Bit32u pointIx = 0; pointIx < getArraySize(dataPoints); pointIx++) {
		SineDataPoint dataPoint = dataPoints[pointIx];
		float actual = fsin(dataPoint.arg);
		CAPTURE(pointIx);
		CAPTURE(dataPoint.arg);
		CHECK(Approx(actual).epsilon(MAX_ERROR) == dataPoint.expected);
	}
}

#define MT32EMU_CHECK_CONSISTENCY(func, arg) \
	do { \
		float argHolder = float(arg); \
		float vanillaResult(VanillaMath::func(arg)); \
		float fasterMathResult = FasterMath::func(arg); \
		float absError = fasterMathResult - vanillaResult; \
		INFO("FasterMath::" << #func << "(" << argHolder << "); error=" << absError); \
		CHECK(Approx(fasterMathResult).epsilon(MAX_ERROR) == vanillaResult); \
	} while(false)

TEST_CASE("LA32FloatMath::FasterMath should be consistent with VanillaMath") {
	ensureTableInitialised<Exp2Table <LinearInterpolator, BIG_EXP2_LUT_BITS> >();
	ensureTableInitialised<SineTable <LinearInterpolator, BIG_SINE_LUT_BITS> >();
	typedef LA32FloatMath::FasterMath<LinearInterpolator, BIG_EXP2_LUT_BITS, BIG_SINE_LUT_BITS> FasterMath;

	MT32EMU_CHECK_CONSISTENCY(fsin, 1.234567f);

	MT32EMU_CHECK_CONSISTENCY(fcos, 0.0f);
	MT32EMU_CHECK_CONSISTENCY(fcos, 0.5f);
	MT32EMU_CHECK_CONSISTENCY(fcos, 1.0f);
	MT32EMU_CHECK_CONSISTENCY(fcos, 3.5f);
	MT32EMU_CHECK_CONSISTENCY(fcos, 1.234567f);

	MT32EMU_CHECK_CONSISTENCY(cosineLengthFactor, 128.0f);
	MT32EMU_CHECK_CONSISTENCY(cosineLengthFactor, 235.49671f);
	MT32EMU_CHECK_CONSISTENCY(cosineLengthFactor, 240.0f);

	MT32EMU_CHECK_CONSISTENCY(unlogPCMSample, 0);
	MT32EMU_CHECK_CONSISTENCY(unlogPCMSample, 8191);
	MT32EMU_CHECK_CONSISTENCY(unlogPCMSample, 21351);
	MT32EMU_CHECK_CONSISTENCY(unlogPCMSample, 32767);

	MT32EMU_CHECK_CONSISTENCY(resonanceAmpBase, 0);
	MT32EMU_CHECK_CONSISTENCY(resonanceAmpBase, 13);
	MT32EMU_CHECK_CONSISTENCY(resonanceAmpBase, 31);

	MT32EMU_CHECK_CONSISTENCY(pulseLengthFactor, 0);
	MT32EMU_CHECK_CONSISTENCY(pulseLengthFactor, 128);
	MT32EMU_CHECK_CONSISTENCY(pulseLengthFactor, 191);
	MT32EMU_CHECK_CONSISTENCY(pulseLengthFactor, 255);

	MT32EMU_CHECK_CONSISTENCY(linearAmpFactor, 0);
	MT32EMU_CHECK_CONSISTENCY(linearAmpFactor, 171);
	MT32EMU_CHECK_CONSISTENCY(linearAmpFactor, 33333333);
	MT32EMU_CHECK_CONSISTENCY(linearAmpFactor, 67117056);

	MT32EMU_CHECK_CONSISTENCY(frequency, 0);
	MT32EMU_CHECK_CONSISTENCY(frequency, 8191);
	MT32EMU_CHECK_CONSISTENCY(frequency, 33333);
	MT32EMU_CHECK_CONSISTENCY(frequency, 65535);

	MT32EMU_CHECK_CONSISTENCY(attenuationFactorBelowMiddleCutoff, 0.0f);
	MT32EMU_CHECK_CONSISTENCY(attenuationFactorBelowMiddleCutoff, 0.051f);
	MT32EMU_CHECK_CONSISTENCY(attenuationFactorBelowMiddleCutoff, 111.7777f);
	MT32EMU_CHECK_CONSISTENCY(attenuationFactorBelowMiddleCutoff, 128.0f);

	MT32EMU_CHECK_CONSISTENCY(unlogResonanceAmpFade, 0.0f);
	MT32EMU_CHECK_CONSISTENCY(unlogResonanceAmpFade, -1.2345678f);
	MT32EMU_CHECK_CONSISTENCY(unlogResonanceAmpFade, -29.9998f);
	MT32EMU_CHECK_CONSISTENCY(unlogResonanceAmpFade, -30.0f);
	MT32EMU_CHECK_CONSISTENCY(unlogResonanceAmpFade, -30.001f);
	MT32EMU_CHECK_CONSISTENCY(unlogResonanceAmpFade, -40.0f);
}

} // namespace Test

} // namespace MT32Emu
