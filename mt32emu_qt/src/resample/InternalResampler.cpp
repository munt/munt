/* Copyright (C) 2015-2016 Sergey V. Mikayev
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

#include "InternalResampler.h"

#include <QtGlobal>
#include <QtDebug>

#include "SincResampler.h"
#include "IIRDecimator.h"

using namespace MT32Emu;

static const double MAX_AUDIBLE_FREQUENCY = 20000.0;
static const double DB_SNR = 106;
static const uint CHANNEL_COUNT = 2;

class CascadeStage {
protected:
	FloatSample buffer[CHANNEL_COUNT * MAX_SAMPLES_PER_RUN];
	const FloatSample *bufferPtr;
	uint size;

public:
	CascadeStage();
};

class SincStage : public CascadeStage {
private:
	Synth &synth;
	SincResampler resampler;

public:
	SincStage(Synth &synth, const double outputFrequency, const double inputFrequency, const double passbandFrequency, const double stopbandFrequency, const double dbSNR);
	void getOutputSamples(FloatSample *buffer, uint length);
};

class IIRStage : public CascadeStage {
private:
	SincStage &sincStage;
	IIRDecimator decimator;

public:
	IIRStage(SincStage &sincStage, const IIRDecimator::Quality quality);
	void getOutputSamples(FloatSample *buffer, uint length);
};

InternalResampler * InternalResampler::createInternalResampler(MT32Emu::Synth *synth, double targetSampleRate, SRCQuality quality) {
	double inSampleRate = synth->getStereoOutputSampleRate();
	bool needIIRStage = true;
	if (MAX_AUDIBLE_FREQUENCY < (0.25 * inSampleRate)) {
		// Oversampled input allows to bypass IIR stage
		needIIRStage = targetSampleRate < (0.5 * inSampleRate);
	} else if (inSampleRate < targetSampleRate) {
		qWarning() << "InternalResampler intended to interpolate oversampled signal only";
		return NULL;
	}
	double sincOutSampleRate, passband, stopband;
	if (needIIRStage) {
		sincOutSampleRate = 2.0 * targetSampleRate;
		passband = 0.5 * targetSampleRate;
		stopband = 1.5 * targetSampleRate;
	} else {
		sincOutSampleRate = targetSampleRate;
		passband = MAX_AUDIBLE_FREQUENCY;
		stopband = 0.5 * inSampleRate + MAX_AUDIBLE_FREQUENCY;
	}
	SincStage *sincStage = new SincStage(*synth, sincOutSampleRate, inSampleRate, passband, stopband, DB_SNR);
	IIRStage *iirStage = needIIRStage ? new IIRStage(*sincStage, IIRDecimator::Quality(quality)) : NULL;
	return new InternalResampler(synth, targetSampleRate, quality, sincStage, iirStage);
}

InternalResampler::InternalResampler(Synth *synth, double targetSampleRate, SRCQuality quality, SincStage * const sincStage, IIRStage * const iirStage) :
	SampleRateConverter(synth, targetSampleRate, quality),
	sincStage(sincStage),
	iirStage(iirStage)
{}

InternalResampler::~InternalResampler() {
	delete iirStage;
	delete sincStage;
}

void InternalResampler::getOutputSamples(Bit16s *outBuffer, uint totalFrames) {
	FloatSample buffer[CHANNEL_COUNT * MAX_SAMPLES_PER_RUN];
	while (totalFrames > 0) {
		uint size = qMin(totalFrames, MAX_SAMPLES_PER_RUN);
		if (iirStage == NULL) {
			sincStage->getOutputSamples(buffer, size);
		} else {
			iirStage->getOutputSamples(buffer, size);
		}
		FloatSample *outs = buffer;
		FloatSample *ends = buffer + CHANNEL_COUNT * size;
		while (outs < ends) {
			FloatSample f = *(outs++) * 16384.0f;
			*(outBuffer++) = (Bit16s)qBound(-0x8000, Bit32s((f < 0.0f) ? f - 0.5f : f + 0.5f), 0x7FFF);
		}
		totalFrames -= size;
	}
}

static void getInputSamples(Synth &synth, FloatSample *outBuffer, uint size) {
	synth.render(outBuffer, size);
}

CascadeStage::CascadeStage() :
	bufferPtr(buffer),
	size()
{}

SincStage::SincStage(Synth &synth, const double outputFrequency, const double inputFrequency, const double passbandFrequency, const double stopbandFrequency, const double dbSNR) :
	synth(synth),
	resampler(outputFrequency, inputFrequency, passbandFrequency, stopbandFrequency, dbSNR)
{}

void SincStage::getOutputSamples(FloatSample *outBuffer, uint length) {
	while (length > 0) {
		if (size == 0) {
			size = resampler.estimateInLength(length);
			size = qBound((uint)1, size, (uint)MAX_SAMPLES_PER_RUN);
			getInputSamples(synth, buffer, size);
			bufferPtr = buffer;
		}
		resampler.process(bufferPtr, size, outBuffer, length);
	}
}

IIRStage::IIRStage(SincStage &sincStage, const IIRDecimator::Quality quality) :
	sincStage(sincStage),
	decimator(quality)
{}

void IIRStage::getOutputSamples(FloatSample *outBuffer, uint length) {
	while (length > 0) {
		if (size == 0) {
			size = decimator.estimateInLength(length);
			size = qBound((uint)1, size, (uint)MAX_SAMPLES_PER_RUN);
			sincStage.getOutputSamples(buffer, size);
			bufferPtr = buffer;
		}
		decimator.process(bufferPtr, size, outBuffer, length);
	}
}
