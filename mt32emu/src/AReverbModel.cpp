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

using namespace MT32Emu;

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

void RingBuffer::mute() {
	float *buf = buffer;
	for (Bit32u i = 0; i < size; i++) {
		*buf++ = 0;
	}
}

AllPassFilter::AllPassFilter(Bit32u size) : RingBuffer(size) {
}

Delay::Delay(Bit32u size) : RingBuffer(size) {
}

float AllPassFilter::process(float in) {
	// This model corresponds to the allpass filter implementation in the real CM-32L device
	// found from sample analysis

	float out;

	// move to the next position
	index++;
	if (index > size) {
		index = 0;
	}

	// get buffer output
	out = buffer[index];

	// store input - feedback / 2
	buffer[index] = in - 0.5f * out;

	// return buffer output + feedforward / 2
	return out + 0.5f * buffer[index];
}

float Delay::process(float in) {
	// Implements a very simple delay

	float out;

	// move to the next position
	index++;
	if (index > size) {
		index = 0;
	}

	// get buffer output
	out = buffer[index];

	// store input
	buffer[index] = in;

	// return buffer output
	return out;
}

AReverbModel::AReverbModel(const AReverbSettings *newSettings) {
	bActive = false;
	currentSettings = newSettings;
	for (Bit32u i = 0; i < numAllpasses; i++) {
		allpasses[i] = NULL;
	}
	for (Bit32u i = 0; i < numDelays; i++) {
		delays[i] = NULL;
	}
}

AReverbModel::~AReverbModel() {
	close();
}

void AReverbModel::open(unsigned int /*sampleRate*/) {
	// FIXME: filter sizes must be multiplied by sample rate to 32000Hz ratio
	// IIR filter values depend on sample rate as well
	for (Bit32u i = 0; i < numAllpasses; i++) {
		allpasses[i] = new AllPassFilter(currentSettings->allpassSizes[i]);
	}
	for (Bit32u i = 0; i < numDelays; i++) {
		delays[i] = new Delay(currentSettings->delaySizes[i]);
	}
	mute();
}

void AReverbModel::close() {
	for (Bit32u i = 0; i < numAllpasses; i++) {
		if (allpasses[i] != NULL) {
			delete allpasses[i];
			allpasses[i] = NULL;
		}
	}
	for (Bit32u i = 0; i < numDelays; i++) {
		if (delays[i] != NULL) {
			delete delays[i];
			delays[i] = NULL;
		}
	}
}

void AReverbModel::mute() {
	for (Bit32u i = 0; i < numAllpasses; i++) {
		allpasses[i]->mute();
	}
	for (Bit32u i = 0; i < numDelays; i++) {
		delays[i]->mute();
	}
	filterhist1 = 0;
	filterhist2 = 0;
	combhist = 0;
}

void AReverbModel::setParameters(Bit8u time, Bit8u level) {
// FIXME: wetLevel definitely needs ramping when changed
// Although, most games don't set reverb level during MIDI playback
	decayTime = currentSettings->decayTimes[time];
	wetLevel = currentSettings->wetLevels[level];
}

bool AReverbModel::isActive() const {
	return bActive;
}

void AReverbModel::process(const float *inLeft, const float *inRight, float *outLeft, float *outRight, unsigned long numSamples) {
// Three series allpass filters followed by a delay, fourth allpass filter and another delay
	float dry, link, outL1, outL2, outR1, outR2;

	for (unsigned long i = 0; i < numSamples; i++) {
		dry = *inLeft + *inRight;

		// Implementation of 2-stage IIR single-pole low-pass filter
		// found at the entrance of reverb processing on real devices
		filterhist1 += (dry - filterhist1) * currentSettings->filtVal;
		filterhist2 += (filterhist1 - filterhist2) * currentSettings->filtVal;

		link = allpasses[0]->process(filterhist2);
		link = allpasses[1]->process(link);
		link = allpasses[2]->process(link);
		link = delays[0]->process(link);
		outL1 = link;
		link = delays[1]->process(link);
		outR1 = link;

		// this implements a comb filter cross-linked with the fourth allpass filter
		link += combhist * decayTime;
		link = allpasses[3]->process(link);
		link = delays[2]->process(link);
		outL2 = link;
		link = delays[3]->process(link);
		outR2 = link;
		link = delays[4]->process(link);

		// comb filter end point
		combhist = combhist * currentSettings->damp1 + link * currentSettings->damp2;

		*outLeft = (outL1 + outL2) * wetLevel;
		*outRight = (outR1 + outR2) * wetLevel;

		inLeft++;
		inRight++;
		outLeft++;
		outRight++;
	}
}
