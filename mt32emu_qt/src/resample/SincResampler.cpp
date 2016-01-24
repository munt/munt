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

#include "SincResampler.h"

#include <cmath>
#include <iostream>

#ifndef M_PI
static const double M_PI = 3.1415926535897931;
#endif

static const double RATIONAL_RATIO_ACCURACY_FACTOR = 1E15;
static const unsigned int MAX_NUMBER_OF_PHASES = 512;

unsigned int SincResampler::I::greatestCommonDivisor(unsigned int a, unsigned int b) {
	while (0 < b) {
		unsigned int r = a % b;
		a = b;
		b = r;
	}
	return a;
}

double SincResampler::I::bessel(const double x) {
	static const double EPS = 1.11E-16;

	double sum = 0.0;
	double f = 1.0;
	for (unsigned int i = 1;; ++i) {
		f *= (0.5 * x / i);
		double f2 = f * f;
		if (f2 <= sum * EPS) break;
		sum += f2;
	}
	return 1.0 + sum;
}

void SincResampler::I::computeResampleFactors(const double inputFrequency, const double outputFrequency) {
	upsampleFactor = (unsigned int)outputFrequency;
	unsigned int downsampleFactorInt = (unsigned int)inputFrequency;
	if ((upsampleFactor == outputFrequency) && (downsampleFactorInt == inputFrequency)) {
		// Input and output frequencies are integers, try to reduce them
		const unsigned int gcd = greatestCommonDivisor(upsampleFactor, downsampleFactorInt);
		if (gcd > 1) {
			upsampleFactor /= gcd;
			downsampleFactor = downsampleFactorInt / gcd;
		} else {
			downsampleFactor = downsampleFactorInt;
		}
		if (upsampleFactor <= MAX_NUMBER_OF_PHASES) return;
	} else {
		// Try to recover rational resample ratio by brute force
		double inputToOutputRatio = inputFrequency / outputFrequency;
		for (unsigned int i = 1; i <= MAX_NUMBER_OF_PHASES; ++i) {
			double testFactor = inputToOutputRatio * i;
			if (floor(RATIONAL_RATIO_ACCURACY_FACTOR * testFactor + 0.5) == RATIONAL_RATIO_ACCURACY_FACTOR * floor(testFactor + 0.5)) {
				// inputToOutputRatio found to be rational within accuracy
				upsampleFactor = i;
				downsampleFactor = floor(testFactor + 0.5);
				return;
			}
		}
	}
	// Use interpolation of FIR taps as a last resort
	upsampleFactor = MAX_NUMBER_OF_PHASES;
	downsampleFactor = MAX_NUMBER_OF_PHASES * inputFrequency / outputFrequency;
}

void SincResampler::I::designKaiser() {
	beta = 0.1102 * (dbRipple - 8.7);
	const double transBW = (fs - fp);
	order = (unsigned int)ceil((dbRipple - 8) / (2.285 * 2 * M_PI * transBW));
}

const FIRCoefficient * SincResampler::I::windowedSinc(FIRCoefficient *kernel, const double amp) const {
	const double fc_pi = M_PI * fc;
	const double recipOrder = 1.0 / order;
	const double mult = 2.0 * fc * amp / bessel(beta);
	for (int i = order, j = 0; 0 <= i; i -= 2, ++j) {
		double xw = i * recipOrder;
		double win = bessel(beta * sqrt(fabs(1.0 - xw * xw)));
		double xs = i * fc_pi;
		double sinc = (i == 0) ? 1.0 : sin(xs) / xs;
		FIRCoefficient imp = FIRCoefficient(mult * sinc * win);
		kernel[j] = imp;
		kernel[order - j] = imp;
	}
	kernel[order + 1] = 0;
	return kernel;
}

SincResampler::C::C(const double outputFrequency, const double inputFrequency, const double passbandFrequency, const double stopbandFrequency, const double dbSNR) {
	I i;
	i.computeResampleFactors(inputFrequency, outputFrequency);
	double baseSamplePeriod = 1.0 / (inputFrequency * i.upsampleFactor);
	i.fp = passbandFrequency * baseSamplePeriod;
	i.fs = stopbandFrequency * baseSamplePeriod;
	i.fc = 0.5 * (i.fp + i.fs);
	i.dbRipple = dbSNR;
	i.designKaiser();
	const unsigned int kernelLength = i.order + 1;
	kernel = i.windowedSinc(new FIRCoefficient[kernelLength + 1], i.upsampleFactor);
	lpf = new FIRInterpolator(i.upsampleFactor, i.downsampleFactor, kernel, kernelLength);
	std::cerr << "FIR: " << i.upsampleFactor << "/" << i.downsampleFactor << ", N=" << kernelLength << ", C=" << 2.0 * i.fc * kernelLength << ", fp=" << i.fp << ", fs=" << i.fs << std::endl;
}

SincResampler::SincResampler(const double outputFrequency, const double inputFrequency, const double passbandFrequency, const double stopbandFrequency, const double dbSNR) :
	c(outputFrequency, inputFrequency, passbandFrequency, stopbandFrequency, dbSNR)
{}

SincResampler::~SincResampler() {
	delete c.lpf;
	delete[] c.kernel;
}

void SincResampler::process(const FloatSample *&inSamples, unsigned int &inLength, FloatSample *&outSamples, unsigned int &outLength) const {
	c.lpf->process(inSamples, inLength, outSamples, outLength);
}

unsigned int SincResampler::estimateInLength(const unsigned int outLength) const {
	return c.lpf->estimateInLength(outLength);
}
