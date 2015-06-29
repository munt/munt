/* Copyright (C) 2011-2015 Jerome Fisher, Sergey V. Mikayev
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

#include <QtGlobal>
#include <QtDebug>

#if defined WITH_LIBSOXR_RESAMPLER
#include "SoxrAdapter.h"
#elif defined WITH_LIBSAMPLERATE_RESAMPLER
#include "SamplerateAdapter.h"
#else
#include "InternalResampler.h"
#include "LinearResampler.h"
#endif

using namespace MT32Emu;

AnalogOutputMode SampleRateConverter::chooseActualAnalogOutputMode(AnalogOutputMode desiredMode, double targetSampleRate, SampleRateConverter::SRCQuality srcQuality) {
#if !defined WITH_LIBSOXR_RESAMPLER && !defined WITH_LIBSAMPLERATE_RESAMPLER
	if (srcQuality != SRC_FASTEST && desiredMode != AnalogOutputMode_DIGITAL_ONLY && targetSampleRate > 0.0) {
		if (Synth::getStereoOutputSampleRate(AnalogOutputMode_ACCURATE) < targetSampleRate) {
			desiredMode = AnalogOutputMode_OVERSAMPLED;
		} else if (Synth::getStereoOutputSampleRate(AnalogOutputMode_COARSE) < targetSampleRate) {
			desiredMode = AnalogOutputMode_ACCURATE;
		} else {
			desiredMode = AnalogOutputMode_COARSE;
		}
	}
#else
	Q_UNUSED(targetSampleRate)
	Q_UNUSED(srcQuality)
#endif
	return desiredMode;
}

SampleRateConverter *SampleRateConverter::createSampleRateConverter(Synth *synth, double targetSampleRate, SRCQuality quality) {
#if defined WITH_LIBSOXR_RESAMPLER
	return new SoxrAdapter(synth, targetSampleRate, quality);
#elif defined WITH_LIBSAMPLERATE_RESAMPLER
	return new SamplerateAdapter(synth, targetSampleRate, quality);
#else
	if (quality != SRC_FASTEST) {
		InternalResampler *resampler = InternalResampler::createInternalResampler(synth, targetSampleRate, quality);
		if (resampler != NULL) return resampler;
		qWarning() << "Unable to create InternalResampler, fall back to LinearResampler";
	}
	return new LinearResampler(synth, targetSampleRate);
#endif
}

SampleRateConverter::SampleRateConverter(Synth *useSynth, double targetSampleRate, SRCQuality quality) :
	synth(useSynth),
	inputToOutputRatio(useSynth->getStereoOutputSampleRate() / targetSampleRate),
	outputToInputRatio(targetSampleRate / useSynth->getStereoOutputSampleRate()),
	srcQuality(quality)
{}

double SampleRateConverter::getInputToOutputRatio() {
	return inputToOutputRatio;
}

double SampleRateConverter::getOutputToInputRatio() {
	return outputToInputRatio;
}
