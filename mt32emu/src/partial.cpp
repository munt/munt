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

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "mt32emu.h"

#define FIXEDPOINT_UDIV(x, y, point) (((x) << (point)) / ((y)))
#define FIXEDPOINT_SDIV(x, y, point) (((x) * (1 << point)) / ((y)))
#define FIXEDPOINT_UMULT(x, y, point) (((x) * (y)) >> point)
#define FIXEDPOINT_SMULT(x, y, point) (((x) * (y)) / (1 << point))

using namespace MT32Emu;

Partial::Partial(Synth *useSynth, int debugPartialNum) :
	synth(useSynth), debugPartialNum(debugPartialNum), tva(new TVA(this)), tvp(new TVP(this)) {
	ownerPart = -1;
	poly = NULL;
	pair = NULL;
}

Partial::~Partial() {
	delete tva;
	delete tvp;
}

int Partial::getOwnerPart() const {
	return ownerPart;
}

bool Partial::isActive() const {
	return ownerPart > -1;
}

const Poly *Partial::getPoly() const {
	return this->poly;
}

void Partial::activate(int part) {
	// This just marks the partial as being assigned to a part
	ownerPart = part;
}

void Partial::deactivate() {
	if (!isActive()) {
		return;
	}
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
	if (keyfollow > 108) {
		keyfollow = 108;
	} else if (keyfollow < -108) {
		keyfollow = -108;
	}
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

	Bit8u panSetting = rhythmTemp != NULL ? rhythmTemp->panpot : part->getPatchTemp()->panpot;
	if (mixType == 3) {
		if (structurePosition == 0) {
			if (panSetting > 7) {
				panSetting = (panSetting - 7) * 2;
			} else {
				panSetting = 0;
			}
		} else {
			if (panSetting < 7) {
				panSetting = panSetting * 2;
			} else {
				panSetting = 14;
			}
		}
		// Do a normal mix independent of any pair partial.
		mixType = 0;
		pairPartial = NULL;
	}
	// FIXME: Sample analysis suggests that this is linear, but there are some some quirks that still need to be resolved.
	// On the real devices, there are only 8 real pan positions.
	stereoVolume.leftvol = panSetting * 32768 / 14;
	stereoVolume.rightvol = 32768 - stereoVolume.leftvol;

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
		synthPulseCounter = 0.0f;
	}

	// CONFIRMED: pulseWidthVal calculation is based on information from Mok
	pulseWidthVal = (poly->getVelocity() - 64) * (patchCache->srcPartial.wg.pulseWidthVeloSensitivity - 7) + synth->tables.pulseWidth100To255[patchCache->srcPartial.wg.pulseWidth];
	if (pulseWidthVal < 0) {
		pulseWidthVal = 0;
	} else if (pulseWidthVal > 255) {
		pulseWidthVal = 255;
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
	alreadyOutputed = false;
	tva->reset(part, patchCache, rhythmTemp);
	tvp->reset(part, patchCache);
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

unsigned long Partial::generateSamples(Bit16s *partialBuf, unsigned long length) {
	if (!isActive() || alreadyOutputed) {
		return 0;
	}
	if (poly == NULL) {
		synth->printDebug("*** ERROR: poly is NULL at Partial::generateSamples()!");
		return 0;
	}

	alreadyOutputed = true;

	// Generate samples

	unsigned long sampleNum;
	for (sampleNum = 0; sampleNum < length; sampleNum++) {
		float sample = 0;
		float amp = tva->nextAmp();
		if (!tva->isPlaying()) {
			deactivate();
			break;
		}

		Bit16u pitch = tvp->nextPitch();

		// Aka (slightly slower): pow(2.0f, pitchVal / 4096.0f - 16.0f) * 32000.0f
		float freq = pow(2.0f, pitch / 4096.0f - 1.034215715f);

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

			// Linear interpolation
			int firstSample = synth->pcmROMData[pcmAddr + intPCMPosition];
			int nextSample = getPCMSample(intPCMPosition + 1);
			sample = firstSample + (nextSample - firstSample) * (pcmPosition - intPCMPosition);
			if (pcmWave->loop) {
				newPCMPosition = fmod(newPCMPosition, (float)pcmWave->len);
				newIntPCMPosition = newIntPCMPosition % pcmWave->len;
			}
			pcmPosition = newPCMPosition;
			intPCMPosition = newIntPCMPosition;

			// Multiply sample with current TVA value
			sample *= amp;
		} else {
			// Render synthesised waveform
			float wavePeriod = synth->myProp.sampleRate / freq;

			// Confirmed from sample analysis that a partial with a pulseWidth parameter <= 50
			// (without any keyfollow) gets a 50% high / low ratio
			float pulseLen = 0.5f;
			if (pulseWidthVal > 128) {
				// Formula determined from sample analysis.
				float pt = 0.5f / 127.0f * (pulseWidthVal - 128);
				pulseLen += (1.239f - pt) * pt;
			}

			pulseLen *= wavePeriod;

			//Square wave
			if (pulseLen - synthPulseCounter >= 1.0f) {
				sample = -float(WGAMP);
			} else if (pulseLen - synthPulseCounter > 0.0f) {
				sample = float(2 * WGAMP) * (1.0f + synthPulseCounter - pulseLen) - float(WGAMP);
			} else if (wavePeriod - synthPulseCounter >= 1.0f) {
				sample = float(WGAMP);
			} else {
				sample = float(2 * WGAMP) * (wavePeriod - synthPulseCounter) - float(WGAMP);
			}

			synthPulseCounter++;
			if (synthPulseCounter > wavePeriod) {
				synthPulseCounter -= wavePeriod;
			}

			Bit32s filtVal = getFiltEnvelope();
			float freqsum;
			if (filtVal < 128) {
				// We really don't want the filter to attenuate samples below cutoff 50
				freqsum = freq;
			} else {
				freqsum = pow(2.0f, 8.0f * (((float)filtVal / 128.0f) - 1.0f)) * freq;
			}

			// Limit filter freq to slightly under the Nyquist frequency to avoid aliasing
			// FIXME: Move this calculation elsewhere (if it remains necessary at all)
			float filtergran = 0.484375f * synth->myProp.sampleRate;
			if (filtergran > FILTERGRAN) {
				filtergran = FILTERGRAN;
			}
			if (freqsum > filtergran) {
				freqsum = filtergran;
			}

			sample = (floor((synth->iirFilter)((sample), &history[0], synth->tables.filtCoeff[(Bit32s)freqsum][(int)patchCache->filtEnv.resonance])));

			if ((patchCache->waveform & 1) != 0) {
				// Sawtooth waveform:
				// Confirmed from sample analysis and manuals to be produced simply
				// by multiplying a square wave with the same parameters by a cosine wave.
				sample *= cos(FLOAT_2PI * (synthPulseCounter - pulseLen) / wavePeriod);
			}

			// Instead, we attenuate samples below cutoff 50 another way
			if (filtVal < 128) {
				// Found by sample analysis
				sample *= pow(2.0f, (filtVal - 128) * 0.048820569f);
			}

			// Multiply sample with current TVA value
			sample *= amp;

			if (sample < -32768.0f) {
				synth->printDebug("Overdriven amplitude for waveform=%d, freqsum=%f: %f < -32768", patchCache->waveform, freqsum, sample);
				sample = -32768.0f;
			} else if (sample > 32767.0f) {
				synth->printDebug("Overdriven amplitude for waveform=%d, freqsum=%f: %f > 32767", patchCache->waveform, freqsum, sample);
				sample = 32767.0f;
			}
			filtEnv.envpos++;
		}

		// Add sample to buffer
		*partialBuf++ = (Bit16s)sample;
	}
	// At this point, sampleNum represents the number of samples rendered
	return sampleNum;
}

Bit16s *Partial::mixBuffersRingMix(Bit16s *buf1, Bit16s *buf2, unsigned long len) {
	if (buf1 == NULL) {
		return NULL;
	}
	if (buf2 == NULL) {
		return buf1;
	}

	Bit16s *outBuf = buf1;
	while (len--) {
		Bit32s result = ((*buf1 * *buf2) >> 16) + *buf1;

		if (result > 32767) {
			result = 32767;
		}
		if (result < -32768) {
			result = -32768;
		}
		*buf1 = (Bit16s)(result);
		buf1++;
		buf2++;
	}
	return outBuf;
}

Bit16s *Partial::mixBuffersRing(Bit16s *buf1, Bit16s *buf2, unsigned long len) {
	if (buf1 == NULL) {
		return NULL;
	}
	if (buf2 == NULL) {
		return NULL;
	}

	Bit16s *outBuf = buf1;
	while (len--) {
		Bit32s result = (*buf1 * *buf2) >> 16;

		*buf1 = (Bit16s)(result);
		buf1++;
		buf2++;
	}
	return outBuf;
}

bool Partial::hasRingModulatingSlave() const {
	return pair != NULL && structurePosition == 0 && (mixType == 1 || mixType == 2);
}

bool Partial::isRingModulatingSlave() const {
	return pair != NULL && structurePosition == 1 && (mixType == 1 || mixType == 2);
}

bool Partial::isPCM() const {
	return pcmWave != NULL;
}

const ControlROMPCMStruct *Partial::getControlROMPCMStruct() const {
	if (pcmWave != NULL) {
		return pcmWave->controlROMPCMStruct;
	}
	return NULL;
}

Synth *Partial::getSynth() const {
	return synth;
}

bool Partial::produceOutput(Bit16s *partialBuf, unsigned long length) {
	if (!isActive() || alreadyOutputed || isRingModulatingSlave()) {
		return false;
	}
	if (poly == NULL) {
		synth->printDebug("*** ERROR: poly is NULL at Partial::produceOutput()!");
		return false;
	}

	Bit16s *myBuf = &myBuffer[0];
	unsigned long numGenerated = generateSamples(myBuf, length);
	if (mixType == 1 || mixType == 2) {
		Bit16s *pairBuf;
		unsigned long pairNumGenerated;
		if (pair == NULL) {
			pairBuf = NULL;
			pairNumGenerated = 0;
		} else {
			pairBuf = &pair->myBuffer[0];
			pairNumGenerated = pair->generateSamples(pairBuf, numGenerated);
			// pair will have been set to NULL if it deactivated within generateSamples()
			if (pair != NULL) {
				if (!isActive()) {
					pair->deactivate();
					pair = NULL;
				} else if (!pair->isActive()) {
					pair = NULL;
				}
			}
		}
		if (pairNumGenerated > 0) {
			if (mixType == 1) {
				mixBuffersRingMix(myBuf, pairBuf, pairNumGenerated);
			} else {
				mixBuffersRing(myBuf, pairBuf, pairNumGenerated);
			}
		}
		if (numGenerated > pairNumGenerated) {
			if (mixType == 1) {
				mixBuffersRingMix(myBuf + pairNumGenerated, NULL, numGenerated - pairNumGenerated);
			} else {
				mixBuffersRing(myBuf + pairNumGenerated, NULL, numGenerated - pairNumGenerated);
			}
		}
	}

	for (unsigned int i = 0; i < numGenerated; i++) {
		*partialBuf++ = (Bit16s)(((Bit32s)*myBuf * (Bit32s)stereoVolume.leftvol) >> 16);
		*partialBuf++ = (Bit16s)(((Bit32s)*myBuf * (Bit32s)stereoVolume.rightvol) >> 16);
		myBuf++;
	}
	if (numGenerated < length) {
		memset(partialBuf, 0, sizeof(Bit16s) * 2 * (length - numGenerated));
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
		if (tStat->envpos >= tStat->envsize) {
			reshigh = 0;
		}
	} else {
		if (tStat->envstat == 4) {
			reshigh = patchCache->filtsustain;
			if (!poly->canSustain()) {
				startFiltDecay(reshigh);
			}
		} else {
			if ((tStat->envstat == -1) || (tStat->envpos >= tStat->envsize)) {
				if (tStat->envstat == -1) {
					tStat->envbase = 0;
				} else {
					tStat->envbase = patchCache->filtEnv.envLevel[tStat->envstat];
				}
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
		Bit32u basePitch = tvp->getBasePitch();
		int pitchAdjust = (basePitch >> 4) + cutoff - 3584;
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
	veloFilEnv = 109 - patchCache->srcPartial.tvf.envVeloSensitivity + filEnv;
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
	if (!isActive()) {
		return false;
	}
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
