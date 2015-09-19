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

#include "SoxrAdapter.h"

#include <QtCore>

using namespace MT32Emu;

size_t SoxrAdapter::getInputSamples(void *input_fn_state, soxr_in_t *data, size_t requested_len) {
	uint length = qBound((uint)1, (uint)requested_len, MAX_SAMPLES_PER_RUN);
	SoxrAdapter *instance = (SoxrAdapter *)input_fn_state;
	instance->synth->render(instance->inBuffer, length);
	*data = instance->inBuffer;
	return length;
}

SoxrAdapter::SoxrAdapter(Synth *synth, double targetSampleRate, SRCQuality quality) :
	SampleRateConverter(synth, targetSampleRate, quality),
	inBuffer(new Bit16s[2 * MAX_SAMPLES_PER_RUN])
{
	soxr_io_spec_t ioSpec = soxr_io_spec(SOXR_INT16_I, SOXR_INT16_I);
	unsigned long qualityRecipe;
	switch (quality) {
	case SRC_FASTEST:
		qualityRecipe = SOXR_QQ;
		break;
	case SRC_FAST:
		qualityRecipe = SOXR_LQ;
		break;
	case SRC_BEST:
		qualityRecipe = SOXR_16_BITQ;
		break;
	case SRC_GOOD:
	default:
		qualityRecipe = SOXR_MQ;
		break;
	};
	soxr_quality_spec_t qSpec = soxr_quality_spec(qualityRecipe, 0);
	soxr_runtime_spec_t rtSpec = soxr_runtime_spec(1);
	soxr_error_t error;
	resampler = soxr_create(synth->getStereoOutputSampleRate(), targetSampleRate, 2, &error, &ioSpec, &qSpec, &rtSpec);
	if (error != NULL) {
		qDebug() << "SampleRateConverter: Creation of SOXR instance failed:" << soxr_strerror(error);
		soxr_delete(resampler);
		resampler = NULL;
	}
	error = soxr_set_input_fn(resampler, getInputSamples, this, MAX_SAMPLES_PER_RUN);
	if (error != NULL) {
		qDebug() << "SampleRateConverter: Installing sample feed for SOXR failed:" << soxr_strerror(error);
		soxr_delete(resampler);
		resampler = NULL;
	}
}

SoxrAdapter::~SoxrAdapter() {
	delete[] inBuffer;
	soxr_delete(resampler);
}

void SoxrAdapter::getOutputSamples(Bit16s *buffer, uint length) {
	if (resampler == NULL) {
		Synth::muteSampleBuffer(buffer, 2 * length);
		return;
	}
	while (length > 0) {
		size_t gotFrames = soxr_output(resampler, buffer, (size_t)length);
		soxr_error_t error = soxr_error(resampler);
		if (error != NULL) {
			qDebug() << "SampleRateConverter: SOXR error during processing:" << soxr_strerror(error) << "> resetting";
			error = soxr_clear(resampler);
			if (error != NULL) {
				qDebug() << "SampleRateConverter: SOXR failed to reset:" << soxr_strerror(error);
				soxr_delete(resampler);
				resampler = NULL;
				Synth::muteSampleBuffer(buffer, 2 * length);
				return;
			}
			continue;
		}
		if (gotFrames == 0) {
			qDebug() << "SampleRateConverter: got 0 frames from SOXR, weird";
		}
		buffer += gotFrames << 1;
		length -= (uint)gotFrames;
	}
}
