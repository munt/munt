// Reverb model declaration
//
// Written by Jezar at Dreampoint, June 2000
// Modifications by Jerome Fisher, 2009
// http://www.dreampoint.co.uk
// This code is public domain

#ifndef _revmodel_
#define _revmodel_

#include "filters.h"

// Reverb model tuning values
const int   numcombs        = 8;
const int   numallpasses    = 4;
const float muted           = 0;
const float fixedgain       = 0.015f;
const float scalewet        = 3;
const float scaledry        = 2;
const float scaledamp       = 0.4f;
const float scaleroom       = 0.28f;
const float offsetroom      = 0.7f;
const float initialroom     = 0.5f;
const float initialdamp     = 0.5f;
const float initialwet      = 1/scalewet;
const float initialdry      = 0;
const float initialwidth    = 1;
const float initialmode     = 0;
const float freezemode      = 0.5f;
const int   stereospread    = 23;

const int combtuning[]      = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
const int allpasstuning[]   = {556, 441, 341, 225};

class revmodel
{
public:
			       revmodel(float scaletuning);
						 ~revmodel();
			void   mute();
			void   process(const float *inputL, const float *inputR, float *outputL, float *outputR, long numsamples);
			void   setroomsize(float value);
			float  getroomsize();
			void   setdamp(float value);
			float  getdamp();
			void   setwet(float value);
			float  getwet();
			void   setdry(float value);
			float  getdry();
			void   setwidth(float value);
			float  getwidth();
			void   setmode(float value);
			float  getmode();
			void   setfiltval(float value);
private:
			void   update();
private:
	float  gain;
	float  roomsize,roomsize1;
	float  damp,damp1;
	float  wet,wet1,wet2;
	float  dry;
	float  width;
	float  mode;

	// LPF stuff
	float filtval;
	float filtprev1;
	float filtprev2;

	// Comb filters
	comb   combL[numcombs];
	comb   combR[numcombs];

	// Allpass filters
	allpass	allpassL[numallpasses];
	allpass	allpassR[numallpasses];
};

#endif//_revmodel_
