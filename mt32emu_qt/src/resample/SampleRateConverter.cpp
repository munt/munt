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

#if defined WITH_LIBSOXR_RESAMPLER
#include "SoxrAdapter.h"
#elif defined WITH_LIBSAMPLERATE_RESAMPLER
#include "SamplerateAdapter.h"
#else
#include "LinearResampler.h"
#endif

using namespace MT32Emu;

SampleRateConverter *SampleRateConverter::createSampleRateConverter(Synth *synth, double targetSampleRate) {
#if defined WITH_LIBSOXR_RESAMPLER
	return new SoxrAdapter(synth, targetSampleRate);
#elif defined WITH_LIBSAMPLERATE_RESAMPLER
	return new SamplerateAdapter(synth, targetSampleRate);
#else
	return new LinearResampler(synth, targetSampleRate);
#endif
}

SampleRateConverter::SampleRateConverter(Synth *useSynth, double targetSampleRate) :
	synth(useSynth),
	inputToOutputRatio(SAMPLE_RATE / targetSampleRate),
	outputToInputRatio(targetSampleRate / SAMPLE_RATE)
{}

double SampleRateConverter::getInputToOutputRatio() {
	return inputToOutputRatio;
}

double SampleRateConverter::getOutputToInputRatio() {
	return outputToInputRatio;
}
