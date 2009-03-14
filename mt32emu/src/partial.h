/* Copyright (C) 2003-2009 Dean Beeler, Jerome Fisher
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

#ifndef MT32EMU_PARTIAL_H
#define MT32EMU_PARTIAL_H

namespace MT32Emu {

class Synth;
class Part;
class TVA;
struct NoteLookup;
struct ControlROMPCMStruct;

enum EnvelopeType {
	EnvelopeType_filt = 0,
	EnvelopeType_pitch = 1
};

struct EnvelopeStatus {
	Bit32s envpos;
	Bit32s envstat;
	Bit32s envbase;
	Bit32s envdist;
	Bit32s envsize;

	bool sustaining;
	bool decaying;
	Bit32s prevlevel;

	Bit32s counter;
	Bit32s count;
};

// Class definition of MT-32 partials.  32 in all.
class Partial {
private:
	Synth *synth;
	const int debugPartialNum; // Only used for debugging

	int ownerPart; // -1 if unassigned
	int mixType;
	int structurePosition; // 0 or 1 of a structure pair
	bool useNoisePair;
	int partialChan;
	int pastDelta;

	Bit16s myBuffer[MAX_SAMPLE_OUTPUT];

	// Keyfollowed note value
#if MT32EMU_ACCURATENOTES == 1
	NoteLookup noteLookupStorage;
	float noteVal;
#else
	int noteVal;
	int fineShift;
#endif
	BlitSaw *posSaw;
	BlitSaw *negSaw;
	BlitSaw *saw;

	const NoteLookup *noteLookup; // LUTs for this noteVal
	const KeyLookup *keyLookup; // LUTs for the clamped (12..108) key

	// Keyfollowed filter values
	int realVal;
	int filtVal;

	// Only used for PCM partials
	int pcmNum;
	PCMWaveEntry *pcmWave;

	int pulsewidth;

	Bit32u lfoPos;
	soundaddr partialOff;

	Bit32u pitchEnvVal;

	float history[32];

	bool pitchSustain;

	int loopPos;

	Poly *poly;

	int bendShift;

	Bit16s *mixBuffers(Bit16s *buf1, Bit16s *buf2, int len);
	Bit16s *mixBuffersRingMix(Bit16s *buf1, Bit16s *buf2, int len);
	Bit16s *mixBuffersRing(Bit16s *buf1, Bit16s *buf2, int len);
	void mixBuffersStereo(Bit16s *buf1, Bit16s *buf2, Bit16s *outBuf, int len);

	Bit32s getFiltEnvelope();
	Bit32s getPitchEnvelope();

	void initKeyFollow(int freqNum);

public:
	const PatchCache *patchCache;
	TVA *tva;
	EnvelopeStatus envs[2];
	bool play;

	PatchCache cachebackup;

	Partial *pair;
	bool alreadyOutputed;
	Bit32u age;

	Partial(Synth *synth, int debugPartialNum);
	~Partial();

	int getOwnerPart() const;
	int getKey() const;
	const Poly *getPoly() const;
	bool isActive();
	void activate(int part, int pChan);
	void deactivate(void);
	void startPartial(const Part *part, Poly *usePoly, const PatchCache *useCache, Partial *pairPartial);
	void startDecay(EnvelopeType envnum, Bit32s startval);
	void startDecayAll();
	void setBend(float factor);
	bool shouldReverb();
	bool hasRingModulatingSlave() const;
	bool isRingModulatingSlave() const;
	bool isPCM() const;
	const ControlROMPCMStruct *getControlROMPCMStruct() const;
	Synth *getSynth() const;

	// Returns true only if data written to buffer
	// This function (unlike the one below it) returns processed stereo samples
	// made from combining this single partial with its pair, if it has one.
	bool produceOutput(Bit16s * partialBuf, long length);

	// This function produces mono sample output using the partial's private internal buffer
	Bit16s *generateSamples(long length);
};

}

#endif
