#ifndef SAMPLE_RATE_CONVERTER_H
#define SAMPLE_RATE_CONVERTER_H

#include <mt32emu/mt32emu.h>

class SampleRateConverter {
public:
	enum SRCQuality {SRC_FASTEST, SRC_FAST, SRC_GOOD, SRC_BEST};

	static MT32Emu::AnalogOutputMode chooseActualAnalogOutputMode(MT32Emu::AnalogOutputMode desiredMode, double targetSampleRate, SampleRateConverter::SRCQuality srcQuality);
	static SampleRateConverter *createSampleRateConverter(MT32Emu::Synth *synth, double targetSampleRate, SRCQuality quality);
	virtual ~SampleRateConverter() {}

	virtual void getOutputSamples(MT32Emu::Sample *buffer, unsigned int length) = 0;
	double getInputToOutputRatio();
	double getOutputToInputRatio();

protected:
	MT32Emu::Synth * const synth;
	const double inputToOutputRatio;
	const double outputToInputRatio;
	const SRCQuality srcQuality;

	SampleRateConverter(MT32Emu::Synth *synth, double targetSampleRate, SRCQuality quality);

private:
	SampleRateConverter(const SampleRateConverter &);
};

#endif // SAMPLE_RATE_CONVERTER_H
