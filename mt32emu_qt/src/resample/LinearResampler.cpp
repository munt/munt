/* Copyright (C) 2011, 2012, 2013, 2014 Jerome Fisher, Sergey V. Mikayev
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

#include "LinearResampler.h"

#include <QtGlobal>

#include <cmath>

using namespace MT32Emu;

LinearResampler::LinearResampler(Synth *synth, double targetSampleRate) :
	SampleRateConverter(synth, targetSampleRate, SampleRateConverter::SRC_FASTEST),
	inBuffer(new Sample[2 * MAX_SAMPLES_PER_RUN]),
	inLength(),
	position(2.0f) // Preload delay line which effectively makes resampler zero phase
{
	inBufferPtr = inBuffer;
}

LinearResampler::~LinearResampler() {
	delete[] inBuffer;
}

Sample LinearResampler::computeOutSample(Sample prev, Sample next) {
	float interpolatedSample = prev + position * (next - prev);
#if MT32EMU_USE_FLOAT_SAMPLES
	return interpolatedSample;
#else
	// No need to clamp samples here as overshoots are impossible
	return Sample(interpolatedSample + ((interpolatedSample < 0.0f) ? -0.5f : +0.5f));
#endif
}

void LinearResampler::getOutputSamples(Sample *buffer, unsigned int length) {
	while (length > 0) {
		while (1.0 <= position) {
			// Need next sample
			if (inLength == 0) {
				inLength = (Bit32u)ceil(length * inputToOutputRatio);
				inLength = qBound((Bit32u)1, inLength, (Bit32u)MAX_SAMPLES_PER_RUN);
				synth->render(inBuffer, inLength);
				inBufferPtr = inBuffer;
			}
			--position;
			lastLeft = nextLeft;
			nextLeft = *(inBufferPtr++);
			lastRight = nextRight;
			nextRight = *(inBufferPtr++);
			--inLength;
		}
		*(buffer++) = computeOutSample(lastLeft, nextLeft);
		*(buffer++) = computeOutSample(lastRight, nextRight);
		--length;
		position += inputToOutputRatio;
	}
}
