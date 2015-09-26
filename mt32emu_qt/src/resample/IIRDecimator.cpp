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
	buffer = new BufferedSample[sectionsCount][IIR_DECIMATOR_CHANNEL_COUNT][IIR_SECTION_ORDER];
	BufferedSample *s = buffer[0][0];
	BufferedSample *e = buffer[sectionsCount][0];
	while (s < e) *(s++) = 0;
}

IIRDecimator::IIRDecimator(const Quality quality) :
	c(0, NULL, NULL, quality),
	phase(1)
{}

IIRDecimator::IIRDecimator(const unsigned int useSectionsCount, const IIRCoefficient useFIR, const IIRSection useSections[]) :
	c(useSectionsCount, useFIR, useSections, IIRDecimator::CUSTOM),
	phase(1)
{
	outputSamples[0] = 0.0f;
	outputSamples[1] = 0.0f;
}

IIRDecimator::~IIRDecimator() {
	delete[] c.buffer;
}

void IIRDecimator::process(const FloatSample *&inSamples, unsigned int &inLength, FloatSample *&outSamples, unsigned int &outLength) {
	while (outLength > 0) {
		while (needNextInSample()) {
			if (inLength == 0) return;
			addInSamples(inSamples);
			--inLength;
		}
		getOutSamples(outSamples);
		--outLength;
	}
}

unsigned int IIRDecimator::estimateInLength(const unsigned int outLength) const {
	return (outLength << 1) + phase;
}

bool IIRDecimator::needNextInSample() const {
	return phase != 0;
}

void IIRDecimator::addInSamples(const FloatSample *&inSamples) {
	BufferedSample leftIn = *(inSamples++);
	BufferedSample rightIn = *(inSamples++);
	--phase;
	unsigned int z1Ix = phase;
	unsigned int z2Ix = phase ^ 1;
	BufferedSample leftOut = 0.0;
	BufferedSample rightOut = 0.0;
	for (unsigned int i = 0; i <= c.sectionsCount; ++i) {
		BufferedSample leftRecOut = leftIn - c.sections[i].den1 * c.buffer[i][0][z1Ix] - c.sections[i].den2 * c.buffer[i][0][z2Ix];
		BufferedSample rightRecOut = rightIn - c.sections[i].den1 * c.buffer[i][1][z1Ix] - c.sections[i].den2 * c.buffer[i][1][z2Ix];
		if (needNextInSample()) continue; // Output isn't needed now

		leftOut += c.sections[i].num1 * c.buffer[i][0][z1Ix] + c.sections[i].num2 * c.buffer[i][0][z2Ix];
		c.buffer[i][0][z2Ix] = leftRecOut;

		rightOut += c.sections[i].num1 * c.buffer[i][1][z1Ix] + c.sections[i].num2 * c.buffer[i][1][z2Ix];
		c.buffer[i][1][z2Ix] = rightRecOut;
	}
	outputSamples[0] = FloatSample(leftOut + leftIn * c.fir);
	outputSamples[1] = FloatSample(rightOut + rightIn * c.fir);
}

void IIRDecimator::getOutSamples(FloatSample *&outSamples) {
	*(outSamples++) = outputSamples[0];
	*(outSamples++) = outputSamples[1];
	phase += 2;
}
