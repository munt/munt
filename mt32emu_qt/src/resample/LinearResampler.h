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
