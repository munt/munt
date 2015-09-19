#ifndef SAMPLERATE_ADAPTER_H
#define SAMPLERATE_ADAPTER_H

#include <samplerate.h>

#include "SampleRateConverter.h"

class SamplerateAdapter : public SampleRateConverter {
public:
	SamplerateAdapter(MT32Emu::Synth *synth, double targetSampleRate, SRCQuality quality);
	~SamplerateAdapter();
	void getOutputSamples(MT32Emu::Bit16s *buffer, unsigned int length);

private:
	float * const inBuffer;
	unsigned int inBufferSize;
	SRC_STATE *resampler;

	static long getInputSamples(void *cb_data, float **data);
};

#endif // SAMPLERATE_ADAPTER_H
