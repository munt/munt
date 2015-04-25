#ifndef SINC_RESAMPLER_H
#define SINC_RESAMPLER_H

#include "FIRInterpolator.h"

class SincResampler {
public:
	SincResampler(const double outputFrequency, const double inputFrequency, const double passbandFrequency, const double stopbandFrequency, const double dbSNR);
	~SincResampler();
	void process(const FloatSample *&inSamples, unsigned int &inLength, FloatSample *&outSamples, unsigned int &outLength) const;
	unsigned int estimateInLength(const unsigned int outLength) const;

private:
	struct I {
		static unsigned int greatestCommonDivisor(unsigned int a, unsigned int b);
		static double bessel(const double x);

		unsigned int upsampleFactor;
		double downsampleFactor;
		double fp;
		double fs;
		double fc;
		double dbRipple;
		double beta;
		unsigned int order;

		void computeResampleFactors(const double inputFrequency, const double outputFrequency);
		void designKaiser();
		const FIRCoefficient * windowedSinc(FIRCoefficient *kernel, const double amp = 1.0) const;
	};

	const struct C {
		const FIRCoefficient *kernel;
		FIRInterpolator *lpf;

		C(const double outputFrequency, const double inputFrequency, const double passbandFrequency, const double stopbandFrequency, const double dbSNR);
	} c;
};

#endif // SINC_RESAMPLER_H
