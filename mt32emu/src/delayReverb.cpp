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

#include <cstring>
#include "mt32emu.h"
#include "delayReverb.h"

using namespace MT32Emu;

const float REVERB_DELAY[8] = {0.012531f, 0.0195f, 0.03f, 0.0465625f, 0.070625f, 0.10859375f, 0.165f, 0.25f};
const float REVERB_FADE[8] = {0.0f, 0.072265218f, 0.120255297f, 0.192893979f, 0.288687407f, 0.384667566f, 0.504922864f, 0.745338317f};
const float REVERB_FEEDBACK = -175.0f / 256.0f;
const float LPF_VALUE = 0.594603558f; // = EXP2F(-0.75f)

DelayReverb::DelayReverb() {
	bufLeft = NULL;
	bufRight = NULL;
	sampleRate = 0;
	resetParameters();
}

DelayReverb::~DelayReverb() {
	delete[] bufLeft;
	delete[] bufRight;
}

void DelayReverb::setSampleRate(unsigned int newSampleRate) {
	if (newSampleRate != sampleRate) {
		sampleRate = newSampleRate;

		delete[] bufLeft;
		delete[] bufRight;

		// FIXME: set bufSize to EXP2F(ceil(log2(bufSize))) and use & instead of % to find buf indexes to speedup
		bufSize = Bit32u(2.0f * REVERB_DELAY[7] * sampleRate);
		bufLeft = new float[bufSize];
		bufRight = new float[bufSize];

		reset();
	}
}

void DelayReverb::setParameters(Bit8u /*mode*/, Bit8u time, Bit8u level) {
	delay = Bit32u(REVERB_DELAY[time] * sampleRate);
	fade = REVERB_FADE[level];
	resetBuffer();
}

void DelayReverb::process(const float *inLeft, const float *inRight, float *outLeft, float *outRight, unsigned long numSamples) {
	if ((bufLeft == NULL) || (bufRight == NULL)) {
		return;
	}

	for (unsigned int sampleIx = 0; sampleIx < numSamples; sampleIx++) {

		//FIXME: for speed burst % should be replaced by &
		Bit32u bufIxP1 = (bufSize + bufIx + 1) % bufSize;
		Bit32u bufIxMDelay = (bufSize + bufIx - delay) % bufSize;
		Bit32u bufIxM2Delay = (bufSize + bufIx - delay - delay) % bufSize;

		float left = REVERB_FEEDBACK * bufLeft[bufIxM2Delay];
		float right = REVERB_FEEDBACK * bufRight[bufIxM2Delay];

		bufLeft[bufIxP1] = bufLeft[bufIx] + (left - bufLeft[bufIx]) * LPF_VALUE;
		bufRight[bufIxP1] = bufRight[bufIx] + (right - bufRight[bufIx]) * LPF_VALUE;

		outLeft[sampleIx] = bufLeft[bufIxP1];
		outRight[sampleIx] = bufRight[bufIxP1];

		left = inLeft[sampleIx] * fade;
		right = inRight[sampleIx] * fade;

		bufLeft[bufIxMDelay] += right;
		bufRight[bufIxMDelay] += left;
		bufLeft[bufIx] += left;
		bufRight[bufIx] += right;

		bufIx++;
		bufIx %= bufSize;
	}
}

void DelayReverb::reset() {
	resetBuffer();
	resetParameters();
}

void DelayReverb::resetBuffer() {
	bufIx = 0;
	if (bufLeft != NULL) {
		memset(bufLeft, 0, bufSize * sizeof(float));
	}
	if (bufRight != NULL) {
		memset(bufRight, 0, bufSize * sizeof(float));
	}
}

void DelayReverb::resetParameters() {
	delay = REVERB_DELAY[0];
	fade = REVERB_FADE[0];
}
