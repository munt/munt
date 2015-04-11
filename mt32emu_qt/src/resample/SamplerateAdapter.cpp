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

#include "SamplerateAdapter.h"

#include <QtCore>

using namespace MT32Emu;

long SamplerateAdapter::getInputSamples(void *cb_data, float **data) {
	SamplerateAdapter *instance = (SamplerateAdapter *)cb_data;
	uint length = qBound((uint)1, instance->inBufferSize, MAX_SAMPLES_PER_RUN);
#if MT32EMU_USE_FLOAT_SAMPLES
	instance->synth->render(instance->inBuffer, length);
#else
	Sample inSampleBuffer[2 * MAX_SAMPLES_PER_RUN];
	instance->synth->render(inSampleBuffer, length);
	src_short_to_float_array(inSampleBuffer, instance->inBuffer, int(2 * length));
#endif
	*data = instance->inBuffer;
	return length;
}

SamplerateAdapter::SamplerateAdapter(Synth *synth, double targetSampleRate, SRCQuality quality) :
	SampleRateConverter(synth, targetSampleRate, quality),
	inBuffer(new float[2 * MAX_SAMPLES_PER_RUN]),
	inBufferSize(MAX_SAMPLES_PER_RUN)
{
	int error;
	int conversionType;
	switch (quality) {
	case SRC_FASTEST:
		conversionType = SRC_LINEAR;
		break;
	case SRC_FAST:
		conversionType = SRC_SINC_FASTEST;
		break;
	case SRC_BEST:
		conversionType = SRC_SINC_BEST_QUALITY;
		break;
	case SRC_GOOD:
	default:
		conversionType = SRC_SINC_MEDIUM_QUALITY;
		break;
	};
	resampler = src_callback_new(getInputSamples, conversionType, 2, &error, this);
	if (error != 0) {
		qDebug() << "SampleRateConverter: Creation of Samplerate instance failed:" << src_strerror(error);
		src_delete(resampler);
		resampler = NULL;
	}
}

SamplerateAdapter::~SamplerateAdapter() {
	delete[] inBuffer;
	src_delete(resampler);
}

void SamplerateAdapter::getOutputSamples(MT32Emu::Sample *buffer, unsigned int length) {
	if (resampler == NULL) {
		Synth::muteSampleBuffer(buffer, 2 * length);
		return;
	}
#if MT32EMU_USE_FLOAT_SAMPLES
	float *outBufferPtr = buffer;
#else
	float outBuffer[2 * MAX_SAMPLES_PER_RUN];
	float *outBufferPtr = outBuffer;
#endif
	const uint totalLength = length;
	while (length > 0) {
		inBufferSize = uint(length * inputToOutputRatio + 0.5);
		long gotFrames = src_callback_read(resampler, outputToInputRatio, (long)length, outBufferPtr);
		int error = src_error(resampler);
		if (error != 0) {
			qDebug() << "SampleRateConverter: Samplerate error during processing:" << src_strerror(error) << "> resetting";
			error = src_reset(resampler);
			if (error != 0) {
				qDebug() << "SampleRateConverter: Samplerate failed to reset:" << src_strerror(error);
				src_delete(resampler);
				resampler = NULL;
				Synth::muteSampleBuffer(buffer, 2 * length);
				return;
			}
			continue;
		}
		if (gotFrames <= 0) {
			qDebug() << "SampleRateConverter: got" << gotFrames << "frames from Samplerate, weird";
		}
		outBufferPtr += gotFrames << 1;
		length -= gotFrames;
	}
#if !MT32EMU_USE_FLOAT_SAMPLES
	src_float_to_short_array(outBuffer, buffer, int(2 * totalLength));
#endif
}
