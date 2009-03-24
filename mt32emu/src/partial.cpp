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

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "mt32emu.h"

#define FIXEDPOINT_UDIV(x, y, point) (((x) << (point)) / ((y)))
#define FIXEDPOINT_SDIV(x, y, point) (((x) * (1 << point)) / ((y)))
#define FIXEDPOINT_UMULT(x, y, point) (((x) * (y)) >> point)
#define FIXEDPOINT_SMULT(x, y, point) (((x) * (y)) / (1 << point))

using namespace MT32Emu;

Partial::Partial(Synth *useSynth, int debugPartialNum) :
	synth(useSynth), debugPartialNum(debugPartialNum), tva(new TVA(this)), tvp(new TVP(this))
{
	ownerPart = -1;
	poly = NULL;
	pair = NULL;
	// BlitSaws are initialised with dummy values here - they'll be reset the first time they're used anyway.
	// We're not allocating lazily since deferring memory allocations until sound is actually playing doesn't seem like a good idea.
	posSaw = new BlitSaw(1, 0.5);
	negSaw = new BlitSaw(1, 0.0);
}

Partial::~Partial() {
	delete posSaw;
	delete negSaw;
	delete tva;
	delete tvp;
}

int Partial::getOwnerPart() const {
	return ownerPart;
}

bool Partial::isActive() {
	return ownerPart > -1;
}

const Poly *Partial::getPoly() const {
	return this->poly;
}

void Partial::activate(int part, int pChan) {
	// This just marks the partial as being assigned to a part
	ownerPart = part;
	this->partialChan = pChan;
}

void Partial::deactivate() {
	ownerPart = -1;
	if (poly != NULL) {
		poly->partialDeactivated(this);
		if (pair != NULL) {
			pair->pair = NULL;
		}
	}
}

void Partial::initKeyFollow(int key) {
	// Calculate keyfollow for filter.
	int keyfollow = ((key - MIDDLEC) * patchCache->filtkeyfollow) / 4096;
	if (keyfollow > 108)
		keyfollow = 108;
	else if (keyfollow < -108)
		keyfollow = -108;
	filtVal = synth->tables.tvfKeyfollowMult[keyfollow + 108];
}

// DEPRECATED: This should probably go away eventually, it's currently only used as a kludge to protect our old assumptions that
// rhythm part notes were always played as key MIDDLEC.
int Partial::getKey() const {
	if (poly == NULL) {
		return -1;
	} else if (ownerPart == 8) {
		// FIXME: Hack, should go away after new pitch stuff is committed (and possibly some TVF changes)
		return MIDDLEC;
	} else {
		return poly->getKey();
	}
}

void Partial::startPartial(const Part *part, Poly *usePoly, const PatchCache *useCache, const MemParams::RhythmTemp *rhythmTemp, Partial *pairPartial) {
	if (usePoly == NULL || useCache == NULL) {
		synth->printDebug("*** Error: Starting partial for owner %d, usePoly=%s, useCache=%s", ownerPart, usePoly == NULL ? "*** NULL ***" : "OK", useCache == NULL ? "*** NULL ***" : "OK");
		return;
	}
	patchCache = useCache;
	poly = usePoly;
	mixType = patchCache->structureMix;
	structurePosition = patchCache->structurePosition;

	play = true;
	initKeyFollow(getKey()); // Initialises filtVal and realVal
	keyLookup = &synth->tables.keyLookups[getKey() - 12];

	if (patchCache->PCMPartial) {
		pcmNum = patchCache->pcm;
		if (synth->controlROMMap->pcmCount > 128) {
			// CM-32L, etc. support two "banks" of PCMs, selectable by waveform type parameter.
			if (patchCache->waveform > 1) {
				pcmNum += 128;
			}
		}
		pcmWave = &synth->pcmWaves[pcmNum];
	} else {
		pcmWave = NULL;
	}

	pastCarrier = 0;
	pastOsc = 0;
	pastDesCarrier = 0;

	pulsewidth = patchCache->pulsewidth + synth->tables.pwVelfollowAdd[patchCache->pwsens][poly->getVelocity()];
	if (pulsewidth > 100) {
		pulsewidth = 100;
	} else if (pulsewidth < 0) {
		pulsewidth = 0;
	}

	filtEnv.envpos = 0;
	filtEnv.envstat = -1;
	filtEnv.envbase = 0;
	filtEnv.envdist = 0;
	filtEnv.envsize = 0;
	filtEnv.sustaining = false;
	filtEnv.decaying = false;
	filtEnv.prevlevel = 0;
	filtEnv.counter = 0;
	filtEnv.count = 0;

	loopPos = 0;
	pcmPosition = 0.0f;
	intPCMPosition = 0;
	pair = pairPartial;
	useNoisePair = pairPartial == NULL && (mixType == 1 || mixType == 2);
	age = 0;
	alreadyOutputed = false;
	tva->reset(part, patchCache, rhythmTemp);
	tvp->reset(part, patchCache);
	firstSample = true;
	memset(history,0,sizeof(history));
}

Bit16s Partial::getPCMSample(unsigned int position) {
	if (position >= pcmWave->len) {
		if (!pcmWave->loop) {
			return 0;
		}
		position = position % pcmWave->len;
	}
	return synth->pcmROMData[pcmWave->addr + position];
}

Bit16s *Partial::generateSamples(long length) {
	if (!isActive() || alreadyOutputed) {
		return NULL;
	}
	if (poly == NULL) {
		synth->printDebug("*** ERROR: poly is NULL at Partial::generateSamples()!");
		return NULL;
	}

	alreadyOutputed = true;

	// Generate samples

	Bit16s *partialBuf = &myBuffer[0];
	while (length--) {
		float sample = 0;
		float amp = tva->nextAmp();
		if (!tva->play) {
			deactivate();
			break;
		}

		Bit16u pitch = tvp->nextPitch();

		// Aka (slightly slower): powf(2.0f, pitchVal / 4096.0f - 16.0f) * 32000.0f
		float freq = powf(2.0f, pitch / 4096.0f - 1.034215715f);

		if (patchCache->PCMPartial) {
			// Render PCM waveform
			int len = pcmWave->len;
			if (intPCMPosition >= len && !pcmWave->loop) {
				// We're now past the end of a non-looping PCM waveform so it's time to die.
				play = false;
				deactivate();
				break;
			}
			Bit32u pcmAddr = pcmWave->addr;
			float positionDelta = freq * 2048.0f / synth->myProp.sampleRate;
			float newPCMPosition = pcmPosition + positionDelta;
			int newIntPCMPosition = (int)newPCMPosition;

			if (amp != 0.0f) {
				// Only bother doing the actual sample calculation if someone's going to hear it.
				if (positionDelta < 1.0f) {
					// Linear interpolation
					int firstSample = synth->pcmROMData[pcmAddr + intPCMPosition];
					int nextSample = getPCMSample(intPCMPosition + 1);
					sample = firstSample + (nextSample - firstSample) * (pcmPosition - intPCMPosition);
				} else if (intPCMPosition == newIntPCMPosition) {
					// Small optimisation
					sample = synth->pcmROMData[pcmAddr + newIntPCMPosition];
				} else {
					// Average all the samples in the range
					double sampleSum = synth->pcmROMData[pcmAddr + intPCMPosition] * ((intPCMPosition + 1) - pcmPosition); // First sample may not be 100% in range
					for (int position = intPCMPosition + 1; position < newIntPCMPosition; position++) {
						sampleSum += getPCMSample(position);
					}
					sampleSum += getPCMSample(newIntPCMPosition) * (newPCMPosition - newIntPCMPosition); // Last sample may not be 100% in range
					sample = (float)(sampleSum / positionDelta);
				}
			} else {
				// If a sample is calculated in the woods, and the current TVA value's too low to hear it, is there any point?
				sample = 0.0f;
			}
			if (pcmWave->loop) {
				newPCMPosition = fmodf(newPCMPosition, pcmWave->len);
				newIntPCMPosition = newIntPCMPosition % pcmWave->len;
			}
			pcmPosition = newPCMPosition;
			intPCMPosition = newIntPCMPosition;
		} else {
			// Render synthesised waveform
			if(firstSample) {
				firstSample = false;
				float spw = synth->tables.pwFactorf[pulsewidth];
				if ((patchCache->waveform & 1) != 0 && spw < 0.5f) {
					spw = 0.5f - ((0.5f - spw) * 2.0f);
				}
				posSaw->reset(freq, spw);
				negSaw->reset(freq, 0.0);
			} else {
				posSaw->setFrequency(freq);
				negSaw->setFrequency(freq);
			}
			Bit32s filtval = getFiltEnvelope();

			float phase = negSaw->getPhase();

			sample = posSaw->tick() - negSaw->tick();
			float freqsum = 0;
			freqsum = ((powf(256.0f, (((float)filtval / 128.0f) - 1.0f))) * posSaw->getStartFreq());
			if(freqsum >= (FILTERGRAN - 500.0))
				freqsum = (FILTERGRAN - 500.0f);
			filtval = (Bit32s)freqsum;

			sample = (floorf((synth->iirFilter)((sample * WGAMP), &history[0], synth->tables.filtCoeff[filtval][(int)patchCache->filtEnv.resonance])));

			if ((patchCache->waveform & 1) != 0) {
				//CC: Sawtooth samples are finally generated here by multipling an in-sync cosine
				//with the generated square wave.

				//CC: Computers are fast these days.  Not caring to use a LUT or fixed point anymore.
				//If I port this to the iPhone I may reconsider.
				sample = ((cosf(phase * 2.0f)) * sample) + (WGAMP * 0.1618f);
			}

			if (sample < -32768.0f) {
				synth->printDebug("Overdriven amplitude for waveform=%d, freqsum=%f: %f < -32768", patchCache->waveform, freqsum, sample);
				sample = -32768.0f;
			}
			else if (sample > 32767.0f) {
				synth->printDebug("Overdriven amplitude for waveform=%d, freqsum=%f: %f > 32767", patchCache->waveform, freqsum, sample);
				sample = 32767.0f;
			}
			filtEnv.envpos++;
		}

		// Multiply sample with current TVA value and add to buffer.
		*partialBuf++ = (Bit16s)(amp * sample);
	}
	// We may have deactivated and broken out of the loop before the end of the buffer,
	// if so then fill the remainder with 0s.
	if (++length > 0)
		memset(partialBuf, 0, length * 2);
	return &myBuffer[0];
}

Bit16s *Partial::mixBuffers(Bit16s * buf1, Bit16s *buf2, int len) {
	if (buf1 == NULL)
		return buf2;
	if (buf2 == NULL)
		return buf1;

	Bit16s *outBuf = buf1;
#if MT32EMU_USE_MMX >= 1
	// KG: This seems to be fine
	int donelen = i386_mixBuffers(buf1, buf2, len);
	len -= donelen;
	buf1 += donelen;
	buf2 += donelen;
#endif
	while (len--) {
		*buf1 = *buf1 + *buf2;
		buf1++, buf2++;
	}
	return outBuf;
}

Bit16s *Partial::mixBuffersRingMix(Bit16s * buf1, Bit16s *buf2, int len) {
	if (buf1 == NULL)
		return NULL;
	if (buf2 == NULL) {
		Bit16s *outBuf = buf1;
		while (len--) {
			if (*buf1 < -8192)
				*buf1 = -8192;
			else if (*buf1 > 8192)
				*buf1 = 8192;
			buf1++;
		}
		return outBuf;
	}

	Bit16s *outBuf = buf1;
#ifdef MT32EMU_USE_MMX_MIXING
	// KG: This seems to be fine
	int donelen = i386_mixBuffersRingMix(buf1, buf2, len);
	len -= donelen;
	buf1 += donelen;
	buf2 += donelen;
#endif
	#define CUTOFF 2048
	while (len--) {
		int a[3], b[3], c[2], d[2], result;
		a[0] = ((Bit32s)*buf1);
		b[0] = ((Bit32s)*buf2);

		a[1] = pastCarrier + ((CUTOFF * (a[0] - pastCarrier)) >> 12);
		a[2] = a[1] + ((CUTOFF * (0 - a[1])) >> 12);
		pastCarrier = a[2];

		b[1] = pastOsc + ((CUTOFF * (b[0] - pastOsc)) >> 12);
		b[2] = b[1] + ((CUTOFF * (0 - b[1])) >> 12);
		pastOsc = b[2];

		c[0] = a[1] ^ b[1];
		c[1] = (a[2] * b[2]);

		d[0] = pastDesCarrier + ((CUTOFF * (c[0] - pastDesCarrier)) >> 12);
		d[1] = d[0] + ((CUTOFF * (c[1] - d[0])) >> 12);

		pastDesCarrier = d[1];

		result = ((d[0] >> 5) + a[0]);

		if (result>32767)
			result = 32767;
		if (result<-32768)
			result = -32768;
		*buf1 = (Bit16s)(result);
		buf1++;
		buf2++;
	}
	return outBuf;
}

Bit16s *Partial::mixBuffersRing(Bit16s * buf1, Bit16s *buf2, int len) {
	if (buf1 == NULL) {
		return NULL;
	}
	if (buf2 == NULL) {
		return NULL;
	}

	Bit16s *outBuf = buf1;
#if MT32EMU_USE_MMX_MIXING
	// FIXME:KG: Not really checked as working
	int donelen = i386_mixBuffersRing(buf1, buf2, len);
	len -= donelen;
	buf1 += donelen;
	buf2 += donelen;
#endif
	while (len--) {
	int a[3], b[3], c[2], d[2], result;

		a[0] = ((Bit32s)*buf1);
		b[0] = ((Bit32s)*buf2);


		a[1] = pastCarrier + ((CUTOFF * (a[0] - pastCarrier)) >> 12);
		a[2] = a[1] + ((CUTOFF * (0 - a[1])) >> 12);
		pastCarrier = a[2];


		b[1] = pastOsc + ((CUTOFF * (b[0] - pastOsc)) >> 12);
		b[2] = b[1] + ((CUTOFF * (0 - b[1])) >> 12);
		pastOsc = b[2];

		c[0] = a[1] ^ b[1];
		c[1] = (a[2] * b[2]);

		d[0] = pastDesCarrier + ((CUTOFF * (c[0] - pastDesCarrier)) >> 12);
		d[1] = d[0] + ((CUTOFF * (c[1] - d[0])) >> 12);

		pastDesCarrier = d[1];

		result = d[0] >> 5;

		if (result>32767)
			result = 32767;
		if (result<-32768)
			result = -32768;
		*buf1 = (Bit16s)(result);
		buf1++;
		buf2++;
	}
	return outBuf;
}

void Partial::mixBuffersStereo(Bit16s *buf1, Bit16s *buf2, Bit16s *outBuf, int len) {
	if (buf2 == NULL) {
		while (len--) {
			*outBuf++ = *buf1++;
			*outBuf++ = 0;
		}
	} else if (buf1 == NULL) {
		while (len--) {
			*outBuf++ = 0;
			*outBuf++ = *buf2++;
		}
	} else {
		while (len--) {
			*outBuf++ = *buf1++;
			*outBuf++ = *buf2++;
		}
	}
}

bool Partial::hasRingModulatingSlave() const {
	// FIXME:KG: Check that I got structurePosition the right way around
	return pair != NULL && structurePosition == 0 && (mixType == 1 || mixType == 2);
}

bool Partial::isRingModulatingSlave() const {
	// FIXME:KG: Check that I got structurePosition the right way around
	return pair != NULL && structurePosition == 1 && (mixType == 1 || mixType == 2);
}

bool Partial::isPCM() const {
	return pcmWave != NULL;
}

const ControlROMPCMStruct *Partial::getControlROMPCMStruct() const {
	if(pcmWave != NULL)
		return pcmWave->controlROMPCMStruct;
	return NULL;
}

Synth *Partial::getSynth() const {
	return synth;
}

bool Partial::produceOutput(Bit16s *partialBuf, long length) {
	if (!isActive() || alreadyOutputed)
		return false;
	if (poly == NULL) {
		synth->printDebug("*** ERROR: poly is NULL at Partial::produceOutput()!");
		return false;
	}

	Bit16s *pairBuf = NULL;
	// Check for dependant partial
	if (pair != NULL) {
		if (!pair->alreadyOutputed) {
			// Note: pair may have become NULL after this
			pairBuf = pair->generateSamples(length);
		}
	} else if (useNoisePair) {
		// Generate noise for pairless ring mix
		pairBuf = synth->tables.noiseBuf;
	}

	Bit16s *myBuf = generateSamples(length);

	if (myBuf == NULL && pairBuf == NULL)
		return false;

	Bit16s *p1buf, *p2buf;

	if (structurePosition == 0 || pairBuf == NULL) {
		p1buf = myBuf;
		p2buf = pairBuf;
	} else {
		p2buf = myBuf;
		p1buf = pairBuf;
	}

	//synth->printDebug("mixType: %d", mixType);

	Bit16s *mixedBuf;
	switch (mixType) {
	case 0:
		// Standard sound mix
		mixedBuf = mixBuffers(p1buf, p2buf, length);
		break;

	case 1:
		// Ring modulation with sound mix
		mixedBuf = mixBuffersRingMix(p1buf, p2buf, length);
		break;

	case 2:
		// Ring modulation alone
		mixedBuf = mixBuffersRing(p1buf, p2buf, length);
		break;

	case 3:
		// Stereo mixing.  One partial to one speaker channel, one to another.
		// FIXME:KG: Surely we should be multiplying by the left/right volumes here?
		mixBuffersStereo(p1buf, p2buf, partialBuf, length);
		return true;

	default:
		mixedBuf = mixBuffers(p1buf, p2buf, length);
		break;
	}

	if (mixedBuf == NULL)
		return false;

	Bit16s leftvol, rightvol;
	leftvol = patchCache->pansetptr->leftvol;
	rightvol = patchCache->pansetptr->rightvol;

#if MT32EMU_USE_MMX >= 2
	// FIXME:KG: This appears to introduce crackle
	int donelen = i386_partialProductOutput(length, leftvol, rightvol, partialBuf, mixedBuf);
	length -= donelen;
	mixedBuf += donelen;
	partialBuf += donelen * 2;
#endif
	while (length--) {
		*partialBuf++ = (Bit16s)(((Bit32s)*mixedBuf * (Bit32s)leftvol) >> 16);
		*partialBuf++ = (Bit16s)(((Bit32s)*mixedBuf * (Bit32s)rightvol) >> 16);
		mixedBuf++;
	}
	return true;
}

Bit32s Partial::getFiltEnvelope() {
	int reshigh;

	int cutoff;

	EnvelopeStatus *tStat = &filtEnv;

	if (tStat->decaying) {
		reshigh = tStat->envbase;
		reshigh = (reshigh + ((tStat->envdist * tStat->envpos) / tStat->envsize));
		if (tStat->envpos >= tStat->envsize)
			reshigh = 0;
	} else {
		if (tStat->envstat==4) {
			reshigh = patchCache->filtsustain;
			if (!poly->canSustain()) {
				startFiltDecay(reshigh);
			}
		} else {
			if ((tStat->envstat==-1) || (tStat->envpos >= tStat->envsize)) {
				if (tStat->envstat==-1)
					tStat->envbase = 0;
				else
					tStat->envbase = patchCache->filtEnv.envLevel[tStat->envstat];
				tStat->envstat++;
				tStat->envpos = 0;
				if (tStat->envstat == 3) {
					tStat->envsize = synth->tables.envTime[(int)patchCache->filtEnv.envTime[tStat->envstat]];
				} else {
					Bit32u envTime = (int)patchCache->filtEnv.envTime[tStat->envstat];
					if (tStat->envstat > 1) {
						int envDiff = abs(patchCache->filtEnv.envLevel[tStat->envstat] - patchCache->filtEnv.envLevel[tStat->envstat - 1]);
						if (envTime > synth->tables.envDeltaMaxTime[envDiff]) {
							envTime = synth->tables.envDeltaMaxTime[envDiff];
						}
					}

					tStat->envsize = (synth->tables.envTime[envTime] * keyLookup->envTimeMult[(int)patchCache->filtEnv.envTimeKeyfollow]) >> 8;
				}

				tStat->envsize++;
				tStat->envdist = patchCache->filtEnv.envLevel[tStat->envstat] - tStat->envbase;
			}

			reshigh = tStat->envbase;
			reshigh = (reshigh + ((tStat->envdist * tStat->envpos) / tStat->envsize));
		}
		tStat->prevlevel = reshigh;
	}

	cutoff = filtMultKeyfollow[patchCache->srcPartial.tvf.keyfollow] - filtMultKeyfollow[patchCache->srcPartial.wg.pitchKeyfollow];
	cutoff *= ((Bit32s)getKey() - 60);
	int dynamicBiasPoint = (Bit32s)patchCache->srcPartial.tvf.biasPoint;
	if ((dynamicBiasPoint & 0x40) == 0) {
		dynamicBiasPoint = dynamicBiasPoint + 33 - (Bit32s)getKey();
		if (dynamicBiasPoint > 0) {
			dynamicBiasPoint = -dynamicBiasPoint;
			dynamicBiasPoint *= BiasLevel_MulTable[patchCache->srcPartial.tvf.biasLevel];
			cutoff += dynamicBiasPoint;
		}
	} else {
		dynamicBiasPoint = dynamicBiasPoint - 31 - (Bit32s)getKey();
		if (dynamicBiasPoint <= 0) {
			dynamicBiasPoint *= BiasLevel_MulTable[patchCache->srcPartial.tvf.biasLevel];
			cutoff += dynamicBiasPoint;
		}
	}

	cutoff += (((Bit32s)patchCache->srcPartial.tvf.cutoff << 4) - 800);
	if (cutoff >= 0) {

		// FIXME: CC: Coarse pitch calculation placeholder until end-to-end use of MT-32-like pitch calculation
		// is application-wide.
		int oc  = getKey() + 12;
		int pitch = pitchROMTable[oc % 12];
		pitch += ((oc / 12) << 12) - 24576;

		pitch = (pitch * romMultKeyfollow[patchCache->srcPartial.wg.pitchKeyfollow]) >> 13;
		if (patchCache->srcPartial.wg.waveform & 1) {
			pitch += 33037;
		} else {
			pitch += 37133;
		}

		if (pitch < 0) {
			pitch = 0;
		}
		if (pitch > 59392) {
			pitch = 59392;
		}

		int pitchAdjust = (pitch >> 4) + cutoff - 3584;
		if (pitchAdjust > 0) {
			cutoff -= pitchAdjust;
		}
	} else {
		if (cutoff < -2048) {
			cutoff = -2048;
		}
	}
	cutoff = (cutoff + 2056) >> 4;
	if (cutoff > 255) {
		cutoff = 255;
	}

	int veloFilEnv = (Bit32s)poly->getVelocity() * (Bit32s)patchCache->srcPartial.tvf.envVeloSensitivity;
	int filEnv = (veloFilEnv << 2) >> 8;
	veloFilEnv  = 109 - patchCache->srcPartial.tvf.envVeloSensitivity + filEnv;
	filEnv = ((Bit32s)getKey() - 60) >> (4 - (Bit32s)patchCache->srcPartial.tvf.envDepthKeyfollow);
	veloFilEnv += filEnv;
	if (veloFilEnv < 0) {
		veloFilEnv = 0;
	}
	veloFilEnv *= patchCache->srcPartial.tvf.envDepth;
	filEnv = (veloFilEnv << 2) >> 8;
	if (filEnv > 255) {
		filEnv = 255;
	}

	filEnv = (filEnv * reshigh) >> 8;

	return cutoff + filEnv;
}

bool Partial::shouldReverb() {
	if (!isActive())
		return false;
	return patchCache->reverb;
}

void Partial::startDecayAll() {
	tva->startDecay();
	tvp->startDecay();
	startFiltDecay(filtEnv.prevlevel);
}

void Partial::startFiltDecay(Bit32s startval) {
	filtEnv.sustaining = false;
	filtEnv.decaying = true;
	filtEnv.envpos = 0;
	filtEnv.envbase = startval;

	filtEnv.envsize = FIXEDPOINT_UMULT(synth->tables.envDecayTime[(int)patchCache->filtEnv.envTime[4]], keyLookup->envTimeMult[(int)patchCache->filtEnv.envTimeKeyfollow], 8);
	filtEnv.envdist = -startval;
	filtEnv.envsize++;
}
