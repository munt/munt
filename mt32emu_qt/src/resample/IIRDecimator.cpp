/* Copyright (C) 2015 Sergey V. Mikayev
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

#include "IIRDecimator.h"

#include <cmath>
#include <cstddef>

#ifndef M_PI
static const double M_PI = 3.1415926535897931;
#endif

// Sharp elliptic filter with symmetric ripple: N=18, Ap=As=-106 dB, fp=0.238, fs = 0.25 (in terms of sample rate)
static const IIRCoefficient NUMERATOR_BEST[] = {0.00143137924716997, 0.00789698517354518, 0.0294398110190094, 0.0799173748728548, 0.176072871289035,
	0.323814086626527, 0.511425793167272, 0.702365967828135, 0.847109517379751, 0.901144542299114, 0.847109517379751, 0.702365967828135,
	0.511425793167272, 0.323814086626527, 0.176072871289035, 0.0799173748728548, 0.0294398110190094, 0.00789698517354518, 0.00143137924716997};
static const IIRCoefficient DENOMINATOR_BEST[] = {1.0, -2.69897208543826, 8.80398722907614, -16.1424580924884, 29.5248804330701, -40.1684486817326,
	51.5296017286296, -54.0515071033872, 52.1159369741569, -42.6527147191785, 31.4919185157548, -19.9291740542395, 11.1241159254197,
	-5.25734624012707, 2.10951442619652, -0.68470573213706, 0.17303565409162, -0.0304599797948271, 0.0029192180943142};

// Average elliptic filter with symmetric ripple: N=12, Ap=As=-106 dB, fp=0.193, fs = 0.25 (in terms of sample rate)
static const IIRCoefficient NUMERATOR_GOOD[] = {0.000891054570312711, 0.00401699642072141, 0.0116339860079785, 0.0241764921599365, 0.0395683393387297,
	0.0525279880454076, 0.0576516896473707, 0.0525279880454076, 0.0395683393387297, 0.0241764921599365, 0.0116339860079785, 0.00401699642072141,
	0.000891054570312711};
static const IIRCoefficient DENOMINATOR_GOOD[] = {1.0, -3.75282253489812, 9.02653937439474, -14.9124231385925, 18.9475953813622, -18.8010210368353,
	14.9129109507335, -9.40866788231319, 4.68220224634757, -1.78427646611801, 0.49489467322571, -0.0897350197525806, 0.00808647158647441};

// Fast elliptic filter with symmetric ripple: N=8, Ap=As=-99 dB, fp=0.125, fs = 0.25 (in terms of sample rate)
static const IIRCoefficient NUMERATOR_FAST[] = {0.000919048607199723, 0.00296201761602858, 0.00637178853917259, 0.00955422804027462, 0.0109258558065688,
	0.00955422804027462, 0.00637178853917259, 0.00296201761602858, 0.000919048607199723};
static const IIRCoefficient DENOMINATOR_FAST[] = {1.0, -4.0206974114077, 8.10330349808915, -10.1791188149644, 8.59525285186386, -4.94319220919619,
	1.88033873505286, -0.430744335378428, 0.0453982115557566};

IIRDecimator::C::C(const unsigned int useOrder, const IIRCoefficient useNumerator[], const IIRCoefficient useDenominator[], const Quality quality) {
	if (quality == CUSTOM) {
		order = useOrder;
		numerator = useNumerator;
		denominator = useDenominator;
	} else {
		unsigned int numeratorSize;
		switch (quality) {
		case FAST:
			numeratorSize = sizeof(NUMERATOR_FAST);
			numerator = NUMERATOR_FAST;
			denominator = DENOMINATOR_FAST;
			break;
		case GOOD:
			numeratorSize = sizeof(NUMERATOR_GOOD);
			numerator = NUMERATOR_GOOD;
			denominator = DENOMINATOR_GOOD;
			break;
		case BEST:
			numeratorSize = sizeof(NUMERATOR_BEST);
			numerator = NUMERATOR_BEST;
			denominator = DENOMINATOR_BEST;
			break;
		default:
			numeratorSize = 0;
			break;
		}
		order = (numeratorSize / sizeof(IIRCoefficient)) - 1;
	}
	unsigned int delayLineLength = 2;
	while (delayLineLength < (order + 1)) delayLineLength <<= 1;
	delayLineMask = delayLineLength - 1;
	ringBuffer = new BufferedSample[delayLineLength][IIR_DECIMATOR_CHANNEL_COUNT];
	BufferedSample *s = *ringBuffer;
	BufferedSample *e = ringBuffer[delayLineLength];
	while (s < e) *(s++) = 0;
}

IIRDecimator::IIRDecimator(const Quality quality) :
	c(0, NULL, NULL, quality),
	ringBufferPosition(0),
	phase(1)
{}

IIRDecimator::IIRDecimator(const unsigned int order, const IIRCoefficient numerator[], const IIRCoefficient denominator[]) :
	c(order, numerator, denominator, IIRDecimator::CUSTOM),
	ringBufferPosition(0),
	phase(1)
{}

IIRDecimator::~IIRDecimator() {
	delete[] c.ringBuffer;
}

void IIRDecimator::process(const FloatSample *&inSamples, unsigned int &inLength, FloatSample *&outSamples, unsigned int &outLength) {
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

unsigned int IIRDecimator::estimateInLength(const unsigned int outLength) const {
	return (outLength << 1) + phase;
}

bool IIRDecimator::needNextInSample() const {
	return phase != 0;
}

void IIRDecimator::addInSamples(const FloatSample *&inSamples) {
	ringBufferPosition = (ringBufferPosition - 1) & c.delayLineMask;
	BufferedSample leftSample = *(inSamples++);
	BufferedSample rightSample = *(inSamples++);
	unsigned int delaySampleIx = ringBufferPosition;
	for (unsigned int i = 1; i <= c.order; ++i) {
		delaySampleIx = (delaySampleIx + 1) & c.delayLineMask;
		leftSample -= c.denominator[i] * c.ringBuffer[delaySampleIx][0];
		rightSample -= c.denominator[i] * c.ringBuffer[delaySampleIx][1];
	}
	c.ringBuffer[ringBufferPosition][0] = leftSample;
	c.ringBuffer[ringBufferPosition][1] = rightSample;
	--phase;
}

void IIRDecimator::getOutSamples(FloatSample *&outSamples) {
	BufferedSample leftSample = 0.0;
	BufferedSample rightSample = 0.0;
	unsigned int delaySampleIx = ringBufferPosition;
	for (unsigned int i = 0; i <= c.order; ++i) {
		leftSample += c.numerator[i] * c.ringBuffer[delaySampleIx][0];
		rightSample += c.numerator[i] * c.ringBuffer[delaySampleIx][1];
		delaySampleIx = (delaySampleIx + 1) & c.delayLineMask;
	}
	*(outSamples++) = (FloatSample)leftSample;
	*(outSamples++) = (FloatSample)rightSample;
	phase += 2;
}
