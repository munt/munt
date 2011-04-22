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
#include <cstring>
#include "mt32emu.h"
#include "delayReverb.h"

using namespace MT32Emu;


// The values below are found via analysis of digital samples

const float REVERB_DELAY[8] = {0.012531f, 0.0195f, 0.03f, 0.0465625f, 0.070625f, 0.10859375f, 0.165f, 0.25f};
const float REVERB_FADE[8] = {0.0f, 0.072265218f, 0.120255297f, 0.192893979f, 0.288687407f, 0.384667566f, 0.504922864f, 0.745338317f};
const float REVERB_FEEDBACK = -175.0f / 256.0f;
const float LPF_VALUE = 0.594603558f; // = EXP2F(-0.75f)

DelayReverb::DelayReverb() {
	buf = NULL;
	sampleRate = 0;
	resetParameters();
}

DelayReverb::~DelayReverb() {
	delete[] buf;
}

void DelayReverb::setSampleRate(unsigned int newSampleRate) {
	if (newSampleRate != sampleRate) {
		sampleRate = newSampleRate;

		delete[] buf;

		// If we ever need a speedup, set bufSize to EXP2F(ceil(log2(bufSize))) and use & instead of % to find buf indexes
		bufSize = Bit32u(2.0f * REVERB_DELAY[7] * sampleRate);
		buf = new float[bufSize];

		reset();
	}
}

void DelayReverb::setParameters(Bit8u /*mode*/, Bit8u time, Bit8u level) {

	// Time in samples between impulse responses
	delay = Bit32u(REVERB_DELAY[time] * sampleRate);

	// Fading speed, i.e. amplitude ratio of neighbor responses
	fade = REVERB_FADE[level];
	resetBuffer();
}

void DelayReverb::process(const float *inLeft, const float *inRight, float *outLeft, float *outRight, unsigned long numSamples) {
	if (buf == NULL) {
		return;
	}

	for (unsigned int sampleIx = 0; sampleIx < numSamples; sampleIx++) {

		// Since speed isn't likely an issue here, we use a simple approach for ring buffer indexing
		Bit32u bufIxM1 = (bufSize + bufIx - 1) % bufSize;
		Bit32u bufIxMDelay = (bufSize + bufIx - delay) % bufSize;
		Bit32u bufIxM2Delay = (bufSize + bufIx - delay - delay) % bufSize;

		// Attenuated input samples and feedback response are directly added to the current ring buffer location
		float sample = REVERB_FEEDBACK * ((inLeft[sampleIx] + inRight[sampleIx]) * fade + buf[bufIxM2Delay]);

		// Single-pole IIR filter found on real devices
		buf[bufIx] = buf[bufIxM1] + (sample - buf[bufIxM1]) * LPF_VALUE;

		// Output left channel by Delay samples earlier
		outLeft[sampleIx] = buf[bufIxMDelay];
		outRight[sampleIx] = buf[bufIxM2Delay];

		bufIx = (bufIx + 1) % bufSize;
	}
}

void DelayReverb::reset() {
	resetBuffer();
	resetParameters();
}

void DelayReverb::resetBuffer() {
	bufIx = 0;
	if (buf != NULL) {
		memset(buf, 0, bufSize * sizeof(float));
	}
}

void DelayReverb::resetParameters() {
	delay = Bit32u(REVERB_DELAY[0] * sampleRate);
	fade = REVERB_FADE[0];
}

bool DelayReverb::isActive() const {
	// Quick hack: Return true iff all samples in the left buffer are the same and
	// all samples in the right buffers are the same (within the sample output threshold).
	if (bufSize == 0) {
		return false;
	}
	Bit32s last = (Bit32s)floor(buf[0] * 8192.0f);
	for (unsigned int i = 1; i < bufSize; i++) {
		Bit32s s = (Bit32s)floor(buf[i] * 8192.0f);
		if (s != last) {
			return true;
		}
	}
	return false;
}
