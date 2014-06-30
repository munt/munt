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

#ifndef LINEAR_RESAMPLER_H
#define LINEAR_RESAMPLER_H

#include "SampleRateConverter.h"

class LinearResampler : public SampleRateConverter {
public:
	LinearResampler(MT32Emu::Synth *synth, double targetSampleRate);
	~LinearResampler();
	void getOutputSamples(MT32Emu::Sample *buffer, unsigned int length);

private:
	MT32Emu::Sample * const inBuffer;
	MT32Emu::Sample *inBufferPtr;
	MT32Emu::Bit32u inLength;

	float position;
	MT32Emu::Sample lastLeft, lastRight;

	MT32Emu::Sample computeOutSample(MT32Emu::Sample prev, MT32Emu::Sample next);
};

#endif // LINEAR_RESAMPLER_H
