#ifndef SOXR_ADAPTER_H
#define SOXR_ADAPTER_H

#include <soxr.h>

#include "SampleRateConverter.h"

class SoxrAdapter : public SampleRateConverter {
public:
	SoxrAdapter(MT32Emu::Synth *synth, double targetSampleRate, SRCQuality quality);
	~SoxrAdapter();
	void getOutputSamples(MT32Emu::Bit16s *buffer, unsigned int length);

private:
	MT32Emu::Bit16s * const inBuffer;
	soxr_t resampler;

	static size_t getInputSamples(void *input_fn_state, soxr_in_t *data, size_t requested_len);
};

#endif // SOXR_ADAPTER_H
