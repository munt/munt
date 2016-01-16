/* Copyright (C) 2015-2016 Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "FIRInterpolator.h"

#include <cmath>

#ifndef M_PI
static const double M_PI = 3.1415926535897931;
#endif

FIRInterpolator::C::C(const unsigned int upsampleFactor, const double downsampleFactor, const FIRCoefficient kernel[], const unsigned int kernelLength) {
	taps = kernel;
	usePhaseInterpolation = downsampleFactor != floor(downsampleFactor);
	numberOfTaps = kernelLength;
	numberOfPhases = upsampleFactor;
	phaseIncrement = downsampleFactor;
	unsigned int minDelayLineLength = (unsigned int)ceil(kernelLength / upsampleFactor);
	unsigned int delayLineLength = 2;
	while (delayLineLength < minDelayLineLength) delayLineLength <<= 1;
	delayLineMask = delayLineLength - 1;
	ringBuffer = new FloatSample[delayLineLength][FIR_INTERPOLATOR_CHANNEL_COUNT];
	FloatSample *s = *ringBuffer;
	FloatSample *e = ringBuffer[delayLineLength];
	while (s < e) *(s++) = 0;
}

FIRInterpolator::FIRInterpolator(const unsigned int upsampleFactor, const double downsampleFactor, const FIRCoefficient kernel[], const unsigned int kernelLength) :
	c(upsampleFactor, downsampleFactor, kernel, kernelLength),
	ringBufferPosition(0),
	phase(c.numberOfPhases)
{}

FIRInterpolator::~FIRInterpolator() {
	delete[] c.ringBuffer;
}

void FIRInterpolator::process(const FloatSample *&inSamples, unsigned int &inLength, FloatSample *&outSamples, unsigned int &outLength) {
	while (outLength > 0) {
		while (needNextInSample()) {
			if (inLength == 0) return;
			addInSamples(inSamples);
			--inLength;
		}
		getOutSamples(outSamples);
		--outLength;
	}
}

unsigned int FIRInterpolator::estimateInLength(const unsigned int outLength) const {
	return (unsigned int)((outLength * c.phaseIncrement + phase) / c.numberOfPhases);
}

bool FIRInterpolator::needNextInSample() const {
	return c.numberOfPhases <= phase;
}

void FIRInterpolator::addInSamples(const FloatSample *&inSamples) {
	ringBufferPosition = (ringBufferPosition - 1) & c.delayLineMask;
	c.ringBuffer[ringBufferPosition][0] = *(inSamples++);
	c.ringBuffer[ringBufferPosition][1] = *(inSamples++);
	phase -= c.numberOfPhases;
}

void FIRInterpolator::getOutSamples(FloatSample *&outSamples) {
	FloatSample leftSample = 0.0;
	FloatSample rightSample = 0.0;
	if (c.usePhaseInterpolation) {
		double phaseFraction = phase - floor(phase);
		unsigned int delaySampleIx = ringBufferPosition;
		for (unsigned int tapIx = (unsigned int)phase; tapIx < c.numberOfTaps; tapIx += c.numberOfPhases) {
			FIRCoefficient tap = FIRCoefficient(c.taps[tapIx] + (c.taps[tapIx + 1] - c.taps[tapIx]) * phaseFraction);
			leftSample += tap * c.ringBuffer[delaySampleIx][0];
			rightSample += tap * c.ringBuffer[delaySampleIx][1];
			delaySampleIx = (delaySampleIx + 1) & c.delayLineMask;
		}
	} else {
		// Optimised for rational resampling ratios when phase is always integer
		unsigned int delaySampleIx = ringBufferPosition;
		for (unsigned int tapIx = (unsigned int)phase; tapIx < c.numberOfTaps; tapIx += c.numberOfPhases) {
			FIRCoefficient tap = c.taps[tapIx];
			leftSample += tap * c.ringBuffer[delaySampleIx][0];
			rightSample += tap * c.ringBuffer[delaySampleIx][1];
			delaySampleIx = (delaySampleIx + 1) & c.delayLineMask;
		}
	}
	*(outSamples++) = leftSample;
	*(outSamples++) = rightSample;
	phase += c.phaseIncrement;
}
