/* Copyright (C) 2015-2016 Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "IIRDecimator.h"

#include <cmath>
#include <cstddef>

#ifndef M_PI
static const double M_PI = 3.1415926535897931;
#endif

// Sharp elliptic filter with symmetric ripple: N=18, Ap=As=-106 dB, fp=0.238, fs = 0.25 (in terms of sample rate)
static const IIRCoefficient FIR_BEST = 0.0014313792470984f;
static const IIRSection SECTIONS_BEST[] = {
	{ 2.85800356692148000f,-0.2607342682253230f,-0.602478421807085f, 0.109823442522145f},
	{-4.39519408383016000f, 1.4651975326003500f,-0.533817668127954f, 0.226045921792036f},
	{ 0.86638550740991800f,-2.1053851417898500f,-0.429134968401065f, 0.403512574222174f},
	{ 1.67161485530774000f, 0.7963595880494520f,-0.324989203363446f, 0.580756666711889f},
	{-1.19962759276471000f, 0.5873595178851540f,-0.241486447489019f, 0.724264899930934f},
	{ 0.01631779946479250f,-0.6282334739461620f,-0.182766025706656f, 0.827774001858882f},
	{ 0.28404415859352400f, 0.1038619997715160f,-0.145276649558926f, 0.898510501923554f},
	{-0.08105788424234910f, 0.0781551578108934f,-0.123965846623366f, 0.947105257601873f},
	{-0.00872608905948005f,-0.0222098231712466f,-0.115056854360748f, 0.983542001125711f}
};

// Average elliptic filter with symmetric ripple: N=12, Ap=As=-106 dB, fp=0.193, fs = 0.25 (in terms of sample rate)
static const IIRCoefficient FIR_GOOD = 0.000891054570268146f;
static const IIRSection SECTIONS_GOOD[] = {
	{ 2.2650157226725700f,-0.4034180565140230f,-0.750061486095301f, 0.157801404511953f},
	{-3.2788261989161700f, 1.3952152147542600f,-0.705854270206788f, 0.265564985645774f},
	{ 0.4397975114813240f,-1.3957634748753100f,-0.639718853965265f, 0.435324134360315f},
	{ 0.9827040216680520f, 0.1837182774040940f,-0.578569965618418f, 0.615205557837542f},
	{-0.3759752818621670f, 0.3266073609399490f,-0.540913588637109f, 0.778264420176574f},
	{-0.0253548089519618f,-0.0925779221603846f,-0.537704370375240f, 0.925800083252964f}
};

// Fast elliptic filter with symmetric ripple: N=8, Ap=As=-99 dB, fp=0.125, fs = 0.25 (in terms of sample rate)
static const IIRCoefficient FIR_FAST = 0.000882837778745889f;
static const IIRSection SECTIONS_FAST[] = {
	{ 1.215377077431620f,-0.35864455030878000f,-0.972220718789242f, 0.252934735930620f},
	{-1.525654419254140f, 0.86784918631245500f,-0.977713689358124f, 0.376580616703668f},
	{ 0.136094441564220f,-0.50414116798010400f,-1.007004471865290f, 0.584048854845331f},
	{ 0.180604082285806f,-0.00467624342403851f,-1.093486919012100f, 0.844904524843996f}
};

IIRDecimator::C::C(const unsigned int useSectionsCount, const IIRCoefficient useFIR, const IIRSection useSections[], const Quality quality) {
	if (quality == CUSTOM) {
		sectionsCount = useSectionsCount;
		fir = useFIR;
		sections = useSections;
	} else {
		unsigned int sectionsSize;
		switch (quality) {
		case FAST:
			fir = FIR_FAST;
			sections = SECTIONS_FAST;
			sectionsSize = sizeof(SECTIONS_FAST);
			break;
		case GOOD:
			fir = FIR_GOOD;
			sections = SECTIONS_GOOD;
			sectionsSize = sizeof(SECTIONS_GOOD);
			break;
		case BEST:
			fir = FIR_BEST;
			sections = SECTIONS_BEST;
			sectionsSize = sizeof(SECTIONS_BEST);
			break;
		default:
			sectionsSize = 0;
			break;
		}
		sectionsCount = (sectionsSize / sizeof(IIRSection));
	}
	buffer = new SectionBuffer[sectionsCount];
	BufferedSample *s = buffer[0][0];
	BufferedSample *e = buffer[sectionsCount][0];
	while (s < e) *(s++) = 0;
}

IIRDecimator::IIRDecimator(const Quality quality) :
	c(0, 0.0f, NULL, quality)
{}

IIRDecimator::IIRDecimator(const unsigned int useSectionsCount, const IIRCoefficient useFIR, const IIRSection useSections[]) :
	c(useSectionsCount, useFIR, useSections, IIRDecimator::CUSTOM)
{}

IIRDecimator::~IIRDecimator() {
	delete[] c.buffer;
}

static inline BufferedSample calcNumerator(const IIRSection &section, const BufferedSample buffer1, const BufferedSample buffer2) {
	return section.num1 * buffer1 + section.num2 * buffer2;
}

static inline void calcDenominator(const IIRSection &section, const BufferedSample input, const BufferedSample buffer1, BufferedSample &buffer2) {
	buffer2 = input - section.den1 * buffer1 - section.den2 * buffer2;
}

void IIRDecimator::process(const FloatSample *&inSamples, unsigned int &inLength, FloatSample *&outSamples, unsigned int &outLength) {
	while (outLength > 0 && inLength > 1) {
		inLength -= 2;
		--outLength;
		BufferedSample tmpOut[IIR_DECIMATOR_CHANNEL_COUNT];
		for (unsigned int chIx = 0; chIx < IIR_DECIMATOR_CHANNEL_COUNT; ++chIx) {
			tmpOut[chIx] = (BufferedSample)0.0;
		}
		for (unsigned int i = 0; i < c.sectionsCount; ++i) {
			const IIRSection &section = c.sections[i];
			SectionBuffer &sectionBuffer = c.buffer[i];

			for (unsigned int chIx = 0; chIx < IIR_DECIMATOR_CHANNEL_COUNT; ++chIx) {
				BufferedSample *buffer = sectionBuffer[chIx];
				tmpOut[chIx] += calcNumerator(section, buffer[0], buffer[1]);
				calcDenominator(section, BIAS + inSamples[chIx], buffer[0], buffer[1]);
				calcDenominator(section, BIAS + inSamples[chIx + IIR_DECIMATOR_CHANNEL_COUNT], buffer[1], buffer[0]);
			}
		}
		for (unsigned int chIx = 0; chIx < IIR_DECIMATOR_CHANNEL_COUNT; ++chIx) {
			*(outSamples++) = FloatSample(tmpOut[chIx] + *(inSamples++) * c.fir);
		}
		inSamples += IIR_DECIMATOR_CHANNEL_COUNT;
	}
}

unsigned int IIRDecimator::estimateInLength(const unsigned int outLength) const {
	return (outLength << 1);
}
