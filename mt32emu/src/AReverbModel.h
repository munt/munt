/* Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MT32EMU_AREVERB_MODEL_H
#define MT32EMU_AREVERB_MODEL_H

namespace MT32Emu {

#define numAllpasses 4
#define numDelays 5

struct AReverbSettings {
	Bit32u *allpassSizes;
	Bit32u *delaySizes;
	float *decayTimes;
	float *wetLevels;
	float filtVal;
	float damp1;
	float damp2;
};

class RingBuffer abstract {
protected:
	float *buffer;
	Bit32u size;
	Bit32u index;
public:
	RingBuffer(Bit32u size);
	~RingBuffer();
	float next();
	bool isEmpty();
	void mute();
	float process(float in);
};

class AllPassFilter : public RingBuffer {
public:
	AllPassFilter(Bit32u size);
	float process(float in);
};

class Delay : public RingBuffer {
public:
	Delay(Bit32u size);
	float process(float in);
};

class AReverbModel : public ReverbModel {
	AllPassFilter *allpasses[numAllpasses];
	Delay *delays[numDelays];

	const AReverbSettings *currentSettings;
	float decayTime;
	float wetLevel;
	float filterhist1, filterhist2;
	float combhist;
	void mute();
public:
	AReverbModel(const AReverbSettings *newSettings);
	~AReverbModel();
	void open(unsigned int sampleRate);
	void close();
	void setParameters(Bit8u time, Bit8u level);
	void process(const float *inLeft, const float *inRight, float *outLeft, float *outRight, unsigned long numSamples);
	bool isActive() const;
};

// Default reverb settings for modes 0-2

static Bit32u reverbMode0allpasses[] = {78, 729, 994, 389};
static Bit32u reverbMode0delays[] = {846, 4, 1819, 778, 346};
static float reverbMode0Times[] = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.9f};
static float reverbMode0Levels[] = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 1.01575f};
static AReverbSettings reverbMode0Settings = {reverbMode0allpasses, reverbMode0delays,
			reverbMode0Times, reverbMode0Levels, 0.687770909f, 0.2f, 0.8f};

static Bit32u reverbMode1allpasses[] = {176, 809, 1324, 1258};
static Bit32u reverbMode1delays[] = {2262, 124, 974, 2516, 356};
static float reverbMode1Times[] = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.95f};
static float reverbMode1Levels[] = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 1.01575f};
static AReverbSettings reverbMode1Settings = {reverbMode1allpasses, reverbMode1delays,
			reverbMode1Times,	reverbMode1Levels, 0.712025098f, 0.375f, 0.625f};

static Bit32u reverbMode2allpasses[] = {78, 729, 994, 389};
static Bit32u reverbMode2delays[] = {846, 4, 1819, 778, 346};
static float reverbMode2Times[] = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f};
static float reverbMode2Levels[] = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f};
static AReverbSettings reverbMode2Settings = {reverbMode2allpasses, reverbMode2delays,
			reverbMode2Times,	reverbMode2Levels, 0.939522749f, 0.0f, 0.0f};

}

#endif
