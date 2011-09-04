// Filter declarations
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// Cleaned up to reduce some of the code size
// This code is public domain

#ifndef _filters_
#define _filters_

// Based on IS_DENORMAL macro by Jon Watte
static inline float undenormalise(float x) {
	union {
		float f;
		unsigned int i;
	} u;
	u.f = x;
	if ((u.i & 0x7f800000) == 0) {
		return 0.0f;
	}
	return x;
}

class BaseFilter {
public:
	BaseFilter() : bufidx(0) {}
	~BaseFilter() {}

	void setbuffer(float *buf, int size) {
		buffer = buf;
		bufsize = size;
	}

	void deletebuffer() {
		delete[] buffer;
		buffer = 0;
	}

	void mute() {
		for (int i = 0; i < bufsize; i++)
			buffer[i] = 0;
	}

	void setfeedback(float val) {
		feedback = val;
	}

	float getfeedback() {
		return feedback;
	}

protected:
	float   feedback;
	float   *buffer;
	int     bufsize;
	int     bufidx;
};

class allpass : public BaseFilter
{
public:
	allpass() {}

	// Big to inline - but crucial for speed
	inline  float   process(float input) {
		float bufout = undenormalise(buffer[bufidx]);
		float output = -input + bufout;

		buffer[bufidx] = input + (bufout*feedback);

		if (++bufidx>=bufsize) bufidx = 0;

		return output;
	}
};

class comb : public BaseFilter
{
public:
	comb() : filterstore(0) {}

	// Big to inline - but crucial for speed
	inline  float   process(float input) {
		float output = undenormalise(buffer[bufidx]);

		filterstore = undenormalise((output*damp2) + (filterstore*damp1));

		buffer[bufidx] = input + (filterstore*feedback);

		if (++bufidx>=bufsize) bufidx = 0;

		return output;
	}

	void    setdamp(float val) {
		damp1 = val;
		damp2 = 1-val;
	}

	float   getdamp() {
		return damp1;
	}

private:
	float   filterstore;
	float   damp1;
	float   damp2;
};


#endif //_filters_
