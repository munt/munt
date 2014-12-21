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

#ifndef SAMPLE_RATE_CONVERTER_H
#define SAMPLE_RATE_CONVERTER_H

#include <mt32emu/mt32emu.h>

class SampleRateConverter {
public:
	enum SRCQuality {SRC_FASTEST, SRC_FAST, SRC_GOOD, SRC_BEST};

	static SampleRateConverter *createSampleRateConverter(MT32Emu::Synth *synth, double targetSampleRate, SRCQuality quality);
	virtual ~SampleRateConverter() {}

	virtual void getOutputSamples(MT32Emu::Sample *buffer, unsigned int length) = 0;
	double getInputToOutputRatio();
	double getOutputToInputRatio();

protected:
	MT32Emu::Synth * const synth;
	const double inputToOutputRatio;
	const double outputToInputRatio;
	const SRCQuality srcQuality;

	SampleRateConverter(MT32Emu::Synth *synth, double targetSampleRate, SRCQuality quality);

private:
	SampleRateConverter(const SampleRateConverter &);
};

#endif // SAMPLE_RATE_CONVERTER_H
