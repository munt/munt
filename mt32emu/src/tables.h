/* Copyright (C) 2003-2009 Dean Beeler, Jerome Fisher
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

#ifndef MT32EMU_TABLES_H
#define MT32EMU_TABLES_H

#include "blit/BlitSaw.h"
#include "blit/BlitSquare.h"

namespace MT32Emu {

// Mathematical constants
const double DOUBLE_PI = 3.1415926535897932384626433832795;
const double DOUBLE_LN_10 = 2.3025850929940456840179914546844;
const float FLOAT_PI = 3.1415926535897932384626433832795f;
const float FLOAT_LN_10 = 2.3025850929940456840179914546844f;
const float FLOAT_LN_2 = 0.6931472f;

// Filter settings
//const int FILTERGRAN = 512;
const int FILTERGRAN = 32768;


// Amplitude of waveform generator
// FIXME: This value is the amplitude possible whilst avoiding
// overdriven values immediately after filtering when playing
// back SQ3MT.MID. Needs to be checked.
const int WGAMP = 12382;

const int MIDDLEC = 60;
const int MIDDLEA = 69; // By this I mean "A above middle C"

// FIXME:KG: may only need to do 12 to 108
// 12..108 is the range allowed by note on commands, but the key can be modified by pitch keyfollow
// and adjustment for timbre pitch, so the results can be outside that range.
// Should we move it (by octave) into the 12..108 range, or keep it in 0..127 range,
// or something else altogether?
const int LOWEST_NOTE = 12;
const int HIGHEST_NOTE = 127;
const int NUM_NOTES = HIGHEST_NOTE - LOWEST_NOTE + 1; // Number of slots for note LUT

static const int romMultKeyfollow[17] = {
	-8192, -4096, -2048, 0, 1024, 2048, 3072, 4096, 5120, 6144, 7168, 8192, 10240, 12288, 16384, 8198, 8226
};

static const int filtMultKeyfollow[17] = {
	-21, -10, -5, 0, 2, 5, 8, 10, 13, 16, 18, 21, 26, 32, 42, 21, 21
};

static const int BiasLevel_MulTable[15] = {
	85, 42, 21, 16, 10, 5, 2, 0, -2, -5, -10, -16, -21, -74, -85
};

static const int pitchROMTable[12] = {
	0, 341, 683, 1024, 1365, 1707, 2048, 2389, 2731, 3072, 3413, 3755
};


class Synth;

struct NoteLookup {
	float freq;
	Bit32u div2;
	Bit32u *wavTable;
};

struct KeyLookup {
	Bit32s envTimeMult[5]; // For envelope time adjustment for key pressed
};

class Tables {
	float initialisedSampleRate;
	float initialisedMasterTune;
	void initMT32ConstantTables(Synth *synth);
	static Bit16s clampWF(Synth *synth, const char *n, float ampVal, double input);
	static File *initWave(Synth *synth, NoteLookup *noteLookup, float ampsize, float div2, File *file);
	bool initNotes(Synth *synth, PCMWaveEntry *pcmWaves, float rate, float tuning);
	void initEnvelopes(float sampleRate);
	void initFiltCoeff(float samplerate);
public:
	// Constant LUTs

	// CONFIRMED: This is used to convert several parameters to amp-modifying values in the TVA envelope:
	// - PatchTemp.outputLevel
	// - RhythmTemp.outlevel
	// - PartialParam.tva.level
	// - expression
	// It's used to determine how much to subtract from the amp envelope's target value
	Bit8u levelToAmpSubtraction[101];

	// CONFIRMED: ...
	Bit8u envLogarithmicTime[256];

	// CONFIRMED: ...
	Bit8u masterVolToAmpSubtraction[101];

	Bit32s tvfKeyfollowMult[217];
	Bit16s noiseBuf[MAX_SAMPLE_OUTPUT];
	Bit16s sintable[65536];
	Bit32s pitchEnvVal[16][101];
	Bit32s envTimeVelfollowMult[5][128];
	Bit32s pwVelfollowAdd[15][128];
	float resonanceFactor[31];
	Bit32u lfoShift[101][101];
	float pwFactorf[101];

	// LUTs varying with sample rate
	Bit32u envTime[101];
	Bit32u envDeltaMaxTime[101];
	Bit32u envDecayTime[101];
	Bit32u lfoPeriod[101];
	float filtCoeff[FILTERGRAN][31][8];

	// Various LUTs for each note and key
	NoteLookup noteLookups[NUM_NOTES];
	KeyLookup keyLookups[97];

	Tables();
	bool init(Synth *synth, PCMWaveEntry *pcmWaves, float sampleRate, float masterTune);
	void initNote(Synth *synth, NoteLookup *noteLookup, float note, float rate, float tuning, PCMWaveEntry *pcmWaves);
	void freeNotes();
};

}

#endif
