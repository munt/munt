#ifndef LINEAR_RESAMPLER_H
#define LINEAR_RESAMPLER_H

#include "SampleRateConverter.h"

class LinearResampler : public SampleRateConverter {
public:
	LinearResampler(MT32Emu::Synth *synth, double targetSampleRate);
	~LinearResampler();
	void getOutputSamples(MT32Emu::Bit16s *buffer, unsigned int length);

private:
	MT32Emu::Bit16s * const inBuffer;
	MT32Emu::Bit16s *inBufferPtr;
	MT32Emu::Bit32u inLength;

	float position;
	MT32Emu::Bit16s lastLeft, lastRight;
	MT32Emu::Bit16s nextLeft, nextRight;

	MT32Emu::Bit16s computeOutSample(MT32Emu::Bit16s prev, MT32Emu::Bit16s next);
};

#endif // LINEAR_RESAMPLER_H
