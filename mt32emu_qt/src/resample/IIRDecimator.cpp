/* Copyright (C) 2015 Sergey V. Mikayev
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
static const IIRCoefficient FIR_BEST = 0.0014313792470984;
static const IIRSection SECTIONS_BEST[] = {
	{ 2.85800356692148000,-0.2607342682253230,-0.602478421807085, 0.109823442522145},
	{-4.39519408383016000, 1.4651975326003500,-0.533817668127954, 0.226045921792036},
	{ 0.86638550740991800,-2.1053851417898500,-0.429134968401065, 0.403512574222174},
	{ 1.67161485530774000, 0.7963595880494520,-0.324989203363446, 0.580756666711889},
	{-1.19962759276471000, 0.5873595178851540,-0.241486447489019, 0.724264899930934},
	{ 0.01631779946479250,-0.6282334739461620,-0.182766025706656, 0.827774001858882},
	{ 0.28404415859352400, 0.1038619997715160,-0.145276649558926, 0.898510501923554},
	{-0.08105788424234910, 0.0781551578108934,-0.123965846623366, 0.947105257601873},
	{-0.00872608905948005,-0.0222098231712466,-0.115056854360748, 0.983542001125711}
};

// Average elliptic filter with symmetric ripple: N=12, Ap=As=-106 dB, fp=0.193, fs = 0.25 (in terms of sample rate)
static const IIRCoefficient FIR_GOOD = 0.000891054570268146;
static const IIRSection SECTIONS_GOOD[] = {
	{ 2.2650157226725700,-0.4034180565140230,-0.7500614860953010, 0.1578014045119530},
	{-3.2788261989161700, 1.3952152147542600,-0.7058542702067880, 0.2655649856457740},
	{ 0.4397975114813240,-1.3957634748753100,-0.6397188539652650, 0.4353241343603150},
	{ 0.9827040216680520, 0.1837182774040940,-0.5785699656184180, 0.6152055578375420},
	{-0.3759752818621670, 0.3266073609399490,-0.5409135886371090, 0.7782644201765740},
	{-0.0253548089519618,-0.0925779221603846,-0.5377043703752400, 0.9258000832529640}
};

// Fast elliptic filter with symmetric ripple: N=8, Ap=As=-99 dB, fp=0.125, fs = 0.25 (in terms of sample rate)
static const IIRCoefficient FIR_FAST = 0.000882837778745889;
static const IIRSection SECTIONS_FAST[] = {
	{ 1.21537707743162000,-0.35864455030878000,-0.97222071878924200, 0.25293473593062000},
	{-1.52565441925414000, 0.86784918631245500,-0.97771368935812400, 0.37658061670366800},
	{ 0.13609444156422000,-0.50414116798010400,-1.00700447186529000, 0.58404885484533100},
	{ 0.18060408228580600,-0.00467624342403851,-1.09348691901210000, 0.84490452484399600}
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
	c(0, NULL, NULL, quality)
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
				calcDenominator(section, inSamples[chIx], buffer[0], buffer[1]);
				calcDenominator(section, inSamples[chIx], buffer[1], buffer[0]);
			}
		}
		for (unsigned int chIx = 0; chIx < IIR_DECIMATOR_CHANNEL_COUNT; ++chIx) {
			*(outSamples++) = FloatSample(tmpOut[chIx] + *(inSamples++) * c.fir);
		}
	}
}

unsigned int IIRDecimator::estimateInLength(const unsigned int outLength) const {
	return (outLength << 1);
}
