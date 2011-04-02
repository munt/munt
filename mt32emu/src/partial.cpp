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
#include "mmath.h"

using namespace MT32Emu;

Partial::Partial(Synth *useSynth, int useDebugPartialNum) :
	synth(useSynth), debugPartialNum(useDebugPartialNum), tva(new TVA(this)), tvp(new TVP(this)), tvf(new TVF(this)) {
	ownerPart = -1;
	poly = NULL;
	pair = NULL;
}

Partial::~Partial() {
	delete tva;
	delete tvp;
	delete tvf;
}

int Partial::getOwnerPart() const {
	return ownerPart;
}

bool Partial::isActive() const {
	return ownerPart > -1;
}

const Poly *Partial::getPoly() const {
	return poly;
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

void Partial::startPartial(const Part *part, Poly *usePoly, const PatchCache *usePatchCache, const MemParams::RhythmTemp *rhythmTemp, Partial *pairPartial) {
	if (usePoly == NULL || usePatchCache == NULL) {
		synth->printDebug("*** Error: Starting partial for owner %d, usePoly=%s, usePatchCache=%s", ownerPart, usePoly == NULL ? "*** NULL ***" : "OK", usePatchCache == NULL ? "*** NULL ***" : "OK");
		return;
	}
	patchCache = usePatchCache;
	poly = usePoly;
	mixType = patchCache->structureMix;
	structurePosition = patchCache->structurePosition;

	play = true;

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
		wavePos = 0.0f;
	}

	// CONFIRMED: pulseWidthVal calculation is based on information from Mok
	pulseWidthVal = (poly->getVelocity() - 64) * (patchCache->srcPartial.wg.pulseWidthVeloSensitivity - 7) + synth->tables.pulseWidth100To255[patchCache->srcPartial.wg.pulseWidth];
	if (pulseWidthVal < 0) {
		pulseWidthVal = 0;
	} else if (pulseWidthVal > 255) {
		pulseWidthVal = 255;
	}

	pcmPosition = 0.0f;
	intPCMPosition = 0;
	pair = pairPartial;
	alreadyOutputed = false;
	tva->reset(part, patchCache->partialParam, rhythmTemp);
	tvp->reset(part, patchCache->partialParam);
	tvf->reset(patchCache->partialParam, tvp->getBasePitch());
	memset(history, 0, sizeof(history));
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

		float freq = synth->tables.pitchToFreq[pitch];

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
			if (pulseLen - wavePos >= 1.0f) {
				sample = -float(WGAMP);
			} else if (pulseLen - wavePos > 0.0f) {
				sample = float(2 * WGAMP) * (1.0f + wavePos - pulseLen) - float(WGAMP);
			} else if (wavePeriod - wavePos >= 1.0f) {
				sample = float(WGAMP);
			} else {
				sample = float(2 * WGAMP) * (wavePeriod - wavePos) - float(WGAMP);
			}

			wavePos++;
			if (wavePos > wavePeriod) {
				wavePos -= wavePeriod;
			}

			float filtVal = tvf->getBaseCutoff();
			// Sample analysis suggests that the modifier is simply added directly to filtVal.
			filtVal += tvf->nextCutoffModifier();

			float freqsum;
			if (filtVal < 128) {
				// We really don't want the filter to attenuate samples below cutoff 50
				freqsum = freq;
			} else {
				freqsum = EXP2F(8.0f * (((float)filtVal / 128.0f) - 1.0f)) * freq;
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

			sample = (floor((synth->iirFilter)((sample), &history[0], synth->tables.filtCoeff[(Bit32s)freqsum][(int)patchCache->srcPartial.tvf.resonance])));

			if ((patchCache->waveform & 1) != 0) {
				// Sawtooth waveform:
				// Confirmed from sample analysis and manuals to be produced simply
				// by multiplying a square wave with the same parameters by a cosine wave.
				sample *= cos(FLOAT_2PI * (wavePos - pulseLen) / wavePeriod);
			}

			// Instead, we attenuate samples below cutoff 50 another way
			if (filtVal < 128) {
				// Found by sample analysis
				sample *= EXP2F(-0.048820569f * (128 - filtVal));
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

bool Partial::shouldReverb() {
	if (!isActive()) {
		return false;
	}
	return patchCache->reverb;
}

void Partial::startDecayAll() {
	tva->startDecay();
	tvp->startDecay();
	tvf->startDecay();
}
