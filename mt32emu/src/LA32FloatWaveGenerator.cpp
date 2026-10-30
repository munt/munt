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

#include <cstddef>

#include "internals.h"

#include "LA32FloatWaveGenerator.h"
#include "mmath.h"
#include "Tables.h"

namespace MT32Emu {

static const float MIDDLE_CUTOFF_VALUE = 128.0f;
static const float RESONANCE_DECAY_THRESHOLD_CUTOFF_VALUE = 144.0f;

static const Bit8u *resAmpDecayFactors;

#if MT32EMU_FASTER_MATH

#if MT32EMU_FASTER_MATH > 1
static const Bit32u FEXP2_TABLE_SIZE_M1_LOG2 = 6;
#else
// We don't need that much with the linear interpolation: 10 would be enough.
// However, picking the table size this big allows fexp2i to be simplified a lot leading to a considerable speedup.
static const Bit32u FEXP2_TABLE_SIZE_M1_LOG2 = 12;
#endif

// The table includes one more entry for 2 ^ 1 = 2 to simplify the rounding logic.
static const Bit32u FEXP2_TABLE_SIZE_M1 = 1 << FEXP2_TABLE_SIZE_M1_LOG2;
static const Bit32u FEXP2_TABLE_SIZE = FEXP2_TABLE_SIZE_M1 + 1;
static const Bit32u FEXP2I_FRAC_MASK = 4095;
static const float FEXP2_SCALE_FACTOR = FLOAT_LN_2 / FEXP2_TABLE_SIZE_M1;
#if MT32EMU_FASTER_MATH > 1
static const float FEXP2I_SCALE_FACTOR = 1.0f / 4096.0f;
#endif
static float FEXP2_TABLE[FEXP2_TABLE_SIZE];

// The table covers the entire sine period to simplify the computational logic.
// Even an attempt to halve the table by taking advantage of the sine's antisymmetry well degrades the performance (either using
// simple conditions or various dirty tricks for setting the sign bit directly). The tests showed numbers close to those achieved
// with the quadratic interpolation, but the table is already 8 times shorter in the latter case.
#if MT32EMU_FASTER_MATH > 1
static const Bit32u FSIN_TABLE_SIZE_LOG2 = 9;
#else
static const Bit32u FSIN_TABLE_SIZE_LOG2 = 12;
#endif

static const Bit32u FSIN_TABLE_SIZE = 1 << FSIN_TABLE_SIZE_LOG2;
static const Bit32u FSIN_TABLE_MASK = FSIN_TABLE_SIZE - 1;
static const Bit32u FSIN_TABLE_PI_IX = FSIN_TABLE_SIZE >> 1;
static const Bit32u FSIN_TABLE_HALF_PI_IX = FSIN_TABLE_SIZE >> 2;
static const float FSIN_SCALE_FACTOR = FLOAT_2PI / FSIN_TABLE_SIZE;
static float FSIN_TABLE[FSIN_TABLE_SIZE];

// This function takes the parameter in the LA32 native fixed-point logarithmic format with 12-bit fractional part.
// Splitting the integer argument is more efficient, yet the 12-bit fractions map 1:1 to the "big" LUT, so we can look up directly.
static inline float fexp2i(Bit32u x) {
	Bit32u upScale = x >> 12;

#if MT32EMU_FASTER_MATH > 1
	float scaledFracX = (x & FEXP2I_FRAC_MASK) * FEXP2_TABLE_SIZE_M1 * FEXP2I_SCALE_FACTOR;
	Bit32u tableIx = Bit32u(scaledFracX + 0.5f);
	float approxExp2 = FEXP2_TABLE[tableIx];
	float delta = (scaledFracX - tableIx) * FEXP2_SCALE_FACTOR;
	float mantissa = approxExp2 + (approxExp2 + 0.5f * approxExp2 * delta) * delta;
#else
	float mantissa = FEXP2_TABLE[x & FEXP2I_FRAC_MASK];
#endif

	return mantissa * (1 << upScale);
}

// Here we cut some corners to avoid standard math library calls each of which significantly degrades the performance in most cases.
// First limitation: the parameter must never be negative. To ensure this, we shake the related formulae.
// Next thing: the maximum value of the argument is also limited by 30.0 to avoid the integer overflow when scaling the result.
static float fexp2(float x) {
	Bit32u upScale = Bit32u(x);
	float fracX = x - upScale;
	float scaledFracX = fracX * FEXP2_TABLE_SIZE_M1;
	Bit32u tableIx = Bit32u(scaledFracX + 0.5f);
	float approxExp2 = FEXP2_TABLE[tableIx];
	float delta = (scaledFracX - tableIx) * FEXP2_SCALE_FACTOR;

#if MT32EMU_FASTER_MATH > 1
	float mantissa = approxExp2 + (approxExp2 + 0.5f * approxExp2 * delta) * delta;
#else
	float mantissa = approxExp2 + delta * approxExp2;
#endif

	return mantissa * (1 << upScale);
}

// The first cut corner: the argument is scaled by PI, meaning a frequency relative to the Nyquist frequency.
// In 100% of our call sites we had to multiply the argument by PI just to revert that here. Additionally, this also _reduces_
// the computation error for bigger arguments since the irrational factor PI isn't involved.
//
// The second cut corner: the argument must never be negative nor exceed 2^20. That's always the case with our usages. This makes
// the argument reduction unnecessary as a separate step.
static float fsin(float x) {
	float scaledX = x * FSIN_TABLE_PI_IX;
	Bit32u sinIx = Bit32u(scaledX + 0.5f);
	float approxSin = FSIN_TABLE[sinIx & FSIN_TABLE_MASK];
	Bit32u cosIx = sinIx + FSIN_TABLE_HALF_PI_IX;
	float approxCos = FSIN_TABLE[cosIx & FSIN_TABLE_MASK];
	float delta = (scaledX - sinIx) * FSIN_SCALE_FACTOR;

#if MT32EMU_FASTER_MATH > 1
	float res = approxSin + (approxCos - 0.5f * approxSin * delta) * delta;
#else
	float res = approxSin + approxCos * delta;
#endif

	return res;
}

static inline float fcos(float x) {
	return fsin(x + 0.5f);
}

#else // #if MT32EMU_FASTER_MATH

static inline float fsin(float x) {
	return sin(FLOAT_PI * x);
}

static inline float fcos(float x) {
	return cos(FLOAT_PI * x);
}

#endif // #if MT32EMU_FASTER_MATH

void LA32FloatWaveGenerator::updateWaveFormParameters() {
	cosineLen = 0.5f * waveLen;
	if (cutoff > MIDDLE_CUTOFF_VALUE) {
#if MT32EMU_FASTER_MATH
		static const float cosineLenFactor = 1.0f / 256.0f; // EXP2F(-MIDDLE_CUTOFF_VALUE / 16.0f)
		cosineLen *= fexp2((2.0f * MIDDLE_CUTOFF_VALUE - cutoff) / 16.0f) * cosineLenFactor;
#else
		cosineLen *= EXP2F((cutoff - MIDDLE_CUTOFF_VALUE) / -16.0f); // found from sample analysis
#endif
	}
}

float LA32FloatWaveGenerator::getPCMSample(unsigned int position) {
	if (position >= pcmWaveLength) {
		if (!pcmWaveLooped) {
			return 0;
		}
		position = position % pcmWaveLength;
	}
	Bit16s pcmSample = pcmWaveAddress[position];
	float sampleValue;
#if MT32EMU_FASTER_MATH
	{
		static const float pcmSampleFactor = 1.51609812e-5f; // EXP2F(-32787.0f / 2048.0f)
		sampleValue = fexp2i((pcmSample & 32767) << 1) * pcmSampleFactor;
	}
#else
	sampleValue = EXP2F(((pcmSample & 32767) - 32787.0f) / 2048.0f);
#endif
	return ((pcmSample & 32768) == 0) ? sampleValue : -sampleValue;
}

void LA32FloatWaveGenerator::initSynth(const bool useSawtoothWaveform, const Bit8u usePulseWidth, const Bit8u useResonance) {
	sawtoothWaveform = useSawtoothWaveform;
	pulseWidth = usePulseWidth;
	resonance = useResonance;

#if MT32EMU_FASTER_MATH
	{
		static const float resonanceAmpFactor = 1.0f / 128.0f;
		resAmpBase = fexp2i(resonance << 10) * resonanceAmpFactor;
	}
#else
	resAmpBase = EXP2F(1.0f - (32 - resonance) / 4.0f);
#endif
	resAmp = resAmpBase;

	if (pulseWidth > 128) {
#if MT32EMU_FASTER_MATH
		pulseLenFactor = fexp2i((256 - pulseWidth) << 6) * 0.125f;
#else
		pulseLenFactor = EXP2F((64 - pulseWidth) / 64.0f);
#endif
	} else {
		pulseLenFactor = 0.5f;
	}

	cutoff = 0.0f;
	freq = 0.0f;
	waveLen = 0.0f;
	wavePos = 0.0f;

	pcmWaveAddress = NULL;
	active = true;
}

void LA32FloatWaveGenerator::initPCM(const Bit16s * const usePCMWaveAddress, const Bit32u usePCMWaveLength, const bool usePCMWaveLooped, const bool usePCMWaveInterpolated) {
	pcmWaveAddress = usePCMWaveAddress;
	pcmWaveLength = usePCMWaveLength;
	pcmWaveLooped = usePCMWaveLooped;
	pcmWaveInterpolated = usePCMWaveInterpolated;

	pcmPosition = 0.0f;
	active = true;
}

void LA32FloatWaveGenerator::setAmp(const Bit32u amp) {
	// See Partial::getAmpValue for more details
#if MT32EMU_FASTER_MATH
	{
		static const float logAmpScaleFactor = 1.0f / 1024.0f / 4096.0f;
		static const float linearAmpFactor = 1.5238139e-5f; // EXP2F(-67117056.0f * logAmpScaleFactor)
		ampFactor = fexp2((67117056 - amp) * logAmpScaleFactor) * linearAmpFactor;
	}
#else
	ampFactor = EXP2F(amp / -1024.0f / 4096.0f);
#endif
}

void LA32FloatWaveGenerator::setPitch(const Bit16u pitch) {
	float lastFreq = freq;
#if MT32EMU_FASTER_MATH
	{
		static const float freqFactor = SAMPLE_RATE / 65536.0f;
		freq = fexp2i(pitch) * freqFactor;
	}
#else
	freq = EXP2F(pitch / 4096.0f - 16.0f) * SAMPLE_RATE;
#endif

	if (isPCMWave()) {
		pcmPositionDelta = freq * 2048.0f / SAMPLE_RATE;
	} else {
		waveLen = SAMPLE_RATE / freq;
		wavePos *= lastFreq / freq;
		updateWaveFormParameters();
	}
}

void LA32FloatWaveGenerator::setCutoff(const Bit32u cutoffVal) {
	cutoff = cutoffVal / 262144.0f;

	// Correct resAmp for cutoff in range 50..66
	if ((cutoff >= MIDDLE_CUTOFF_VALUE) && (cutoff < RESONANCE_DECAY_THRESHOLD_CUTOFF_VALUE)) {
		resAmp = resAmpBase * fsin((cutoff - MIDDLE_CUTOFF_VALUE) / 32.0f);
	} else {
		resAmp = resAmpBase;
	}
	lowCutoffAttenuationFactor = cutoff < MIDDLE_CUTOFF_VALUE ?
#if MT32EMU_FASTER_MATH
		fexp2(0.125f * cutoff) * 0.0000152587890625f // EXP2F(-0.125f * MIDDLE_CUTOFF_VALUE)
#else
		EXP2F(-0.125f * (MIDDLE_CUTOFF_VALUE - cutoff))
#endif
		: 1;
	updateWaveFormParameters();
}

float LA32FloatWaveGenerator::generateNextSample() {
	if (!active) {
		return 0.0f;
	}

	float sample = 0.0f;

	if (isPCMWave()) {
		// Render PCM waveform
		int len = pcmWaveLength;
		int intPCMPosition = int(pcmPosition);
		if (intPCMPosition >= len && !pcmWaveLooped) {
			// We're now past the end of a non-looping PCM waveform so it's time to die.
			deactivate();
			return 0.0f;
		}

		// Linear interpolation
		float firstSample = getPCMSample(intPCMPosition);
		// We observe that for partial structures with ring modulation the interpolation is not applied to the slave PCM partial.
		// It's assumed that the multiplication circuitry intended to perform the interpolation on the slave PCM partial
		// is borrowed by the ring modulation circuit (or the LA32 chip has a similar lack of resources assigned to each partial pair).
		if (pcmWaveInterpolated) {
			sample = firstSample + (getPCMSample(intPCMPosition + 1) - firstSample) * (pcmPosition - intPCMPosition);
		} else {
			sample = firstSample;
		}

		float newPCMPosition = pcmPosition + pcmPositionDelta;
		if (pcmWaveLooped) {
			newPCMPosition = fmod(newPCMPosition, float(pcmWaveLength));
		}
		pcmPosition = newPCMPosition;
	} else {
		// Render synthesised waveform

		// Start playing in center of first cosine segment
		// relWavePos is shifted by a half of cosineLen
		float relWavePos = wavePos + 0.5f * cosineLen;
		if (relWavePos > waveLen) {
			relWavePos -= waveLen;
		}

		float pulseLen = pulseLenFactor * waveLen;
		float hLen = pulseLen - cosineLen;

		// Ignore pulsewidths too high for given freq
		if (hLen < 0.0f) {
			hLen = 0.0f;
		}

		// Produce filtered square wave with 2 cosine waves on slopes
		float squareWaveSample;

		// 1st cosine segment
		if (relWavePos < cosineLen) {
			squareWaveSample = -fcos(relWavePos / cosineLen);
		} else

		// high linear segment
		if (relWavePos < (cosineLen + hLen)) {
			squareWaveSample = 1.f;
		} else

		// 2nd cosine segment
		if (relWavePos < (2 * cosineLen + hLen)) {
			squareWaveSample = fcos((relWavePos - (cosineLen + hLen)) / cosineLen);
		} else {

		// low linear segment
			squareWaveSample = -1.f;
		}

		sample = squareWaveSample;

		if (cutoff < MIDDLE_CUTOFF_VALUE) {
			sample *= lowCutoffAttenuationFactor;
		} else {

			// Add resonance sine. Effective for cutoff > 50 only
			float resSample = 1.0f;

			// Resonance decay speed factor
			float resAmpDecayFactor = resAmpDecayFactors[resonance >> 2];

			// Now relWavePos counts from the middle of first cosine
			relWavePos = wavePos;

			// negative segments
			if (!(relWavePos < (cosineLen + hLen))) {
				resSample = -resSample;
				relWavePos -= cosineLen + hLen;

				// From the digital captures, the decaying speed of the resonance sine is found a bit different for the positive and the negative segments
				resAmpDecayFactor += 0.25f;
			}

			// Resonance sine WG
			resSample *= fsin(relWavePos / cosineLen);

			// Resonance sine amp
			float resAmpFadeLog2 = -0.125f * resAmpDecayFactor * (relWavePos / cosineLen); // seems to be exact
			float resAmpFade;
#if MT32EMU_FASTER_MATH
			{
				static const float resAmpFadeFactor = 1.0f / (1 << 30);
				resAmpFade = (resAmpFadeLog2 < -30.0f) ? 0.0f : fexp2(30.0f + resAmpFadeLog2) * resAmpFadeFactor;
			}
#else
			resAmpFade = EXP2F(resAmpFadeLog2);
#endif

			// Now relWavePos set negative to the left from center of any cosine
			relWavePos = wavePos;

			// negative segment
			if (!(wavePos < (waveLen - 0.5f * cosineLen))) {
				relWavePos -= waveLen;
			} else

			// positive segment
			if (!(wavePos < (hLen + 0.5f * cosineLen))) {
				relWavePos -= cosineLen + hLen;
			}

			// To ensure the output wave has no breaks, two different windows are applied to the beginning and the ending of the resonance sine segment
			if (relWavePos < 0.5f * cosineLen) {
				if (relWavePos < 0.0f) {
					// The window is synchronous square sine here
					resAmpFade *= squareWaveSample * squareWaveSample;
				} else {
					// The window is synchronous sine here
					resAmpFade *= squareWaveSample < 0 ? -squareWaveSample : squareWaveSample;
				}
			}

			sample += resSample * resAmp * resAmpFade;
		}

		// sawtooth waves
		if (sawtoothWaveform) {
			sample *= fcos(2 * wavePos / waveLen);
		}

		wavePos++;

		// wavePos isn't supposed to be > waveLen
		if (wavePos > waveLen) {
			wavePos -= waveLen;
		}
	}

	// Multiply sample with current TVA value
	sample *= ampFactor;
	return sample;
}

void LA32FloatWaveGenerator::deactivate() {
	active = false;
}

bool LA32FloatWaveGenerator::isActive() const {
	return active;
}

bool LA32FloatWaveGenerator::isPCMWave() const {
	return pcmWaveAddress != NULL;
}

void LA32FloatPartialPair::initTables(const Tables &tables) {
	resAmpDecayFactors = tables.resAmpDecayFactors;

#if MT32EMU_FASTER_MATH > 0

	for (Bit32u i = 0; i < FEXP2_TABLE_SIZE_M1; i++) {
		FEXP2_TABLE[i] = exp(FEXP2_SCALE_FACTOR * i);
	}
	FEXP2_TABLE[FEXP2_TABLE_SIZE_M1] = 2.0f;

	for (Bit32u i = 0; i < FSIN_TABLE_SIZE; i++) {
		FSIN_TABLE[i] = sin(FSIN_SCALE_FACTOR * i);
	}

#endif

}

void LA32FloatPartialPair::init(const bool useRingModulated, const bool useMixed) {
	ringModulated = useRingModulated;
	mixed = useMixed;
	masterOutputSample = 0.0f;
	slaveOutputSample = 0.0f;
}

void LA32FloatPartialPair::initSynth(const PairType useMaster, const bool sawtoothWaveform, const Bit8u pulseWidth, const Bit8u resonance) {
	if (useMaster == MASTER) {
		master.initSynth(sawtoothWaveform, pulseWidth, resonance);
	} else {
		slave.initSynth(sawtoothWaveform, pulseWidth, resonance);
	}
}

void LA32FloatPartialPair::initPCM(const PairType useMaster, const Bit16s *pcmWaveAddress, const Bit32u pcmWaveLength, const bool pcmWaveLooped) {
	if (useMaster == MASTER) {
		master.initPCM(pcmWaveAddress, pcmWaveLength, pcmWaveLooped, true);
	} else {
		slave.initPCM(pcmWaveAddress, pcmWaveLength, pcmWaveLooped, !ringModulated);
	}
}

void LA32FloatPartialPair::setAmp(const PairType useMaster, const Bit32u amp) {
	if (useMaster == MASTER) {
		master.setAmp(amp);
	} else {
		slave.setAmp(amp);
	}
}

void LA32FloatPartialPair::setPitch(const PairType useMaster, const Bit16u pitch) {
	if (useMaster == MASTER) {
		master.setPitch(pitch);
	} else {
		slave.setPitch(pitch);
	}
}

void LA32FloatPartialPair::setCutoff(const PairType useMaster, const Bit32u cutoff) {
	if (useMaster == MASTER) {
		master.setCutoff(cutoff);
	} else {
		slave.setCutoff(cutoff);
	}
}

void LA32FloatPartialPair::generateNextSample(const PairType useMaster) {
	if (useMaster == MASTER) {
		masterOutputSample = master.generateNextSample();
	} else {
		slaveOutputSample = slave.generateNextSample();
	}
}

static inline float produceDistortedSample(float sample) {
	if (sample < -1.0f) {
		return sample + 2.0f;
	} else if (1.0f < sample) {
		return sample - 2.0f;
	}
	return sample;
}

float LA32FloatPartialPair::nextOutSample() {
	// Note, LA32FloatWaveGenerator produces each sample normalised in terms of a single playing partial,
	// so the unity sample corresponds to the internal LA32 logarithmic fixed-point unity sample.
	// However, each logarithmic sample is then unlogged to a 14-bit signed integer value, i.e. the max absolute value is 8192.
	// Thus, considering that samples are further mapped to a 16-bit signed integer,
	// we apply a conversion factor 0.25 to produce properly normalised float samples.
	if (!ringModulated) {
		return 0.25f * (masterOutputSample + slaveOutputSample);
	}
	/*
	 * SEMI-CONFIRMED: Ring modulation model derived from sample analysis of specially constructed patches which exploit distortion.
	 * LA32 ring modulator found to produce distorted output in case if the absolute value of maximal amplitude of one of the input partials exceeds 8191.
	 * This is easy to reproduce using synth partials with resonance values close to the maximum. It looks like an integer overflow happens in this case.
	 * As the distortion is strictly bound to the amplitude of the complete mixed square + resonance wave in the linear space,
	 * it is reasonable to assume the ring modulation is performed also in the linear space by sample multiplication.
	 * Most probably the overflow is caused by limited precision of the multiplication circuit as the very similar distortion occurs with panning.
	 */
	float ringModulatedSample = produceDistortedSample(masterOutputSample) * produceDistortedSample(slaveOutputSample);
	return 0.25f * (mixed ? masterOutputSample + ringModulatedSample : ringModulatedSample);
}

void LA32FloatPartialPair::deactivate(const PairType useMaster) {
	if (useMaster == MASTER) {
		master.deactivate();
		masterOutputSample = 0.0f;
	} else {
		slave.deactivate();
		slaveOutputSample = 0.0f;
	}
}

bool LA32FloatPartialPair::isActive(const PairType useMaster) const {
	return useMaster == MASTER ? master.isActive() : slave.isActive();
}

} // namespace MT32Emu
