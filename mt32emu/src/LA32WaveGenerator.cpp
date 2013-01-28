/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

namespace MT32Emu {

static const Bit32u MIDDLE_CUTOFF_VALUE = 128 << 18;
static const Bit32u RESONANCE_DECAY_THRESHOLD_CUTOFF_VALUE = 144 << 18;
static const Bit32u MAX_CUTOFF_VALUE = 240 << 18;
static const LogSample SILENCE = {65535, LogSample::POSITIVE};

Bit16u LA32Utilites::interpolateExp(Bit16u fract) {
	Bit16u expTabIndex = fract >> 3;
	Bit16u extraBits = fract & 7;
	Bit16u expTabEntry2 = 8191 - Tables::getInstance().exp9[expTabIndex];
	Bit16u expTabEntry1 = expTabIndex == 0 ? 8191 : (8191 - Tables::getInstance().exp9[expTabIndex - 1]);
	return expTabEntry1 + (((expTabEntry2 - expTabEntry1) * extraBits) >> 3);
}

Bit16s LA32Utilites::unlog(LogSample logSample) {
	//Bit16s sample = (Bit16s)EXP2F(13.0f - logSample.logValue / 1024.0f);
	Bit32u intLogValue = logSample.logValue >> 12;
	Bit32u fracLogValue = logSample.logValue & 4095;
	Bit16s sample = interpolateExp(fracLogValue) >> intLogValue;
	return logSample.sign == LogSample::POSITIVE ? sample : -sample;
}

LogSample LA32Utilites::addLogSamples(LogSample sample1, LogSample sample2) {
	Bit32u logSampleValue = sample1.logValue + sample2.logValue;
	LogSample logSample;
	logSample.logValue = logSampleValue < 65536 ? logSampleValue : 65535;
	logSample.sign = sample1.sign == sample2.sign ? LogSample::POSITIVE : LogSample::NEGATIVE;
	return logSample;
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

	// The 240 cutoffVal limit was determined via sample analysis (internal Munt capture IDs: glop3, glop4).
	// More research is needed to be sure that this is correct, however.
	if (cutoffVal > MAX_CUTOFF_VALUE) {
		cutoffVal = MAX_CUTOFF_VALUE;
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
	if (phase == POSITIVE_LINEAR_SEGMENT) {
		if (squareWavePosition >= highLen) {
			squareWavePosition -= highLen;
			phase = POSITIVE_FALLING_SINE_SEGMENT;
		}
	} else if (phase == NEGATIVE_LINEAR_SEGMENT) {
		if (squareWavePosition >= lowLen) {
			squareWavePosition -= lowLen;
			phase = NEGATIVE_RISING_SINE_SEGMENT;
		}
	} else if (squareWavePosition >= (1 << 18)) {
		squareWavePosition -= 1 << 18;
		if (phase == NEGATIVE_RISING_SINE_SEGMENT) {
			phase = POSITIVE_RISING_SINE_SEGMENT;
			resonancePhase = POSITIVE_RISING_RESONANCE_SINE_SEGMENT;
			resonanceSinePosition = squareWavePosition;
		} else {
			// phase incrementing hack
			++(*(int*)&phase);
			if (phase == NEGATIVE_FALLING_SINE_SEGMENT) {
				resonancePhase = NEGATIVE_FALLING_RESONANCE_SINE_SEGMENT;
				resonanceSinePosition = squareWavePosition;
			}
		}
	}
	// resonancePhase computation hack
	*(int*)&resonancePhase = ((resonanceSinePosition >> 18) + (phase > POSITIVE_FALLING_SINE_SEGMENT ? 2 : 0)) & 3;
}

LogSample LA32WaveGenerator::nextSquareWaveLogSample() {
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

	LogSample logSample;
	logSample.logValue = logSampleValue < 65536 ? logSampleValue : 65535;
	logSample.sign = phase < NEGATIVE_FALLING_SINE_SEGMENT ? LogSample::POSITIVE : LogSample::NEGATIVE;
	return logSample;
}

LogSample LA32WaveGenerator::nextResonanceWaveLogSample() {
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

	LogSample logSample;
	logSample.logValue = logSampleValue < 65536 ? logSampleValue : 65535;
	logSample.sign = resonancePhase < NEGATIVE_FALLING_RESONANCE_SINE_SEGMENT ? LogSample::POSITIVE : LogSample::NEGATIVE;
	return logSample;
}

LogSample LA32WaveGenerator::nextSawtoothCosineLogSample() {
	Bit32u logSampleValue;
	if ((sawtoothCosinePosition & (1 << 18)) > 0) {
		logSampleValue = Tables::getInstance().logsin9[~(sawtoothCosinePosition >> 9) & 511];
	} else {
		logSampleValue = Tables::getInstance().logsin9[(sawtoothCosinePosition >> 9) & 511];
	}
	logSampleValue <<= 2;

	LogSample logSample;
	logSample.logValue = logSampleValue < 65536 ? logSampleValue : 65535;
	logSample.sign = ((sawtoothCosinePosition & (1 << 19)) == 0) ? LogSample::POSITIVE : LogSample::NEGATIVE;
	return logSample;
}

void LA32WaveGenerator::init(bool sawtoothWaveform, Bit8u pulseWidth, Bit8u resonance) {
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

	active = true;
}

void LA32WaveGenerator::generateNextSample(Bit32u amp, Bit16u pitch, Bit32u cutoffVal) {
	if (!active) {
		return;
	}
	this->amp = amp;
	this->pitch = pitch;
	this->cutoffVal = cutoffVal;

	updateWaveGeneratorState();
	squareLogSample = nextSquareWaveLogSample();
	resonanceLogSample = nextResonanceWaveLogSample();
	if (sawtoothWaveform) {
		LogSample cosineLogSample = nextSawtoothCosineLogSample();
		this->squareLogSample = LA32Utilites::addLogSamples(squareLogSample, cosineLogSample);
		this->resonanceLogSample = LA32Utilites::addLogSamples(resonanceLogSample, cosineLogSample);
	}
	advancePosition();
}

LogSample LA32WaveGenerator::getSquareLogSample() {
	return squareLogSample;
}

LogSample LA32WaveGenerator::getResonanceLogSample() {
	return resonanceLogSample;
}

void LA32WaveGenerator::deactivate() {
	squareLogSample = resonanceLogSample = SILENCE;
	active = false;
}

void LA32PartialPair::init(bool ringModulated, bool mixed) {
	this->ringModulated = ringModulated;
	this->mixed = mixed;
}

void LA32PartialPair::initMaster(bool sawtoothWaveform, Bit8u pulseWidth, Bit8u resonance) {
	master.init(sawtoothWaveform, pulseWidth, resonance);
}

void LA32PartialPair::initSlave(bool sawtoothWaveform, Bit8u pulseWidth, Bit8u resonance) {
	slave.init(sawtoothWaveform, pulseWidth, resonance);
}

void LA32PartialPair::generateNextMasterSample(Bit32u amp, Bit16u pitch, Bit32u cutoff) {
	master.generateNextSample(amp, pitch, cutoff);
}

void LA32PartialPair::generateNextSlaveSample(Bit32u amp, Bit16u pitch, Bit32u cutoff) {
	slave.generateNextSample(amp, pitch, cutoff);
}

Bit16s LA32PartialPair::nextOutSample() {
	LogSample masterSquareLogSample = master.getSquareLogSample();
	LogSample masterResonanceLogSample = master.getResonanceLogSample();
	LogSample slaveSquareLogSample = slave.getSquareLogSample();
	LogSample slaveResonanceLogSample = slave.getResonanceLogSample();
	if (ringModulated) {
		Bit16s sample = LA32Utilites::unlog(LA32Utilites::addLogSamples(masterSquareLogSample, slaveSquareLogSample));
		sample += LA32Utilites::unlog(LA32Utilites::addLogSamples(masterSquareLogSample, slaveResonanceLogSample));
		sample += LA32Utilites::unlog(LA32Utilites::addLogSamples(slaveSquareLogSample, masterResonanceLogSample));
		sample += LA32Utilites::unlog(LA32Utilites::addLogSamples(masterResonanceLogSample, slaveResonanceLogSample));
		if (mixed) {
			sample += LA32Utilites::unlog(masterSquareLogSample) + LA32Utilites::unlog(masterResonanceLogSample);
		}
		return sample;
	}
	Bit16s sample = LA32Utilites::unlog(masterSquareLogSample) + LA32Utilites::unlog(masterResonanceLogSample);
	sample += LA32Utilites::unlog(slaveSquareLogSample) + LA32Utilites::unlog(slaveResonanceLogSample);
	return sample;
}

void LA32PartialPair::deactivateMaster() {
	master.deactivate();
}

void LA32PartialPair::deactivateSlave() {
	slave.deactivate();
}

}
