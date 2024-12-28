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

#include "../Analog.h"
#include "../mmath.h"
#include "../Synth.h"

#include "Testing.h"

namespace MT32Emu {

namespace Test {

// In old doctest versions, defining subcases in a function didn't work.
#define createSubcasesForModes(mode, downsampleFactor) \
SUBCASE("Accurate Mode") { \
	mode = AnalogOutputMode_ACCURATE; \
	downsampleFactor = 2; \
} \
\
SUBCASE("Oversampled Mode") { \
	mode = AnalogOutputMode_OVERSAMPLED; \
	downsampleFactor = 1; \
}

static float circularFrequencyFactor(float freq, float sampleRate = SAMPLE_RATE) {
	return FLOAT_2PI * freq / sampleRate;
}

static IntSample absIntSampleDiff(IntSample sample1, IntSample sample2) {
	return sample1 < sample2 ? sample2 - sample1 : sample1 - sample2;
}

TEST_CASE("Analog should upsample and add mirror spectra in Accurate & Oversampled modes with Float samples") {
	bool oldGen;
	float freqL;
	float expectedAmpL;
	float expectedPhaseShiftL;
	// Main tone.
	float freqR1;
	float expectedAmpR1;
	float expectedPhaseShiftR1;
	// Aliased tone.
	float freqR2;
	float expectedAmpR2;
	float expectedPhaseShiftR2;
	AnalogOutputMode mode = AnalogOutputMode_DIGITAL_ONLY;
	Bit32u downsampleFactor = 0;

	SUBCASE("New-GEN: DC, 2048Hz") {
		// At low frequencies, the filter gain is close to unity and the aliasing is not observable.
		oldGen = false;
		freqL = 0.0f;
		expectedAmpL = 1.0f;
		expectedPhaseShiftL = 0.0f;
		freqR1 = 2048.0f;
		expectedAmpR1 = 1.0180677f;
		expectedPhaseShiftR1 = -0.3820356f;
		freqR2 = 0;
		expectedAmpR2 = 0;
		expectedPhaseShiftR2 = 0;

		createSubcasesForModes(mode, downsampleFactor);
	}

	SUBCASE("New-GEN: Nyquist, 13kHz") {
		// The filter boosts high frequencies and produces an audible aliasing distortion.
		oldGen = false;
		freqL = 16000.0f;
		freqR1 = 13000.0f;
		freqR2 = 19000.0f;
		// The amplitude at the Nyquist frequency is doubled due to aliasing.
		expectedAmpL = 2 * 1.453242f;
		expectedPhaseShiftL = 1.2780396f;
		expectedAmpR1 = 1.833776f;
		expectedPhaseShiftR1 = 2.797162f;
		// Attenuation of the mirror spectra is insignificant.
		expectedAmpR2 = 0.8274725f;
		expectedPhaseShiftR2 = 2.831521f;

		createSubcasesForModes(mode, downsampleFactor);
	}

	SUBCASE("Old-GEN: DC, 2048Hz") {
		oldGen = true;
		freqL = 0.0f;
		expectedAmpL = 1.0f;
		expectedPhaseShiftL = 0.0f;
		freqR1 = 2048.0f;
		expectedAmpR1 = 1.0220286f;
		expectedPhaseShiftR1 = -0.40874957f;
		freqR2 = 0;
		expectedAmpR2 = 0;
		expectedPhaseShiftR2 = 0;

		createSubcasesForModes(mode, downsampleFactor);
	}

	SUBCASE("Old-GEN: Nyquist, 11.65kHz") {
		oldGen = true;
		freqL = 16000.0f;
		freqR1 = 11650.0f;
		freqR2 = 20350.0f;
		expectedAmpL = 2 * 0.8727288f;
		expectedPhaseShiftL = 0.7070053f;
		expectedAmpR1 = 1.69966187f;
		expectedPhaseShiftR1 = 2.986075657f;
		expectedAmpR2 = 0.221111031f;
		expectedPhaseShiftR2 = 1.857252537f;

		createSubcasesForModes(mode, downsampleFactor);
	}

	Analog *analog = Analog::createAnalog(mode, oldGen, RendererType_FLOAT);
	REQUIRE(analog != NULL_PTR);
	analog->setSynthOutputGain(1);

	const Bit32u inFrameCount = 64;
	const Bit32u maxOutFrameCount = 3 * inFrameCount;
	const Bit32u outFrameCount = maxOutFrameCount / downsampleFactor;
	REQUIRE(analog->getDACStreamsLength(outFrameCount) == inFrameCount);
	float emptyBuffer[inFrameCount];
	Synth::muteSampleBuffer(emptyBuffer, inFrameCount);
	float inBufferL[inFrameCount];
	float inBufferR[inFrameCount];
	float wfInL = circularFrequencyFactor(freqL);
	float wfInR = circularFrequencyFactor(freqR1);
	for (Bit32u i = 0; i < inFrameCount; i++) {
		inBufferL[i] = cos(wfInL * i);
		inBufferR[i] = sin(wfInR * i);
	}

	float outBuffer[2 * maxOutFrameCount];
	REQUIRE(analog->process(outBuffer, inBufferL, inBufferR, emptyBuffer, emptyBuffer, emptyBuffer, emptyBuffer, outFrameCount));

	const float outSampleRate = 3.0f * SAMPLE_RATE / downsampleFactor;
	CHECK(outSampleRate == analog->getOutputSampleRate());
	const double epsilon = 4e-5;
	float wfOutL = circularFrequencyFactor(freqL, outSampleRate);
	float wfOutR1 = circularFrequencyFactor(freqR1, outSampleRate);
	float wfOutR2 = circularFrequencyFactor(freqR2, outSampleRate);
	// The LPF warm-up takes 48 or 24 samples, depending on the mode. We skip the maximum for simplicity.
	for (Bit32u i = 48; i < outFrameCount; i++) {
		CAPTURE(i);
		Approx actualSampleL = Approx(outBuffer[2 * i]);
		Approx actualSampleR = Approx(outBuffer[2 * i + 1]);
		float expectedSampleL = expectedAmpL * cos(wfOutL * i + expectedPhaseShiftL);
		float expectedSampleR = expectedAmpR1 * sin(wfOutR1 * i + expectedPhaseShiftR1)
			+ expectedAmpR2 * sin(wfOutR2 * i + expectedPhaseShiftR2);
		CHECK(actualSampleL.epsilon(epsilon) == expectedSampleL);
		CHECK(actualSampleR.epsilon(epsilon) == expectedSampleR);
	}

	delete analog;
}

TEST_CASE("Analog should upsample and add mirror spectra in Accurate & Oversampled modes with 16-bit samples") {
	bool oldGen;
	float freqL;
	float expectedAmpL;
	float expectedPhaseShiftL;
	// Main tone.
	float freqR1;
	float expectedAmpR1;
	float expectedPhaseShiftR1;
	// Aliased tone.
	float freqR2;
	float expectedAmpR2;
	float expectedPhaseShiftR2;
	AnalogOutputMode mode = AnalogOutputMode_DIGITAL_ONLY;
	Bit32u downsampleFactor = 0;

	SUBCASE("New-GEN") {
		// The filter boosts high frequencies and produces an audible aliasing distortion.
		oldGen = false;
		freqL = 16000.0f;
		freqR1 = 13000.0f;
		freqR2 = 19000.0f;
		// The amplitude at the Nyquist frequency is doubled due to aliasing.
		expectedAmpL = 2 * 1.453242f;
		expectedPhaseShiftL = 1.2780396f;
		expectedAmpR1 = 1.833776f;
		expectedPhaseShiftR1 = 2.797162f;
		// Attenuation of the mirror spectra is insignificant.
		expectedAmpR2 = 0.8274725f;
		expectedPhaseShiftR2 = 2.831521f;

		createSubcasesForModes(mode, downsampleFactor);
	}

	SUBCASE("Old-GEN") {
		oldGen = true;
		freqL = 16000.0f;
		freqR1 = 11650.0f;
		freqR2 = 20350.0f;
		expectedAmpL = 2 * 0.8727288f;
		expectedPhaseShiftL = 0.7070053f;
		expectedAmpR1 = 1.69966187f;
		expectedPhaseShiftR1 = 2.986075657f;
		expectedAmpR2 = 0.221111031f;
		expectedPhaseShiftR2 = 1.857252537f;

		createSubcasesForModes(mode, downsampleFactor);
	}

	Analog *analog = Analog::createAnalog(mode, oldGen, RendererType_BIT16S);
	REQUIRE(analog != NULL_PTR);
	analog->setSynthOutputGain(1);

	const float scaleFactor = 1 << 14;
	const Bit32u inFrameCount = 64;
	const Bit32u maxOutFrameCount = 3 * inFrameCount;
	const Bit32u outFrameCount = maxOutFrameCount / downsampleFactor;
	REQUIRE(analog->getDACStreamsLength(outFrameCount) == inFrameCount);
	IntSample emptyBuffer[inFrameCount];
	Synth::muteSampleBuffer(emptyBuffer, inFrameCount);
	IntSample inBufferL[inFrameCount];
	IntSample inBufferR[inFrameCount];
	float wfInL = circularFrequencyFactor(freqL);
	float wfInR = circularFrequencyFactor(freqR1);
	for (Bit32u i = 0; i < inFrameCount; i++) {
		inBufferL[i] = IntSample(cos(wfInL * i) * scaleFactor);
		inBufferR[i] = IntSample(sin(wfInR * i) * scaleFactor);
	}

	IntSample outBuffer[2 * maxOutFrameCount];
	REQUIRE(analog->process(outBuffer, inBufferL, inBufferR, emptyBuffer, emptyBuffer, emptyBuffer, emptyBuffer, outFrameCount));

	const float outSampleRate = 3.0f * SAMPLE_RATE / downsampleFactor;
	CHECK(outSampleRate == analog->getOutputSampleRate());
	float wfOutL = circularFrequencyFactor(freqL, outSampleRate);
	float wfOutR1 = circularFrequencyFactor(freqR1, outSampleRate);
	float wfOutR2 = circularFrequencyFactor(freqR2, outSampleRate);
	// The LPF warm-up takes 48 or 24 samples, depending on the mode. We skip the maximum for simplicity.
	for (Bit32u i = 48; i < outFrameCount; i++) {
		CAPTURE(i);

		{
			IntSample actualSampleL = outBuffer[2 * i];
			IntSample expectedSampleL = IntSample(scaleFactor * expectedAmpL * cos(wfOutL * i + expectedPhaseShiftL));
			CAPTURE(actualSampleL);
			CAPTURE(expectedSampleL);
			CHECK(absIntSampleDiff(actualSampleL, expectedSampleL) < 4);
		}

		{
			IntSample actualSampleR = outBuffer[2 * i + 1];
			IntSample expectedSampleR = IntSample(scaleFactor * (expectedAmpR1 * sin(wfOutR1 * i + expectedPhaseShiftR1)
				+ expectedAmpR2 * sin(wfOutR2 * i + expectedPhaseShiftR2)));
			CAPTURE(actualSampleR);
			CAPTURE(expectedSampleR);
			CHECK(absIntSampleDiff(actualSampleR, expectedSampleR) < 4);
		}
	}

	delete analog;
}

TEST_CASE("Analog should only boost high frequencies in Coarse mode with Float samples") {
	bool oldGen;
	float freqL;
	float freqR;
	float expectedAmpL;
	float expectedAmpR;
	float expectedPhaseShiftL;
	float expectedPhaseShiftR;

	// The expected filter gain and phase shift come from the DFT.
	SUBCASE("New-GEN: DC, 2048Hz") {
		oldGen = false;
		freqL = 0.0f;
		freqR = 2048.0f;
		expectedAmpL = 0.9949726f;
		expectedAmpR = 1.0227465f;
		expectedPhaseShiftL = 0.0f;
		expectedPhaseShiftR = 0.10630286f;
	}

	SUBCASE("New-GEN: Nyquist, 13kHz") {
		oldGen = false;
		freqL = 16000.0f;
		freqR = 13000.0f;
		expectedAmpL = 1.5318551f;
		expectedAmpR = 1.8481209f;
		expectedPhaseShiftL = 0.0f;
		expectedPhaseShiftR = 0.0941751f;
	}

	SUBCASE("Old-GEN: DC, 2048Hz") {
		oldGen = true;
		freqL = 0.0f;
		freqR = 2048.0f;
		expectedAmpL = 0.9969666f;
		expectedAmpR = 1.024883f;
		expectedPhaseShiftL = 0.0f;
		expectedPhaseShiftR = 0.1038734f;
	}

	SUBCASE("Old-GEN: Nyquist, 11.75kHz") {
		oldGen = true;
		freqL = 16000.0f;
		freqR = 11750.0f;
		expectedAmpL = 0.9480253f;
		expectedAmpR = 1.6941649f;
		expectedPhaseShiftL = 0.0f;
		expectedPhaseShiftR = -0.004912923f;
	}

	Analog *analog = Analog::createAnalog(AnalogOutputMode_COARSE, oldGen, RendererType_FLOAT);
	REQUIRE(analog != NULL_PTR);
	analog->setSynthOutputGain(1);
	CHECK(SAMPLE_RATE == analog->getOutputSampleRate());

	const Bit32u frameCount = 128;
	REQUIRE(analog->getDACStreamsLength(frameCount) == frameCount);
	float emptyBuffer[frameCount];
	Synth::muteSampleBuffer(emptyBuffer, frameCount);
	float inBufferL[frameCount];
	float inBufferR[frameCount];
	float wfL = circularFrequencyFactor(freqL);
	float wfR = circularFrequencyFactor(freqR);
	for (Bit32u i = 0; i < frameCount; i++) {
		inBufferL[i] = cos(wfL * i);
		inBufferR[i] = sin(wfR * i);
	}

	float outBuffer[2 * frameCount];
	REQUIRE(analog->process(outBuffer, inBufferL, inBufferR, emptyBuffer, emptyBuffer, emptyBuffer, emptyBuffer, frameCount));

	const double epsilon = 3e-5;

	// The first 8 samples aren't interesting as the LPF is still warming up.
	for (Bit32u i = 8; i < frameCount; i++) {
		CAPTURE(i);
		Approx actualSampleL = Approx(outBuffer[2 * i]).epsilon(epsilon);
		Approx actualSampleR = Approx(outBuffer[2 * i + 1]).epsilon(epsilon);
		CHECK(actualSampleL == expectedAmpL * cos(wfL * i + expectedPhaseShiftL));
		CHECK(actualSampleR == expectedAmpR * sin(wfR * i + expectedPhaseShiftR));
	}

	delete analog;
}

TEST_CASE("Analog should only boost high frequencies in Coarse mode with 16-bit samples") {
	bool oldGen;
	float freqL;
	float freqR;
	float expectedAmpL;
	float expectedAmpR;
	float expectedPhaseShiftL;
	float expectedPhaseShiftR;

	// The expected filter gain and phase shift come from the DFT.
	SUBCASE("New-GEN: DC, 2048Hz") {
		oldGen = false;
		freqL = 0.0f;
		freqR = 2048.0f;
		expectedAmpL = 0.99493f;
		expectedAmpR = 1.02283f;
		expectedPhaseShiftL = 0.0f;
		expectedPhaseShiftR = 0.10632f;
	}

	SUBCASE("New-GEN: Nyquist, 13kHz") {
		oldGen = false;
		freqL = 16000.0f;
		freqR = 13000.0f;
		expectedAmpL = 1.5318f;
		expectedAmpR = 1.8482f;
		expectedPhaseShiftL = 0.0f;
		expectedPhaseShiftR = 0.094175f;
	}

	SUBCASE("Old-GEN: DC, 2048Hz") {
		oldGen = true;
		freqL = 0.0f;
		freqR = 2048.0f;
		expectedAmpL = 0.99695f;
		expectedAmpR = 1.02490f;
		expectedPhaseShiftL = 0.0f;
		expectedPhaseShiftR = 0.10385f;
	}

	SUBCASE("Old-GEN: Nyquist, 11.75kHz") {
		oldGen = true;
		freqL = 16000.0f;
		freqR = 11750.0f;
		expectedAmpL = 0.9480f;
		expectedAmpR = 1.6942f;
		expectedPhaseShiftL = 0.0f;
		expectedPhaseShiftR = -0.0049209f;
	}

	Analog *analog = Analog::createAnalog(AnalogOutputMode_COARSE, oldGen, RendererType_BIT16S);
	REQUIRE(analog != NULL_PTR);
	analog->setSynthOutputGain(1);
	CHECK(SAMPLE_RATE == analog->getOutputSampleRate());

	const float scaleFactor = 1 << 14;
	const Bit32u frameCount = 128;
	REQUIRE(analog->getDACStreamsLength(frameCount) == frameCount);
	IntSample emptyBuffer[frameCount];
	Synth::muteSampleBuffer(emptyBuffer, frameCount);
	IntSample inBufferL[frameCount];
	IntSample inBufferR[frameCount];
	float wfL = circularFrequencyFactor(freqL);
	float wfR = circularFrequencyFactor(freqR);
	for (Bit32u i = 0; i < frameCount; i++) {
		inBufferL[i] = IntSample(cos(wfL * i) * scaleFactor);
		inBufferR[i] = IntSample(sin(wfR * i) * scaleFactor);
	}

	IntSample outBuffer[2 * frameCount];
	REQUIRE(analog->process(outBuffer, inBufferL, inBufferR, emptyBuffer, emptyBuffer, emptyBuffer, emptyBuffer, frameCount));

	// The first 8 samples aren't interesting as the LPF is still warming up.
	for (Bit32u i = 8; i < frameCount; i++) {
		CAPTURE(i);

		{
			IntSample actualSampleL = outBuffer[2 * i];
			IntSample expectedSampleL = IntSample(scaleFactor * expectedAmpL * cos(wfL * i + expectedPhaseShiftL));
			CAPTURE(actualSampleL);
			CAPTURE(expectedSampleL);
			CHECK(absIntSampleDiff(actualSampleL, expectedSampleL) < 3);
		}

		{
			IntSample actualSampleR = outBuffer[2 * i + 1];
			IntSample expectedSampleR = IntSample(scaleFactor * expectedAmpR * sin(wfR * i + expectedPhaseShiftR));
			CAPTURE(actualSampleR);
			CAPTURE(expectedSampleR);
			CHECK(absIntSampleDiff(actualSampleR, expectedSampleR) < 3);
		}
	}

	delete analog;
}

TEST_CASE("Analog should only mix input channels in Digital-Only mode with Float samples") {
	const Bit32u frameCount = 4;
	const float inBufferNonReverbL[frameCount] = { 0.100f, 0.1f, 0.0f, 0.0f };
	const float inBufferReverbDryL[frameCount] = { 0.400f, 0.0f, 0.0f, 0.0f };
	const float inBufferReverbWetL[frameCount] = { 0.700f, 0.0f, 0.1f, 0.0f };

	const float inBufferNonReverbR[frameCount] = { 0.110f, 0.0f,  0.1f, 1.0f };
	const float inBufferReverbDryR[frameCount] = { 0.310f, 0.1f, -0.1f, 1.0f };
	const float inBufferReverbWetR[frameCount] = { 0.013f, 0.0f,  0.1f, 1.0f };

	const float expectedOutBufferOldGen[2 * frameCount] = { 1.2f, 0.433f, 0.1f, 0.1f, 0.1f, 0.1f, 0.0f, 3.0f };
	const float expectedOutBufferNewGen[2 * frameCount] = { 0.976f, 0.42884f, 0.1f, 0.1f, 0.068f, 0.068f, 0.0f, 2.68f };

	bool oldGenReverbCompatibilityMode;
	const float *expectedOutBuffer;

	SUBCASE("Old-GEN") {
		oldGenReverbCompatibilityMode = true;
		expectedOutBuffer = expectedOutBufferOldGen;
	}

	SUBCASE("New-GEN") {
		oldGenReverbCompatibilityMode = false;
		expectedOutBuffer = expectedOutBufferNewGen;
	}

	Analog *analog = Analog::createAnalog(AnalogOutputMode_DIGITAL_ONLY, oldGenReverbCompatibilityMode, RendererType_FLOAT);
	REQUIRE(analog != NULL_PTR);
	analog->setSynthOutputGain(1);
	analog->setReverbOutputGain(1, oldGenReverbCompatibilityMode);
	CHECK(SAMPLE_RATE == analog->getOutputSampleRate());
	REQUIRE(analog->getDACStreamsLength(frameCount) == frameCount);

	float outBuffer[2 * frameCount];
	REQUIRE(analog->process(outBuffer, inBufferNonReverbL, inBufferNonReverbR, inBufferReverbDryL, inBufferReverbDryR,
		inBufferReverbWetL, inBufferReverbWetR, frameCount));

	for (Bit32u i = 0; i < 2 * frameCount; i++) {
		CAPTURE(i);
		CHECK(expectedOutBuffer[i] == Approx(outBuffer[i]));
	}

	delete analog;
}

TEST_CASE("Analog should only mix input channels in Digital-Only mode with 16-bit samples") {
	const Bit32u frameCount = 4;
	const IntSample inBufferNonReverbL[frameCount] = { 12000, 16384,     0,     0 };
	const IntSample inBufferReverbDryL[frameCount] = { 16384,     0, 16384,     0 };
	const IntSample inBufferReverbWetL[frameCount] = { 3152,     0,     0, 16384 };

	const IntSample inBufferNonReverbR[frameCount] = { -10000,   -100,  32767, 0 };
	const IntSample inBufferReverbDryR[frameCount] = { 32767, -32768, -32767, 0 };
	const IntSample inBufferReverbWetR[frameCount] = { 100,    100,      0, 0 };

	const IntSample expectedOutBufferOldGen[2 * frameCount] = { 31536, 22867, 16384, -32768, 16384, 0, 16384, 0 };
	const IntSample expectedOutBufferNewGen[2 * frameCount] = { 30526, 22834, 16384, -32768, 16384, 0, 11136, 0 };

	bool oldGenReverbCompatibilityMode;
	const IntSample *expectedOutBuffer;

	SUBCASE("Old-GEN") {
		oldGenReverbCompatibilityMode = true;
		expectedOutBuffer = expectedOutBufferOldGen;
	}

	SUBCASE("New-GEN") {
		oldGenReverbCompatibilityMode = false;
		expectedOutBuffer = expectedOutBufferNewGen;
	}

	Analog *analog = Analog::createAnalog(AnalogOutputMode_DIGITAL_ONLY, oldGenReverbCompatibilityMode, RendererType_BIT16S);
	REQUIRE(analog != NULL_PTR);
	analog->setSynthOutputGain(1);
	analog->setReverbOutputGain(1, oldGenReverbCompatibilityMode);
	CHECK(SAMPLE_RATE == analog->getOutputSampleRate());
	REQUIRE(analog->getDACStreamsLength(frameCount) == frameCount);

	IntSample outBuffer[2 * frameCount];
	REQUIRE(analog->process(outBuffer, inBufferNonReverbL, inBufferNonReverbR, inBufferReverbDryL, inBufferReverbDryR,
		inBufferReverbWetL, inBufferReverbWetR, frameCount));

	for (Bit32u i = 0; i < 2 * frameCount; i++) {
		CAPTURE(i);
		CHECK(expectedOutBuffer[i] == outBuffer[i]);
	}

	delete analog;
}

TEST_CASE("Analog should only process data buffers that match RendererType") {
	const IntSample emptyBufferInt[] = { 0 };
	const float emptyBufferFloat[] = { 0 };
	Analog *analog = NULL;

	SUBCASE("16-bit samples") {
		analog = Analog::createAnalog(AnalogOutputMode_ACCURATE, false, RendererType_BIT16S);
		REQUIRE(analog->process(NULL, emptyBufferInt, emptyBufferInt, emptyBufferInt, emptyBufferInt,
			emptyBufferInt, emptyBufferInt, 1));
		REQUIRE_FALSE(analog->process(NULL, emptyBufferFloat, emptyBufferFloat, emptyBufferFloat, emptyBufferFloat,
			emptyBufferFloat, emptyBufferFloat, 1));
	}

	SUBCASE("Float samples") {
		analog = Analog::createAnalog(AnalogOutputMode_ACCURATE, false, RendererType_FLOAT);
		REQUIRE_FALSE(analog->process(NULL, emptyBufferInt, emptyBufferInt, emptyBufferInt, emptyBufferInt,
			emptyBufferInt, emptyBufferInt, 1));
		REQUIRE(analog->process(NULL, emptyBufferFloat, emptyBufferFloat, emptyBufferFloat, emptyBufferFloat,
			emptyBufferFloat, emptyBufferFloat, 1));
	}

	delete analog;
}

} // namespace Test

} // namespace MT32Emu
