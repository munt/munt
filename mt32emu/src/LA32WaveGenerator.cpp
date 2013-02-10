/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011, 2012, 2013 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

#include <cmath>
#include "mt32emu.h"
#include "mmath.h"
#include "LA32WaveGenerator.h"

#if MT32EMU_ACCURATE_WG == 0

namespace MT32Emu {

static const Bit32u MIDDLE_CUTOFF_VALUE = 128 << 18;
static const Bit32u RESONANCE_DECAY_THRESHOLD_CUTOFF_VALUE = 144 << 18;
static const Bit32u MAX_CUTOFF_VALUE = 240 << 18;
static const LogSample SILENCE = {65535, LogSample::POSITIVE};

Bit16u LA32Utilites::interpolateExp(const Bit16u fract) {
	Bit16u expTabIndex = fract >> 3;
	Bit16u extraBits = fract & 7;
	Bit16u expTabEntry2 = 8191 - Tables::getInstance().exp9[expTabIndex];
	Bit16u expTabEntry1 = expTabIndex == 0 ? 8191 : (8191 - Tables::getInstance().exp9[expTabIndex - 1]);
	return expTabEntry1 + (((expTabEntry2 - expTabEntry1) * extraBits) >> 3);
}

Bit16s LA32Utilites::unlog(const LogSample logSample) {
	//Bit16s sample = (Bit16s)EXP2F(13.0f - logSample.logValue / 1024.0f);
	Bit32u intLogValue = logSample.logValue >> 12;
	Bit32u fracLogValue = logSample.logValue & 4095;
	Bit16s sample = interpolateExp(fracLogValue) >> intLogValue;
	return logSample.sign == LogSample::POSITIVE ? sample : -sample;
}

void LA32Utilites::addLogSamples(LogSample &logSample1, const LogSample logSample2) {
	Bit32u logSampleValue = logSample1.logValue + logSample2.logValue;
	logSample1.logValue = logSampleValue < 65536 ? logSampleValue : 65535;
	logSample1.sign = logSample1.sign == logSample2.sign ? LogSample::POSITIVE : LogSample::NEGATIVE;
}

void LA32WaveGenerator::updateWaveGeneratorState() {
	// sawtoothCosineStep = EXP2F(pitch / 4096. + 4)
	if (sawtoothWaveform) {
		Bit32u expArgInt = pitch >> 12;
		sawtoothCosineStep = LA32Utilites::interpolateExp(~pitch & 4095);
		if (expArgInt < 8) {
			sawtoothCosineStep >>= 8 - expArgInt;
		} else {
			sawtoothCosineStep <<= expArgInt - 8;
		}
	}

	Bit32u cosineLenFactor = 0;
	if (cutoffVal > MIDDLE_CUTOFF_VALUE) {
		cosineLenFactor = (cutoffVal - MIDDLE_CUTOFF_VALUE) >> 10;
	}

	// sampleStep = EXP2F(pitch / 4096. + cosineLenFactor / 4096. + 4)
	{
		Bit32u expArg = pitch + cosineLenFactor;
		Bit32u expArgInt = expArg >> 12;
		sampleStep = LA32Utilites::interpolateExp(~expArg & 4095);
		if (expArgInt < 8) {
			sampleStep >>= 8 - expArgInt;
		} else {
			sampleStep <<= expArgInt - 8;
		}
	}

	// Ratio of positive segment to wave length
	Bit32u pulseLenFactor = 0;
	if (pulseWidth > 128) {
		pulseLenFactor = (pulseWidth - 128) << 6;
	}

	// highLen = EXP2F(19 - pulseLenFactor / 4096. + cosineLenFactor / 4096.) - (2 << 18);
	if (pulseLenFactor < cosineLenFactor) {
		Bit32u expArg = cosineLenFactor - pulseLenFactor;
		Bit32u expArgInt = expArg >> 12;
		highLen = LA32Utilites::interpolateExp(~expArg & 4095);
		highLen <<= 7 + expArgInt;
		highLen -= (2 << 18);
	} else {
		highLen = 0;
	}

	// lowLen = EXP2F(20 + cosineLenFactor / 4096.) - (4 << 18) - highLen;
	lowLen = LA32Utilites::interpolateExp(~cosineLenFactor & 4095);
	lowLen <<= 8 + (cosineLenFactor >> 12);
	lowLen -= (4 << 18) + highLen;
}

void LA32WaveGenerator::advancePosition() {
	squareWavePosition += sampleStep;
	resonanceSinePosition += sampleStep;
	if (sawtoothWaveform) {
		sawtoothCosinePosition = (sawtoothCosinePosition + sawtoothCosineStep) & ((1 << 20) - 1);
	}
	for (;;) {
		if (phase == POSITIVE_LINEAR_SEGMENT) {
			if (squareWavePosition < highLen) {
				break;
			} else {
				squareWavePosition -= highLen;
				phase = POSITIVE_FALLING_SINE_SEGMENT;
			}
		} else if (phase == NEGATIVE_LINEAR_SEGMENT) {
			if (squareWavePosition < lowLen) {
				break;
			} else {
				squareWavePosition -= lowLen;
				phase = NEGATIVE_RISING_SINE_SEGMENT;
			}
		} else if (squareWavePosition < (1 << 18)) {
			break;
		} else {
			squareWavePosition -= 1 << 18;
			if (phase == NEGATIVE_RISING_SINE_SEGMENT) {
				phase = POSITIVE_RISING_SINE_SEGMENT;
				resonanceSinePosition = squareWavePosition;
				sawtoothCosinePosition = 1 << 18;
			} else {
				// phase incrementing hack
				++(*(int*)&phase);
				if (phase == NEGATIVE_FALLING_SINE_SEGMENT) {
					resonanceSinePosition = squareWavePosition;
				}
			}
		}
	}
	// resonancePhase computation hack
	*(int*)&resonancePhase = ((resonanceSinePosition >> 18) + (phase > POSITIVE_FALLING_SINE_SEGMENT ? 2 : 0)) & 3;
}

void LA32WaveGenerator::generateNextSquareWaveLogSample() {
	Bit32u logSampleValue;
	switch (phase) {
		case POSITIVE_RISING_SINE_SEGMENT:
			logSampleValue = Tables::getInstance().logsin9[(squareWavePosition >> 9) & 511];
			break;
		case POSITIVE_LINEAR_SEGMENT:
			logSampleValue = 0;
			break;
		case POSITIVE_FALLING_SINE_SEGMENT:
			logSampleValue = Tables::getInstance().logsin9[~(squareWavePosition >> 9) & 511];
			break;
		case NEGATIVE_FALLING_SINE_SEGMENT:
			logSampleValue = Tables::getInstance().logsin9[(squareWavePosition >> 9) & 511];
			break;
		case NEGATIVE_LINEAR_SEGMENT:
			logSampleValue = 0;
			break;
		case NEGATIVE_RISING_SINE_SEGMENT:
			logSampleValue = Tables::getInstance().logsin9[~(squareWavePosition >> 9) & 511];
			break;
	}
	logSampleValue <<= 2;
	logSampleValue += amp >> 10;
	if (cutoffVal < MIDDLE_CUTOFF_VALUE) {
		logSampleValue += (MIDDLE_CUTOFF_VALUE - cutoffVal) >> 9;
	}

	squareLogSample.logValue = logSampleValue < 65536 ? logSampleValue : 65535;
	squareLogSample.sign = phase < NEGATIVE_FALLING_SINE_SEGMENT ? LogSample::POSITIVE : LogSample::NEGATIVE;
}

void LA32WaveGenerator::generateNextResonanceWaveLogSample() {
	Bit32u logSampleValue;
	if (resonancePhase == POSITIVE_FALLING_RESONANCE_SINE_SEGMENT || resonancePhase == NEGATIVE_RISING_RESONANCE_SINE_SEGMENT) {
		logSampleValue = Tables::getInstance().logsin9[~(resonanceSinePosition >> 9) & 511];
	} else {
		logSampleValue = Tables::getInstance().logsin9[(resonanceSinePosition >> 9) & 511];
	}
	logSampleValue <<= 2;
	logSampleValue += amp >> 10;

	// From the digital captures, the decaying speed of the resonance sine is found a bit different for the positive and the negative segments
	Bit32u decayFactor = phase < NEGATIVE_FALLING_SINE_SEGMENT ? resAmpDecayFactor : resAmpDecayFactor + 1;
	// Unsure about resonanceSinePosition here. It's possible that dedicated counter & decrement are used. Although, cutoff is finely ramped, so maybe not.
	logSampleValue += resonanceAmpSubtraction + ((resonanceSinePosition * decayFactor) >> 12);

	// To ensure the output wave has no breaks, two different windows are appied to the beginning and the ending of the resonance sine segment
	if (phase == POSITIVE_RISING_SINE_SEGMENT || phase == NEGATIVE_FALLING_SINE_SEGMENT) {
		// The window is synchronous sine here
		logSampleValue += Tables::getInstance().logsin9[(squareWavePosition >> 9) & 511] << 2;
	} else if (phase == POSITIVE_FALLING_SINE_SEGMENT || phase == NEGATIVE_RISING_SINE_SEGMENT) {
		// The window is synchronous square sine here
		logSampleValue += Tables::getInstance().logsin9[~(squareWavePosition >> 9) & 511] << 3;
	}

	if (cutoffVal < MIDDLE_CUTOFF_VALUE) {
		// For the cutoff values below the cutoff middle point, it seems the amp of the resonance wave is expotentially decayed
		logSampleValue += 31743 + ((MIDDLE_CUTOFF_VALUE - cutoffVal) >> 9);
	} else if (cutoffVal < RESONANCE_DECAY_THRESHOLD_CUTOFF_VALUE) {
		// For the cutoff values below this point, the amp of the resonance wave is sinusoidally decayed
		Bit32u sineIx = (cutoffVal - MIDDLE_CUTOFF_VALUE) >> 13;
		logSampleValue += Tables::getInstance().logsin9[sineIx] << 2;
	}

	// After all the amp decrements are added, it should be safe now to adjust the amp of the resonance wave to what we see on captures
	logSampleValue -= 1 << 12;

	resonanceLogSample.logValue = logSampleValue < 65536 ? logSampleValue : 65535;
	resonanceLogSample.sign = resonancePhase < NEGATIVE_FALLING_RESONANCE_SINE_SEGMENT ? LogSample::POSITIVE : LogSample::NEGATIVE;
}

LogSample LA32WaveGenerator::nextSawtoothCosineLogSample() const {
	LogSample logSample;
	if ((sawtoothCosinePosition & (1 << 18)) > 0) {
		logSample.logValue = Tables::getInstance().logsin9[~(sawtoothCosinePosition >> 9) & 511];
	} else {
		logSample.logValue = Tables::getInstance().logsin9[(sawtoothCosinePosition >> 9) & 511];
	}
	logSample.logValue <<= 2;

	logSample.sign = ((sawtoothCosinePosition & (1 << 19)) == 0) ? LogSample::POSITIVE : LogSample::NEGATIVE;
	return logSample;
}

LogSample LA32WaveGenerator::pcmSampleToLogSample(const Bit16s pcmSample) const {
	LogSample logSample;
	logSample.sign = pcmSample < 0 ? LogSample::NEGATIVE : LogSample::POSITIVE;
	// TODO: This was accurate for the float model, to be refined
	Bit32u logSampleValue = (32787 - (pcmSample & 32767)) << 1;
	logSampleValue += amp >> 10;
	logSample.logValue = logSampleValue < 65536 ? logSampleValue : 65535;
	return logSample;
}

void LA32WaveGenerator::generateNextPCMWaveLogSamples() {
	// pcmSampleStep = EXP2F(pitch / 4096. - 5.);
	pcmSampleStep = LA32Utilites::interpolateExp(~pitch & 4095);
	pcmSampleStep <<= pitch >> 12;
	// Seeing the actual lengths of the PCM wave for pitches 00..12,
	// the pcmPosition counter can be assumed to have 8-bit fractions
	pcmSampleStep >>= 9;
	// This should emulate the ladder we see in the PCM captures for pitches 01, 02, 07, etc.
	// The most probable cause is the factor in the interpolation formula is one bit less
	// accurate than the sample position counter
	pcmInterpolationFactor = (pcmPosition & 255) >> 1;
	Bit32u pcmWaveTableIx = pcmPosition >> 8;
	firstPCMLogSample = pcmSampleToLogSample(pcmWaveAddress[pcmWaveTableIx]);
	if (pcmWaveInterpolated) {
		pcmWaveTableIx++;
		if (pcmWaveTableIx < pcmWaveLength) {
			secondPCMLogSample = pcmSampleToLogSample(pcmWaveAddress[pcmWaveTableIx]);
		} else {
			if (pcmWaveLooped) {
				pcmWaveTableIx -= pcmWaveLength;
				secondPCMLogSample = pcmSampleToLogSample(pcmWaveAddress[pcmWaveTableIx]);
			} else {
				secondPCMLogSample = SILENCE;
			}
		}
	} else {
		secondPCMLogSample = SILENCE;
	}
	pcmPosition += pcmSampleStep;
	if (pcmPosition >= (pcmWaveLength << 8)) {
		if (pcmWaveLooped) {
			pcmPosition -= pcmWaveLength << 8;
		} else {
			active = false;
		}
	}
}

void LA32WaveGenerator::initSynth(const bool sawtoothWaveform, const Bit8u pulseWidth, const Bit8u resonance) {
	this->sawtoothWaveform = sawtoothWaveform;
	this->pulseWidth = pulseWidth;
	this->resonance = resonance;

	phase = POSITIVE_RISING_SINE_SEGMENT;
	squareWavePosition = 0;
	sawtoothCosinePosition = 1 << 18;

	resonancePhase = POSITIVE_RISING_RESONANCE_SINE_SEGMENT;
	resonanceSinePosition = 0;
	resonanceAmpSubtraction = (32 - resonance) << 10;

	resAmpDecayFactor = Tables::getInstance().resAmpDecayFactor[resonance >> 2] << 2;

	pcmWaveAddress = NULL;
	active = true;
}

void LA32WaveGenerator::initPCM(const Bit16s * const pcmWaveAddress, const Bit32u pcmWaveLength, const bool pcmWaveLooped, const bool pcmWaveInterpolated) {
	this->pcmWaveAddress = pcmWaveAddress;
	this->pcmWaveLength = pcmWaveLength;
	this->pcmWaveLooped = pcmWaveLooped;
	this->pcmWaveInterpolated = pcmWaveInterpolated;

	pcmPosition = 0;
	active = true;
}

void LA32WaveGenerator::generateNextSample(const Bit32u amp, const Bit16u pitch, const Bit32u cutoffVal) {
	if (!active) {
		return;
	}

	this->amp = amp;
	this->pitch = pitch;

	if (isPCMWave()) {
		generateNextPCMWaveLogSamples();
		return;
	}

	// The 240 cutoffVal limit was determined via sample analysis (internal Munt capture IDs: glop3, glop4).
	// More research is needed to be sure that this is correct, however.
	this->cutoffVal = (cutoffVal > MAX_CUTOFF_VALUE) ? MAX_CUTOFF_VALUE : cutoffVal;

	updateWaveGeneratorState();
	generateNextSquareWaveLogSample();
	generateNextResonanceWaveLogSample();
	if (sawtoothWaveform) {
		LogSample cosineLogSample = nextSawtoothCosineLogSample();
		LA32Utilites::addLogSamples(squareLogSample, cosineLogSample);
		LA32Utilites::addLogSamples(resonanceLogSample, cosineLogSample);
	}
	advancePosition();
}

LogSample LA32WaveGenerator::getOutputLogSample(const bool first) const {
	if (!isActive()) {
		return SILENCE;
	}
	if (isPCMWave()) {
		return first ? firstPCMLogSample : secondPCMLogSample;
	}
	return first ? squareLogSample : resonanceLogSample;
}

void LA32WaveGenerator::deactivate() {
	active = false;
}

bool LA32WaveGenerator::isActive() const {
	return active;
}

bool LA32WaveGenerator::isPCMWave() const {
	return pcmWaveAddress != NULL;
}

Bit32u LA32WaveGenerator::getPCMInterpolationFactor() const {
	return pcmInterpolationFactor;
}

void LA32PartialPair::init(const bool ringModulated, const bool mixed) {
	this->ringModulated = ringModulated;
	this->mixed = mixed;
}

void LA32PartialPair::initSynth(const PairType useMaster, const bool sawtoothWaveform, const Bit8u pulseWidth, const Bit8u resonance) {
	if (useMaster == MASTER) {
		master.initSynth(sawtoothWaveform, pulseWidth, resonance);
	} else {
		slave.initSynth(sawtoothWaveform, pulseWidth, resonance);
	}
}

void LA32PartialPair::initPCM(const PairType useMaster, const Bit16s *pcmWaveAddress, const Bit32u pcmWaveLength, const bool pcmWaveLooped) {
	if (useMaster == MASTER) {
		master.initPCM(pcmWaveAddress, pcmWaveLength, pcmWaveLooped, true);
	} else {
		slave.initPCM(pcmWaveAddress, pcmWaveLength, pcmWaveLooped, !ringModulated);
	}
}

void LA32PartialPair::generateNextSample(const PairType useMaster, const Bit32u amp, const Bit16u pitch, const Bit32u cutoff) {
	if (useMaster == MASTER) {
		master.generateNextSample(amp, pitch, cutoff);
	} else {
		slave.generateNextSample(amp, pitch, cutoff);
	}
}

Bit16s LA32PartialPair::unlogAndMixWGOutput(const LA32WaveGenerator &wg, const LogSample * const ringModulatingLogSample) {
	if (!wg.isActive() || ((ringModulatingLogSample != NULL) && (ringModulatingLogSample->logValue == SILENCE.logValue))) {
		return 0;
	}
	LogSample firstLogSample = wg.getOutputLogSample(true);
	LogSample secondLogSample = wg.getOutputLogSample(false);
	if (ringModulatingLogSample != NULL) {
		LA32Utilites::addLogSamples(firstLogSample, *ringModulatingLogSample);
		LA32Utilites::addLogSamples(secondLogSample, *ringModulatingLogSample);
	}
	Bit16s firstSample = LA32Utilites::unlog(firstLogSample);
	Bit16s secondSample = LA32Utilites::unlog(secondLogSample);
	if (wg.isPCMWave()) {
		return Bit16s(firstSample + ((Bit32s(secondSample - firstSample) * wg.getPCMInterpolationFactor()) >> 7));
	}
	return firstSample + secondSample;
}

Bit16s LA32PartialPair::nextOutSample() {
	if (ringModulated) {
		LogSample slaveFirstLogSample = slave.getOutputLogSample(true);
		LogSample slaveSecondLogSample = slave.getOutputLogSample(false);
		Bit16s sample = unlogAndMixWGOutput(master, &slaveFirstLogSample);
		if (!slave.isPCMWave()) {
			sample += unlogAndMixWGOutput(master, &slaveSecondLogSample);
		}
		if (mixed) {
			sample += unlogAndMixWGOutput(master, NULL);
		}
		return sample;
	}
	return unlogAndMixWGOutput(master, NULL) + unlogAndMixWGOutput(slave, NULL);
}

void LA32PartialPair::deactivate(const PairType useMaster) {
	if (useMaster == MASTER) {
		master.deactivate();
	} else {
		slave.deactivate();
	}
}

bool LA32PartialPair::isActive(const PairType useMaster) const {
	return useMaster == MASTER ? master.isActive() : slave.isActive();
}

}

#endif // #if MT32EMU_ACCURATE_WG == 0
