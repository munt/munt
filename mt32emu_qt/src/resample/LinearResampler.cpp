/* Copyright (C) 2011, 2012, 2013, 2014 Jerome Fisher, Sergey V. Mikayev
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

#include "LinearResampler.h"

#include <QtGlobal>

using namespace MT32Emu;

LinearResampler::LinearResampler(Synth *synth, double targetSampleRate) :
	SampleRateConverter(synth, targetSampleRate, SampleRateConverter::SRC_FASTEST),
	inBuffer(new Sample[2 * MAX_SAMPLES_PER_RUN]),
	position(1.0f), // Effectively makes resampler zero phase
	lastLeft(),
	lastRight()
{
	inBufferPtr = inBuffer;
	inLength = 0;
}

LinearResampler::~LinearResampler() {
	delete[] inBuffer;
}

Sample LinearResampler::computeOutSample(Sample prev, Sample next) {
	float interpolatedSample = prev + position * (next - prev);
#if MT32EMU_USE_FLOAT_SAMPLES
	return interpolatedSample;
#else
	return Sample(interpolatedSample + ((interpolatedSample < 0) ? -0.5f : +0.5f));
#endif
}

void LinearResampler::getOutputSamples(Sample *buffer, unsigned int length) {
	while (length > 0) {
		if (inLength == 0) {
			inLength = Bit32u(length * inputToOutputRatio + 0.5);
			inLength = qBound((Bit32u)1, inLength, (Bit32u)MAX_SAMPLES_PER_RUN);
			synth->render(inBuffer, inLength);
			inBufferPtr = inBuffer;
		}
		Sample nextLeft = *(inBufferPtr++);
		Sample nextRight = *(inBufferPtr++);
		--inLength;
		while ((position < 1.0) && (length > 0)) {
			*(buffer++) = computeOutSample(lastLeft, nextLeft);
			*(buffer++) = computeOutSample(lastRight, nextRight);
			--length;
			position += inputToOutputRatio;
		}
		--position;
		lastLeft = nextLeft;
		lastRight = nextRight;
	}
}
