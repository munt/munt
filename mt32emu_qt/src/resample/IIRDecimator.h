#ifndef IIR_DECIMATOR_H
#define IIR_DECIMATOR_H

static const unsigned int IIR_DECIMATOR_CHANNEL_COUNT = 2;
static const unsigned int IIR_SECTION_ORDER = 2;

typedef float IIRCoefficient;
typedef float BufferedSample;
typedef float FloatSample;

typedef BufferedSample SectionBuffer[IIR_DECIMATOR_CHANNEL_COUNT][IIR_SECTION_ORDER];

// Non-trivial coefficients of a 2nd-order section of a parallel bank
// (zero-order numerator coefficient is always zero, zero-order denominator coefficient is always unity)
struct IIRSection {
	IIRCoefficient num1;
	IIRCoefficient num2;
	IIRCoefficient den1;
	IIRCoefficient den2;
};

class IIRDecimator {
public:
	enum Quality {CUSTOM, FAST, GOOD, BEST};

	IIRDecimator(const Quality quality);
	IIRDecimator(const unsigned int useSectionsCount, const IIRCoefficient useFIR, const IIRSection useSections[]);
	~IIRDecimator();
	void process(const FloatSample *&inSamples, unsigned int &inLength, FloatSample *&outSamples, unsigned int &outLength);
	unsigned int estimateInLength(const unsigned int outLength) const;

private:
	const struct C {
		// Coefficient of the 0-order FIR part
		IIRCoefficient fir;
		// 2nd-order sections that comprise a parallel bank
		const IIRSection *sections;
		// Number of 2nd-order sections
		unsigned int sectionsCount;
		// Delay line per each section
		SectionBuffer *buffer;

		C(const unsigned int useSectionsCount, const IIRCoefficient useFIR, const IIRSection useSections[], const Quality quality);
	} c;
};

#endif // IIR_DECIMATOR_H
