#ifndef INTERNAL_RESAMPLER_H
#define INTERNAL_RESAMPLER_H

#include "SampleRateConverter.h"

class SincStage;
class IIRStage;

class InternalResampler : public SampleRateConverter {
public:
	static InternalResampler *createInternalResampler(MT32Emu::Synth *synth, double targetSampleRate, SRCQuality quality);
	~InternalResampler();
	void getOutputSamples(MT32Emu::Sample *buffer, unsigned int length);

private:
	SincStage * const sincStage;
	IIRStage * const iirStage;

	InternalResampler(MT32Emu::Synth *synth, double targetSampleRate, SRCQuality quality, SincStage * const sincStage, IIRStage * const iirStage);
};

#endif // INTERNAL_RESAMPLER_H
