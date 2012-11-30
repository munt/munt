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

#include "mt32emu.h"
#include "AReverbModel.h"

// Analysing of state of reverb RAM address lines gives exact sizes of the buffers of filters used. This also indicates that
// the reverb model implemented in the real devices consists of three series allpass filters preceded by a non-feedback comb (or a delay with a LPF)
// and followed by three parallel comb filters

namespace MT32Emu {

// Default reverb settings for modes 0-2

static const Bit32u NUM_ALLPASSES = 3;
static const Bit32u NUM_COMBS = 4;

static const Bit32u MODE_0_ALLPASSES[] = {994, 729, 78};
static const Bit32u MODE_0_COMBS[] = {705, 2349, 2839, 3632};
static const Bit32u MODE_0_OUTL[] = {2349, 141, 1960};
static const Bit32u MODE_0_OUTR[] = {1174, 1570, 145};
static const float  MODE_0_TIMES[] = {-0.25f, -0.45f, -0.6f, -0.75f, -0.8f, -0.85f, -0.9f, -0.95f};
static const float  MODE_0_LEVELS[] = {0.0f, -0.046f, -0.077f, -0.107f, -0.152f, -0.202f, -0.252f, -0.298f};
static const float  MODE_0_LPF_FACTOR = 0.687771f;

static const Bit32u MODE_1_ALLPASSES[] = {1324, 809, 176};
static const Bit32u MODE_1_COMBS[] = {961, 2619, 3545, 4519};
static const Bit32u MODE_1_OUTL[] = {2618, 1760, 4518};
static const Bit32u MODE_1_OUTR[] = {1300, 3532, 2274};
static const float  MODE_1_TIMES[] = {-0.25f, -0.45f, -0.6f, -0.7f, -0.75f, -0.8f, -0.9f, -0.95f};
static const float  MODE_1_LEVELS[] = {0.0f, -0.043f, -0.079f, -0.111f, -0.143f, -0.190f, -0.238f, -0.303f};
static const float  MODE_1_LPF_FACTOR = 0.712025098f;

static const Bit32u MODE_2_ALLPASSES[] = {969, 644, 157};
static const Bit32u MODE_2_COMBS[] = {116, 2259, 2839, 3539};
static const Bit32u MODE_2_OUTL[] = {2259, 718, 1769};
static const Bit32u MODE_2_OUTR[] = {1136, 2128, 1};
static const float  MODE_2_TIMES[] = {-0.225f, -0.4f, -0.55f, -0.62f, -0.72f, -0.83f, -0.86f, -0.935f};
static const float  MODE_2_LEVELS[] = {0.0f, -0.0335f, -0.0614f, -0.0859f, -0.110f, -0.148f, -0.201f, -0.234f};
static const float  MODE_2_LPF_FACTOR = 0.936272247f;

static const AReverbSettings REVERB_MODE_0_SETTINGS = {MODE_0_ALLPASSES, MODE_0_COMBS, MODE_0_OUTL, MODE_0_OUTR, MODE_0_TIMES, MODE_0_LEVELS, MODE_0_LPF_FACTOR};
static const AReverbSettings REVERB_MODE_1_SETTINGS = {MODE_1_ALLPASSES, MODE_1_COMBS, MODE_1_OUTL, MODE_1_OUTR, MODE_1_TIMES, MODE_1_LEVELS, MODE_1_LPF_FACTOR};
static const AReverbSettings REVERB_MODE_2_SETTINGS = {MODE_2_ALLPASSES, MODE_2_COMBS, MODE_2_OUTL, MODE_2_OUTR, MODE_2_TIMES, MODE_2_LEVELS, MODE_2_LPF_FACTOR};

static const AReverbSettings * const REVERB_SETTINGS[] = {&REVERB_MODE_0_SETTINGS, &REVERB_MODE_1_SETTINGS, &REVERB_MODE_2_SETTINGS, &REVERB_MODE_0_SETTINGS};

RingBuffer::RingBuffer(Bit32u newsize) {
	index = 0;
	size = newsize;
	buffer = new float[size];
}

RingBuffer::~RingBuffer() {
	delete[] buffer;
	buffer = NULL;
	size = 0;
}

float RingBuffer::next() {
	if (++index >= size) {
		index = 0;
	}
	return buffer[index];
}

bool RingBuffer::isEmpty() {
	if (buffer == NULL) return true;

	float *buf = buffer;
	float total = 0;
	for (Bit32u i = 0; i < size; i++) {
		total += (*buf < 0 ? -*buf : *buf);
		buf++;
	}
	return ((total / size) < .0002 ? true : false);
}

void RingBuffer::mute() {
	float *buf = buffer;
	for (Bit32u i = 0; i < size; i++) {
		*buf++ = 0;
	}
}

AllpassFilter::AllpassFilter(Bit32u useSize) : RingBuffer(useSize) {}

float AllpassFilter::process(float in) {
	// This model corresponds to the allpass filter implementation of the real CM-32L device
	// found from sample analysis

	float bufferOut = next();

	// store input - feedback / 2
	buffer[index] = in - 0.5f * bufferOut;

	// return buffer output + feedforward / 2
	return bufferOut + 0.5f * buffer[index];
}

CombFilter::CombFilter(Bit32u useSize) : RingBuffer(useSize) {}

void CombFilter::process(const float in) {
	// This model corresponds to the comb filter implementation of the real CM-32L device
	// found from sample analysis

	// the previously stored value
	float last = buffer[index];

	// prepare input + feedback
	float filterIn = in + next() * feedbackFactor;

	// store input + feedback processed by a low-pass filter
	buffer[index] = last + filterFactor * (filterIn - last);
}

float CombFilter::getOutputAt(const Bit32u outIndex) {
	return buffer[(size + index - outIndex) % size];
}

void CombFilter::setFeedbackFactor(const float useFeedbackFactor) {
	feedbackFactor = useFeedbackFactor;
}

void CombFilter::setFilterFactor(const float useFilterFactor) {
	filterFactor = useFilterFactor;
}

AReverbModel::AReverbModel(const ReverbMode mode) : allpasses(NULL), combs(NULL), currentSettings(*REVERB_SETTINGS[mode]) {}

AReverbModel::~AReverbModel() {
	close();
}

void AReverbModel::open(unsigned int /*sampleRate*/) {
	// FIXME: filter sizes must be multiplied by sample rate to 32000Hz ratio
	// IIR filter values depend on sample rate as well
	allpasses = new AllpassFilter*[NUM_ALLPASSES];
	for (Bit32u i = 0; i < NUM_ALLPASSES; i++) {
		allpasses[i] = new AllpassFilter(currentSettings.allpassSizes[i]);
	}
	combs = new CombFilter*[NUM_COMBS];
	for (Bit32u i = 0; i < NUM_COMBS; i++) {
		combs[i] = new CombFilter(currentSettings.combSizes[i]);
		combs[i]->setFilterFactor(currentSettings.filterFactor);
	}
	mute();
}

void AReverbModel::close() {
	if (allpasses != NULL) {
		for (Bit32u i = 0; i < NUM_ALLPASSES; i++) {
			if (allpasses[i] != NULL) {
				delete allpasses[i];
				allpasses[i] = NULL;
			}
		}
		delete[] allpasses;
		allpasses = NULL;
	}
	if (combs != NULL) {
		for (Bit32u i = 0; i < NUM_COMBS; i++) {
			if (combs[i] != NULL) {
				delete combs[i];
				combs[i] = NULL;
			}
		}
		delete[] combs;
		combs = NULL;
	}
}

void AReverbModel::mute() {
	if (allpasses == NULL || combs == NULL) return;
	for (Bit32u i = 0; i < NUM_ALLPASSES; i++) {
		allpasses[i]->mute();
	}
	for (Bit32u i = 0; i < NUM_COMBS; i++) {
		combs[i]->mute();
	}
}

void AReverbModel::setParameters(Bit8u time, Bit8u level) {
// FIXME: wetLevel definitely needs ramping when changed
// Although, most games don't set reverb level during MIDI playback
	if (combs == NULL) return;
	float decayTime = currentSettings.decayTimes[time];
	for (Bit32u i = 0; i < NUM_COMBS; i++) {
		combs[i]->setFeedbackFactor(i == 0 ? 0.0f : decayTime);
	}
	wetLevel = currentSettings.wetLevels[level];
}

bool AReverbModel::isActive() const {
	for (Bit32u i = 0; i < NUM_ALLPASSES; i++) {
		if (!allpasses[i]->isEmpty()) return true;
	}
	for (Bit32u i = 0; i < NUM_COMBS; i++) {
		if (!combs[i]->isEmpty()) return true;
	}
	return false;
}

void AReverbModel::process(const float *inLeft, const float *inRight, float *outLeft, float *outRight, unsigned long numSamples) {
	float dry, link, outL1;

	for (unsigned long i = 0; i < numSamples; i++) {
		dry = *inLeft + *inRight;

		// Get the last stored sample before processing in order not to loose it
		link = combs[0]->getOutputAt(currentSettings.combSizes[0] - 1);

		combs[0]->process(dry);

		link = allpasses[0]->process(link);
		link = allpasses[1]->process(link);
		link = allpasses[2]->process(link);

		// If the output position is equal to the comb size, get it now in order not to loose it
		outL1 = 1.5f * combs[1]->getOutputAt(currentSettings.outLPositions[0] - 1);

		combs[1]->process(link);
		combs[2]->process(link);
		combs[3]->process(link);

		link = outL1 + 1.5f * combs[2]->getOutputAt(currentSettings.outLPositions[1]);
		link += combs[3]->getOutputAt(currentSettings.outLPositions[2]);
		*outLeft = link * wetLevel;

		link = 1.5f * combs[1]->getOutputAt(currentSettings.outRPositions[0]);
		link += 1.5f * combs[2]->getOutputAt(currentSettings.outRPositions[1]);
		link += combs[3]->getOutputAt(currentSettings.outRPositions[2]);
		*outRight = link * wetLevel;

		inLeft++;
		inRight++;
		outLeft++;
		outRight++;
	}
}

}
