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
#include <cstdlib>
#include <cstring>

#include "mt32emu.h"
#include "mmath.h"

namespace MT32Emu {

//Envelope time keyfollow exponential coefficients
static const double tkcatconst[5] = {0.0, 0.005853144, 0.011148054, 0.019086143, 0.043333215};
static const double tkcatmult[5] = {1.0, 1.058245688, 1.048488989, 1.016049301, 1.097538067};

// Begin filter stuff

// Pre-warp the coefficients of a numerator or denominator.
// Note that a0 is assumed to be 1, so there is no wrapping
// of it.
static void prewarp(double *a1, double *a2, double fc, double fs) {
	double wp;

	wp = 2.0 * fs * tan(DOUBLE_PI * fc / fs);

	*a2 = *a2 / (wp * wp);
	*a1 = *a1 / wp;
}

// Transform the numerator and denominator coefficients
// of s-domain biquad section into corresponding
// z-domain coefficients.
//
//      Store the 4 IIR coefficients in array pointed by coef
//      in following order:
//             beta1, beta2    (denominator)
//             alpha1, alpha2  (numerator)
//
// Arguments:
//             a0-a2   - s-domain numerator coefficients
//             b0-b2   - s-domain denominator coefficients
//             k               - filter gain factor. initially set to 1
//                                and modified by each biquad section in such
//                                a way, as to make it the coefficient by
//                                which to multiply the overall filter gain
//                                in order to achieve a desired overall filter gain,
//                                specified in initial value of k.
//             fs             - sampling rate (Hz)
//             coef    - array of z-domain coefficients to be filled in.
//
// Return:
//             On return, set coef z-domain coefficients
static void bilinear(double a0, double a1, double a2, double b0, double b1, double b2, double *k, double fs, float *coef) {
	double ad, bd;

	// alpha (Numerator in s-domain)
	ad = 4. * a2 * fs * fs + 2. * a1 * fs + a0;
	// beta (Denominator in s-domain)
	bd = 4. * b2 * fs * fs + 2. * b1 * fs + b0;

	// update gain constant for this section
	*k *= ad / bd;

	// Denominator
	*coef++ = (float)((2. * b0 - 8. * b2 * fs * fs) / bd);           // beta1
	*coef++ = (float)((4. * b2 * fs * fs - 2. * b1 * fs + b0) / bd); // beta2

	// Nominator
	*coef++ = (float)((2. * a0 - 8. * a2 * fs * fs) / ad);           // alpha1
	*coef = (float)((4. * a2 * fs * fs - 2. * a1 * fs + a0) / ad);   // alpha2
}

// a0-a2: numerator coefficients
// b0-b2: denominator coefficients
// fc: Filter cutoff frequency
// fs: sampling rate
// k: overall gain factor
// coef: pointer to 4 iir coefficients
static void szxform(double *a0, double *a1, double *a2, double *b0, double *b1, double *b2, double fc, double fs, double *k, float *coef) {
	// Calculate a1 and a2 and overwrite the original values
	prewarp(a1, a2, fc, fs);
	prewarp(b1, b2, fc, fs);
	bilinear(*a0, *a1, *a2, *b0, *b1, *b2, k, fs, coef);
}

static void initFilter(float fs, float fc, float *icoeff, float Q) {
	float *coef;
	double a0, a1, a2, b0, b1, b2;

	double k = 1.0;    // Set overall filter gain factor
	coef = icoeff + 1; // Skip k, or gain

	// Section 1
	a0 = 1.0;
	a1 = 0;
	a2 = 0;
	b0 = 1.0;
	b1 = 0.765367 / Q;
	//b1 = 0.5176387 / Q; // Divide by resonance or Q
	b2 = 1.0;
	szxform(&a0, &a1, &a2, &b0, &b1, &b2, fc, fs, &k, coef);
	coef += 4;         // Point to next filter section

	// Section 2
	a0 = 1.0;
	a1 = 0;
	a2 = 0;
	b0 = 1.0;
	b1 = 1.847759 / Q;
	//b1 = 1.414214  / Q;
	b2 = 1.0;
	szxform(&a0, &a1, &a2, &b0, &b1, &b2, fc, fs, &k, coef);
	coef += 4;         // Point to next filter section

	/*
	// Section 3
	a0 = 1.0;
	a1 = 0;
	a2 = 0;
	b0 = 1.0;
	b1 = 1.931852 / Q;
	b2 = 1.0;
	szxform(&a0, &a1, &a2, &b0, &b1, &b2, fc, fs, &k, coef);
	*/
	icoeff[0] = (float)k;
}

void Tables::initFiltCoeff(float samplerate) {
	for (int j = 0; j < FILTERGRAN; j++) {
		for (int res = 0; res < 31; res++) {
			float tres = resonanceFactor[res];
			initFilter((float)samplerate, (float)j, filtCoeff[j][res], tres);
		}
	}
}

void Tables::initMT32ConstantTables(Synth *synth) {
	int lf;
	synth->printDebug("Initialising Constant Tables");
	for (lf = 0; lf <= 100; lf++) {
		// CONFIRMED:KG: This matches a ROM table found by Mok
		float fVal = (2.0f - LOG10F((float)lf + 1.0f)) * 128.0f;
		int val = (int)(fVal + 1.0);
		if (val > 255) {
			val = 255;
		}
		levelToAmpSubtraction[lf] = (Bit8u)val;
	}

	envLogarithmicTime[0] = 64;
	for (lf = 1; lf <= 255; lf++) {
		// CONFIRMED:KG: This matches a ROM table found by Mok
		envLogarithmicTime[lf] = (Bit8u)ceil(64.0f + LOG2F((float)lf) * 8.0f);
	}

#ifdef EMULATE_LAPC_I // Dummy #ifdef - we'll have runtime emulation mode selection in future.
	// CONFIRMED: Based on a table found by Mok in the LAPC-I control ROM
	// Note that this matches the MT-32 table, but with the values clamped to a maximum of 8.
	memset(masterVolToAmpSubtraction, 8, 71);
	memset(masterVolToAmpSubtraction + 71, 7, 3);
	memset(masterVolToAmpSubtraction + 74, 6, 4);
	memset(masterVolToAmpSubtraction + 78, 5, 3);
	memset(masterVolToAmpSubtraction + 81, 4, 4);
	memset(masterVolToAmpSubtraction + 85, 3, 3);
	memset(masterVolToAmpSubtraction + 88, 2, 4);
	memset(masterVolToAmpSubtraction + 92, 1, 4);
	memset(masterVolToAmpSubtraction + 96, 0, 5);
#else
	// CONFIRMED: Based on a table found by Mok in the MT-32 control ROM
	masterVolToAmpSubtraction[0] = 255;
	for (int masterVol = 1; masterVol <= 100; masterVol++) {
		masterVolToAmpSubtraction[masterVol] = (int)(106.31 - 16.0f * LOG2F((float)masterVol));
	}
#endif

	for (int i = 0; i <= 100; i++) {
		pulseWidth100To255[i] = (int)(i * 255 / 100.0f + 0.5f);
		//synth->printDebug("%d: %d", i, pulseWidth100To255[i]);
	}

	for (int res = 0; res < 31; res++) {
		resonanceFactor[res] = POWF((float)res / 30.0f, 5.0f) + 1.0f;
	}

	for (int i = 0; i < 65536; i++) {
		// Aka (slightly slower): EXP2F(pitchVal / 4096.0f - 16.0f) * 32000.0f
		pitchToFreq[i] = EXP2F(i / 4096.0f - 1.034215715f);
	}
}

Tables::Tables() {
	initialisedSampleRate = 0.0f;
}

bool Tables::init(Synth *synth, PCMWaveEntry *pcmWaves, float sampleRate) {
	if (sampleRate <= 0.0f) {
		synth->printDebug("Bad sampleRate (%f <= 0.0f)", sampleRate);
		return false;
	}
	if (initialisedSampleRate == 0.0f) {
		initMT32ConstantTables(synth);
	}
	if (initialisedSampleRate != sampleRate) {
		initFiltCoeff(sampleRate);
		initialisedSampleRate = sampleRate;
	}
	return true;
}

}
