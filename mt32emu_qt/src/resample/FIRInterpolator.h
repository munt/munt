#ifndef FIR_INTERPOLATOR_H
#define FIR_INTERPOLATOR_H

typedef float FIRCoefficient;
typedef float FloatSample;

static const unsigned int FIR_INTERPOLATOR_CHANNEL_COUNT = 2;

class FIRInterpolator {
public:
	FIRInterpolator(const unsigned int upsampleFactor, const double downsampleFactor, const FIRCoefficient kernel[], const unsigned int kernelLength);
	~FIRInterpolator();
	void process(const FloatSample *&inSamples, unsigned int &inLength, FloatSample *&outSamples, unsigned int &outLength);
	unsigned int estimateInLength(const unsigned int outLength) const;

private:
	const struct C {
		// Filter coefficients
		const FIRCoefficient *taps;
		// Indicates whether to interpolate filter taps
		bool usePhaseInterpolation;
		// Size of array of filter coefficients
		unsigned int numberOfTaps;
		// Upsampling factor
		unsigned int numberOfPhases;
		// Downsampling factor
		double phaseIncrement;
		// Index of last delay line element, generally greater than numberOfTaps to form a proper binary mask
		unsigned int delayLineMask;
		// Delay line
		FloatSample(*ringBuffer)[FIR_INTERPOLATOR_CHANNEL_COUNT];

		C(const unsigned int upsampleFactor, const double downsampleFactor, const FIRCoefficient kernel[], const unsigned int kernelLength);
	} c;
	// Index of current sample in delay line
	unsigned int ringBufferPosition;
	// Current phase
	double phase;

	bool needNextInSample() const;
	void addInSamples(const FloatSample *&inSamples);
	void getOutSamples(FloatSample *&outSamples);
};

#endif // FIR_INTERPOLATOR_H
