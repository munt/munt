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

	for (int i = 0; i < 65536; i++) {
		// Aka (slightly slower): EXP2F(pitchVal / 4096.0f - 16.0f) * 32000.0f
		pitchToFreq[i] = EXP2F(i / 4096.0f - 1.034215715f);
	}
}

Tables::Tables() {
	initialisedSampleRate = 0.0f;
}

bool Tables::init(Synth *synth, float sampleRate) {
	if (sampleRate <= 0.0f) {
		synth->printDebug("Bad sampleRate (%f <= 0.0f)", sampleRate);
		return false;
	}
	if (initialisedSampleRate == 0.0f) {
		initMT32ConstantTables(synth);
	}
	if (initialisedSampleRate != sampleRate) {
		initialisedSampleRate = sampleRate;
	}
	return true;
}

}
