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
	synth(useSynth), debugPartialNum(debugPartialNum), tva(new TVA(this))
{
	ownerPart = -1;
	poly = NULL;
	pair = NULL;
	// BlitSaws are initialised with dummy values here - they'll be reset the first time they're used anyway.
	// We're not allocating lazily since deferring memory allocations until sound is actually playing doesn't seem like a good idea.
	posSaw = new BlitSaw(1, 0.5);
	negSaw = new BlitSaw(1, 0.0);
	saw = new BlitSaw(1, 0.8);
#if MT32EMU_ACCURATENOTES == 1
	for (int i = 0; i < 3; i++) {
		noteLookupStorage.waveforms[i] = new Bit16s[65536];
	}
	noteLookup = &noteLookupStorage;
#endif
}

Partial::~Partial() {
#if MT32EMU_ACCURATENOTES == 1
	for (int i = 0; i < 3; i++) {
		delete[] noteLookupStorage.waveforms[i];
	}
	delete[] noteLookupStorage.wavTable;
#endif
	delete posSaw;
	delete negSaw;
	delete saw;
	delete tva;
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
		for (int i = 0; i < 4; i++) {
			if (poly->partials[i] == this) {
				poly->partials[i] = NULL;
				break;
			}
		}
		if (pair != NULL) {
			pair->pair = NULL;
		}
	}
}

void Partial::initKeyFollow(int key) {
	// Setup partial keyfollow
	// Note follow relative to middle C

	// Calculate keyfollow for pitch
#if 1
	float rel = key == -1 ? 0.0f : (key - MIDDLEC);
	float newPitch = rel * patchCache->pitchKeyfollow + patchCache->pitch + patchCache->pitchShift;
	//FIXME:KG: Does it truncate the keyfollowed pitch to a semitone (towards MIDDLEC)?
	//int newKey = (int)(rel * patchCache->pitchKeyfollow);
	//float newPitch = newKey + patchCache->pitch + patchCache->pitchShift;
#else
	float rel = key == -1 ? 0.0f : (key + patchCache->pitchShift - MIDDLEC);
	float newPitch = rel * patchCache->pitchKeyfollow + patchCache->pitch;
#endif
#if MT32EMU_ACCURATENOTES == 1
	noteVal = newPitch;
	//synth->printDebug("key=%d, pitch=%f, pitchKeyfollow=%f, pitchShift=%f, newPitch=%f", key, patchCache->pitch, patchCache->pitchKeyfollow, patchCache->pitchShift, newPitch);
#else
	float newPitchInt;
	float newPitchFract = modff(newPitch, &newPitchInt);
	if (newPitchFract > 0.5f) {
		newPitchInt += 1.0f;
		newPitchFract -= 1.0f;
	}
	noteVal = (int)newPitchInt;
	fineShift = (int)(powf(2.0f, newPitchFract / 12.0f) * 4096.0f);
	//synth->printDebug("key=%d, pitch=%f, pitchKeyfollow=%f, pitchShift=%f, newPitch=%f, noteVal=%d, fineShift=%d", key, patchCache->pitch, patchCache->pitchKeyfollow, patchCache->pitchShift, newPitch, noteVal, fineShift);
#endif
	// FIXME:KG: Raise/lower by octaves until in the supported range.
	while (noteVal > HIGHEST_NOTE) // FIXME:KG: see tables.cpp: >108?
		noteVal -= 12;
	while (noteVal < LOWEST_NOTE) // FIXME:KG: see tables.cpp: <12?
		noteVal += 12;
	// Calculate keyfollow for filter
	int keyfollow = ((key - MIDDLEC) * patchCache->filtkeyfollow) / 4096;
	if (keyfollow > 108)
		keyfollow = 108;
	else if (keyfollow < -108)
		keyfollow = -108;
	filtVal = synth->tables.tvfKeyfollowMult[keyfollow + 108];
	realVal = synth->tables.tvfKeyfollowMult[(noteVal - MIDDLEC) + 108];
}

int Partial::getKey() const {
	if (poly == NULL) {
		return -1;
	} else {
		return poly->key;
	}
}

void Partial::startPartial(const Part *part, Poly *usePoly, const PatchCache *useCache, Partial *pairPartial) {
	if (usePoly == NULL || useCache == NULL) {
		synth->printDebug("*** Error: Starting partial for owner %d, usePoly=%s, useCache=%s", ownerPart, usePoly == NULL ? "*** NULL ***" : "OK", useCache == NULL ? "*** NULL ***" : "OK");
		return;
	}
	patchCache = useCache;
	poly = usePoly;
	mixType = patchCache->structureMix;
	structurePosition = patchCache->structurePosition;

	play = true;
	initKeyFollow(poly->key); // Initialises noteVal, filtVal and realVal
#if MT32EMU_ACCURATENOTES == 0
	noteLookup = &synth->tables.noteLookups[noteVal - LOWEST_NOTE];
#else
	Tables::initNote(synth, &noteLookupStorage, noteVal, (float)synth->myProp.sampleRate, synth->masterTune, synth->pcmWaves, NULL);
#endif
	keyLookup = &synth->tables.keyLookups[poly->key - 12];

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

	lfoPos = 0;
	pulsewidth = patchCache->pulsewidth + synth->tables.pwVelfollowAdd[patchCache->pwsens][poly->vel];
	if (pulsewidth > 100) {
		pulsewidth = 100;
	} else if (pulsewidth < 0) {
		pulsewidth = 0;
	}

	if ((patchCache->waveform & 1) == 0) {
		posSaw->reset(noteLookup->freq, synth->tables.pwFactorf[pulsewidth]);
		negSaw->reset(noteLookup->freq, 0.0);
		saw->reset(noteLookup->freq, 0.8);
	} else {
		// CC: Sawtooth waves play at half the frequency as the cosine multiplication
		// effectively doubles the rate of the square wave.
		posSaw->reset(noteLookup->freq / 2.0f, synth->tables.pwFactorf[pulsewidth]);
		negSaw->reset(noteLookup->freq / 2.0f, 0.0);
		saw->reset(noteLookup->freq / 2.0f, 0.8);
	}

	for (int e = 0; e < 2; e++) {
		envs[e].envpos = 0;
		envs[e].envstat = -1;
		envs[e].envbase = 0;
		envs[e].envdist = 0;
		envs[e].envsize = 0;
		envs[e].sustaining = false;
		envs[e].decaying = false;
		envs[e].prevlevel = 0;
		envs[e].counter = 0;
		envs[e].count = 0;
	}
	pitchEnvVal = 0;
	pitchSustain = false;
	loopPos = 0;
	partialOff.pcmoffset = partialOff.pcmplace = 0;
	pair = pairPartial;
	useNoisePair = pairPartial == NULL && (mixType == 1 || mixType == 2);
	age = 0;
	alreadyOutputed = false;
	tva->reset(part, patchCache);
	memset(history,0,sizeof(history));
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
		Bit32s envval;
		Bit32s sample = 0;
		Bit32u amp = tva->nextAmp();
		if (!tva->play) {
			deactivate();
			break;
		}

		unsigned int lfoShift = 0x1000;
		if (pitchSustain) {
			// Calculate LFO position
			// LFO does not kick in completely until pitch envelope sustains
			if (patchCache->lfodepth > 0) {
				lfoPos++;
				if (lfoPos >= patchCache->lfoperiod)
					lfoPos = 0;
				int lfoatm = FIXEDPOINT_UDIV(lfoPos, patchCache->lfoperiod, 16);
				int lfoatr = synth->tables.sintable[lfoatm];
				lfoShift = synth->tables.lfoShift[patchCache->lfodepth][lfoatr];
			}
		} else {
			// Calculate Pitch envelope
			envval = getPitchEnvelope();
			int pd = patchCache->pitchEnv.depth;
			pitchEnvVal = synth->tables.pitchEnvVal[pd][envval];
		}

		int delta;

		// Wrap positions or end if necessary
		if (patchCache->PCMPartial) {
			// PCM partial

			delta = noteLookup->wavTable[pcmNum];
			int len = pcmWave->len;
			if (partialOff.pcmplace >= len) {
				if (pcmWave->loop) {
					//partialOff.pcmplace = partialOff.pcmoffset = 0;
					partialOff.pcmplace %= len;
				} else {
					play = false;
					deactivate();
					break;
				}
			}
		} else {
			// Synthesis partial
			delta = 0x10000;
			partialOff.pcmplace %= (Bit16u)noteLookup->div2;
		}

		// Build delta for position of next sample
		// Fix delta code
		Bit32u tdelta = delta;
#if MT32EMU_ACCURATENOTES == 0
		tdelta = FIXEDPOINT_UMULT(tdelta, fineShift, 12);
#endif
		tdelta = FIXEDPOINT_UMULT(tdelta, pitchEnvVal, 12);
		tdelta = FIXEDPOINT_UMULT(tdelta, lfoShift, 12);
		tdelta = FIXEDPOINT_UMULT(tdelta, bendShift, 12);

		if (tdelta != pastDelta) {
			float startFreq = posSaw->getStartFreq();
			startFreq = startFreq * ((float)tdelta / (float)delta);

			posSaw->setFrequency(startFreq);
			negSaw->setFrequency(startFreq);
			pastDelta = tdelta;
		}
		//noteLookup->saw[partialChan]->setFrequency(startFreq);

		delta = (int)tdelta;

		// Get waveform - either PCM or synthesized sawtooth or square
		if (amp > 0) {
			if (patchCache->PCMPartial) {
				// Render PCM sample
				int ra, rb, dist;
				Bit32u taddr;
				Bit32u pcmAddr = pcmWave->addr;
				if (delta < 0x10000) {
					// Linear sound interpolation
					taddr = pcmAddr + partialOff.pcmplace;
					ra = synth->pcmROMData[taddr];
					taddr++;
					if (taddr == pcmAddr + pcmWave->len) {
						// Past end of PCM
						if (pcmWave->loop) {
							rb = synth->pcmROMData[pcmAddr];
						} else {
							rb = 0;
						}
					} else {
						rb = synth->pcmROMData[taddr];
					}
					dist = rb - ra;
					sample = (ra + ((dist * (Bit32s)(partialOff.pcmoffset >> 8)) >> 8));
				} else {
					// Sound decimation
					// The right way to do it is to use a lowpass filter on the waveform before selecting
					// a point.  This is too slow.  The following approximates this as fast as possible
					int idelta = delta >> 16;
					taddr = pcmAddr + partialOff.pcmplace;
					ra = synth->pcmROMData[taddr++];
					for (int ix = 0; ix < idelta - 1; ix++) {
						if (taddr == pcmAddr + pcmWave->len) {
							// Past end of PCM
							if (pcmWave->loop) {
								taddr = pcmAddr;
							} else {
								// Behave as if all subsequent samples were 0
								break;
							}
						}
						ra += synth->pcmROMData[taddr++];
					}
					sample = ra / idelta;
				}
			} else {
				// Render synthesised sample
				Bit32s filterInput;
				Bit32s filtval = getFiltEnvelope();

				float phase = posSaw->getPhase();

				//CC: There used to be a lot of code here.  :-)
				filterInput = (posSaw->tick() - negSaw->tick()) * WGAMP ;

				if ((patchCache->waveform & 1) != 0) {
					//CC: Sawtooth samples are finally generated here by multipling an in-sync cosine
					//with the generated square wave.

					//CC: Computers are fast these days.  Not caring to use a LUT or fixed point anymore.
					//If I port this to the iPhone I may reconsider.
					filterInput = (Bit32s)(cos(phase * 2.0f) * filterInput);
				}

				if (filtval > ((FILTERGRAN * 15) / 16))
					filtval = ((FILTERGRAN * 15) / 16);
				sample = (Bit32s)(floorf((synth->iirFilter)((float)filterInput, &history[0], synth->tables.filtCoeff[filtval][(int)patchCache->filtEnv.resonance])) / synth->tables.resonanceFactor[patchCache->filtEnv.resonance]);
				if (sample < -32768) {
					synth->printDebug("Overdriven amplitude for %d: %d:=%d < -32768", patchCache->waveform, filterInput, sample);
					sample = -32768;
				}
				else if (sample > 32767) {
					synth->printDebug("Overdriven amplitude for %d: %d:=%d > 32767", patchCache->waveform, filterInput, sample);
					sample = 32767;
				}
			}
		}

		// Add calculated delta to our waveform offset
		Bit32u absOff = ((partialOff.pcmplace << 16) | partialOff.pcmoffset);
		absOff += delta;
		partialOff.pcmplace = (Bit16u)((absOff & 0xFFFF0000) >> 16);
		partialOff.pcmoffset = (Bit16u)(absOff & 0xFFFF);

		// Put volume envelope over generated sample
		// FIXME:KG: Note that the "32768.0f" here is slightly arbitrary, and needs to be confirmed.
		// FIXME:KG: Obviously we should use something faster once we've got the details sorted out.
		sample = (Bit32s)(powf(2.0f, amp / 16.0f - 1.0f) / 32768.0f * sample);

		envs[EnvelopeType_pitch].envpos++;
		envs[EnvelopeType_filt].envpos++;

		*partialBuf++ = (Bit16s)sample;
	}
	// We may have deactivated and broken out of the loop before the end of the buffer,
	// if so then fill the remainder with 0s.
	if (++length > 0)
		memset(partialBuf, 0, length * 2);
	return &myBuffer[0];
}

void Partial::setBend(float factor) {
	if (!patchCache->useBender || factor == 0.0f) {
		bendShift = 4096;
		return;
	}
	// NOTE:KG: We can't do this smoothly with lookup tables, unless we use several MB.
	// FIXME:KG: Bend should be influenced by pitch key-follow too, according to docs.
	float bendSemitones = factor * patchCache->benderRange; // -24 .. 24
	float mult = powf(2.0f, bendSemitones / 12.0f);

	/*
	noteLookup->posSaw[partialChan]->setFrequency(noteLookup->posSaw[partialChan]->getStartFreq() * mult);
	noteLookup->negSaw[partialChan]->setFrequency(noteLookup->posSaw[partialChan]->getStartFreq() * mult);
	noteLookup->saw[partialChan]->setFrequency(noteLookup->posSaw[partialChan]->getStartFreq() * mult);
	*/

	//synth->printDebug("setBend(): factor=%f, benderRange=%f, semitones=%f, mult=%f\n", factor, patchCache->benderRange, bendSemitones, mult);
	bendShift = (int)(mult * 4096.0f);
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

		c[0] = a[1] * b[1];
		c[1] = (a[2] * b[2]);

		d[0] = pastDesCarrier + ((CUTOFF * (c[0] - pastDesCarrier)) >> 12);
		d[1] = d[0] + ((CUTOFF * (c[1] - d[0])) >> 12);

		pastDesCarrier = d[1];

		result = ((d[0] >> 12) + a[0]) >> 1;

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

		c[0] = a[1] * b[1];
		c[1] = (a[2] * b[2]);

		d[0] = pastDesCarrier + ((CUTOFF * (c[0] - pastDesCarrier)) >> 12);
		d[1] = d[0] + ((CUTOFF * (c[1] - d[0])) >> 12);

		pastDesCarrier = d[1];

		result = d[0] >> 12;

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

	EnvelopeStatus *tStat  = &envs[EnvelopeType_filt];

	if (tStat->decaying) {
		reshigh = tStat->envbase;
		reshigh = (reshigh + ((tStat->envdist * tStat->envpos) / tStat->envsize));
		if (tStat->envpos >= tStat->envsize)
			reshigh = 0;
	} else {
		if (tStat->envstat==4) {
			reshigh = patchCache->filtsustain;
			if (!poly->sustain) {
				startDecay(EnvelopeType_filt, reshigh);
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
	cutoff *= ((Bit32s)poly->key - 60);
	int dynamicBiasPoint = (Bit32s)patchCache->srcPartial.tvf.biasPoint;
	if ((dynamicBiasPoint & 0x40) == 0) {
		dynamicBiasPoint = dynamicBiasPoint + 33 - (Bit32s)poly->key;
		if (dynamicBiasPoint > 0) {
			dynamicBiasPoint = -dynamicBiasPoint;
			dynamicBiasPoint *= BiasLevel_MulTable[patchCache->srcPartial.tvf.biasLevel];
			cutoff += dynamicBiasPoint;
		}
	} else {
		dynamicBiasPoint = dynamicBiasPoint - 31 - (Bit32s)poly->key;
		if (dynamicBiasPoint <= 0) {
			dynamicBiasPoint *= BiasLevel_MulTable[patchCache->srcPartial.tvf.biasLevel];
			cutoff += dynamicBiasPoint;
		}
	}

	cutoff += (((Bit32s)patchCache->srcPartial.tvf.cutoff << 4) - 800);
	if (cutoff >= 0) {

		// FIXME: CC: Coarse pitch calculation placeholder until end-to-end use of MT-32-like pitch calculation
		// is application-wide.
		int oc  = poly->key + 12;
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

	int veloFilEnv = (Bit32s)poly->vel * (Bit32s)patchCache->srcPartial.tvf.envVeloSensitivity;
	int filEnv = (veloFilEnv << 2) >> 8;
	veloFilEnv  = 109 - patchCache->srcPartial.tvf.envVeloSensitivity + filEnv;
	filEnv = ((Bit32s)poly->key - 60) >> (4 - (Bit32s)patchCache->srcPartial.tvf.envDepthKeyfollow);
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

	int tmp = noteLookup->rfiltTable[cutoff + filEnv];
	return tmp;
}

bool Partial::shouldReverb() {
	if (!isActive())
		return false;
	return patchCache->reverb;
}

Bit32s Partial::getPitchEnvelope() {
	EnvelopeStatus *tStat = &envs[EnvelopeType_pitch];

	Bit32s tc;
	pitchSustain = false;
	if (tStat->decaying) {
		if (tStat->envpos >= tStat->envsize)
			tc = patchCache->pitchEnv.level[4];
		else {
			tc = tStat->envbase;
			tc = (tc + ((tStat->envdist * tStat->envpos) / tStat->envsize));
		}
	} else {
		if (tStat->envstat==3) {
			tc = patchCache->pitchsustain;
			if (poly->sustain)
				pitchSustain = true;
			else
				startDecay(EnvelopeType_pitch, tc);
		} else {
			if ((tStat->envstat==-1) || (tStat->envpos >= tStat->envsize)) {
				tStat->envstat++;

				tStat->envbase = patchCache->pitchEnv.level[tStat->envstat];

				Bit32u envTime = patchCache->pitchEnv.time[tStat->envstat];
				int envDiff = abs(patchCache->pitchEnv.level[tStat->envstat] - patchCache->pitchEnv.level[tStat->envstat + 1]);
				if (envTime > synth->tables.envDeltaMaxTime[envDiff]) {
					envTime = synth->tables.envDeltaMaxTime[envDiff];
				}

				tStat->envsize = (synth->tables.envTime[envTime] * keyLookup->envTimeMult[(int)patchCache->pitchEnv.timeKeyfollow]) >> 8;

				tStat->envpos = 0;
				tStat->envsize++;
				tStat->envdist = patchCache->pitchEnv.level[tStat->envstat + 1] - tStat->envbase;
			}
			tc = tStat->envbase;
			tc = (tc + ((tStat->envdist * tStat->envpos) / tStat->envsize));
		}
		tStat->prevlevel = tc;
	}
	return tc;
}

void Partial::startDecayAll() {
	tva->startDecay();
	startDecay(EnvelopeType_filt, envs[EnvelopeType_filt].prevlevel);
	startDecay(EnvelopeType_pitch, envs[EnvelopeType_pitch].prevlevel);
	pitchSustain = false;
}

void Partial::startDecay(EnvelopeType envnum, Bit32s startval) {
	EnvelopeStatus *tStat  = &envs[envnum];

	tStat->sustaining = false;
	tStat->decaying = true;
	tStat->envpos = 0;
	tStat->envbase = startval;

	switch (envnum) {
	case EnvelopeType_filt:
		tStat->envsize = FIXEDPOINT_UMULT(synth->tables.envDecayTime[(int)patchCache->filtEnv.envTime[4]], keyLookup->envTimeMult[(int)patchCache->filtEnv.envTimeKeyfollow], 8);
		tStat->envdist = -startval;
		break;
	case EnvelopeType_pitch:
		tStat->envsize = FIXEDPOINT_UMULT(synth->tables.envDecayTime[(int)patchCache->pitchEnv.time[3]], keyLookup->envTimeMult[(int)patchCache->pitchEnv.timeKeyfollow], 8);
		tStat->envdist = patchCache->pitchEnv.level[4] - startval;
		break;
	default:
		break;
	}
	tStat->envsize++;
}
