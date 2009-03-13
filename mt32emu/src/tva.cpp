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

#include "mt32emu.h"

namespace MT32Emu {

// CONFIRMED: Matches a table in ROM - haven't got around to coming up with a formula for it yet.
static Bit8u biasLevelToAmpSubtractionCoeff[13] = {255, 187, 137, 100, 74, 54, 40, 29, 21, 15, 10, 5, 0};

TVA::TVA(const Partial *partial) :
	partial(partial), system(&partial->getSynth()->mt32ram.system) {
}

Bit32u TVA::nextAmp() {
	// FIXME: This whole method is based on guesswork
	Bit8u absAmpIncrement = ampIncrement & 0x7F;
	Bit32u target = targetAmp * 0x10000;
	if ((ampIncrement & 0x80) != 0) {
		// Lowering amp
		if (absAmpIncrement > currentAmp) {
			currentAmp = 0;
			nextPhase();
		} else {
			currentAmp -= absAmpIncrement;
			if(currentAmp <= target)
				nextPhase();
		}
	} else {
		// Raising amp
		if(0xFF0000 - currentAmp < absAmpIncrement) {
			currentAmp = 0xFF0000;
			nextPhase();
		} else {
			currentAmp += absAmpIncrement;
			if(currentAmp >= target)
				nextPhase();
		}
	}
	return currentAmp;
}

static int multBias(const Tables *tables, Bit8u biasLevel, int bias) {
	return (bias * biasLevelToAmpSubtractionCoeff[biasLevel]) >> 5;
}

static int calcBiasAmpSubtraction(const Tables *tables, Bit8u biasPoint, Bit8u biasLevel, int key) {
	if ((biasPoint & 0x40) == 0) {
		int bias = 33 - key;
		if (bias > 0) {
			return multBias(tables, biasLevel, bias);
		}
	} else {
		int bias = -31 - key;
		if (bias < 0) {
			bias = -bias;
			return multBias(tables, biasLevel, bias);
		}
	}
	return 0;
}

static int calcBiasAmpSubtractions(const Tables *tables, const TimbreParam::PartialParam *partialParam, int key) {
	int biasAmpSubtraction1 = calcBiasAmpSubtraction(tables, partialParam->tva.biasPoint1, partialParam->tva.biasLevel1, key);
	if (biasAmpSubtraction1 > 255)
		return 255;
	int biasAmpSubtraction2 = calcBiasAmpSubtraction(tables, partialParam->tva.biasPoint2, partialParam->tva.biasLevel2, key);
	if (biasAmpSubtraction2 > 255)
		return 255;
	int biasAmpSubtraction = biasAmpSubtraction1 + biasAmpSubtraction2;
	if (biasAmpSubtraction > 255)
		return 255;
	return biasAmpSubtraction;
}

static int calcVeloAmpSubtraction(const Tables *tables, Bit8u veloSensitivity, int key) {
	// FIXME:KG: Better variable names
	int velocityMult = veloSensitivity - 50;
	int absVelocityMult = velocityMult < 0 ? -velocityMult : velocityMult;
	velocityMult = (velocityMult * (key - 64)) << 2;
	return absVelocityMult - (velocityMult >> 8);
}

static int calcBasicAmp(const Tables *tables, const Partial *partial, const MemParams::System *system, const TimbreParam::PartialParam *partialParam, const MemParams::PatchTemp *patchTemp, const MemParams::RhythmTemp *rhythmTemp, int biasAmpSubtraction, int veloAmpSubtraction, Bit8u expression) {
	int amp = 155;

	if (!partial->isRingModulatingSlave()) {
		amp -= tables->masterVolToAmpSubtraction[system->masterVol];
		if (amp < 0)
			return 0;
		amp -= tables->levelToAmpSubtraction[patchTemp->outputLevel];
		if (amp < 0)
			return 0;
		amp -= tables->levelToAmpSubtraction[expression];
		if (amp < 0)
			return 0;
		if (rhythmTemp != NULL) {
			amp -= tables->levelToAmpSubtraction[rhythmTemp->outputLevel];
			if (amp < 0)
				return 0;
		}
	}
	amp -= biasAmpSubtraction;
	if (amp < 0)
		return 0;
	amp -= tables->levelToAmpSubtraction[partialParam->tva.level];
	if (amp < 0)
		return 0;
	amp -= veloAmpSubtraction;
	if (amp < 0)
		return 0;
	if (amp > 155)
		amp = 155;
	amp -= partialParam->tvf.resonance >> 1;
	if (amp < 0)
		return 0;
	return amp;
}

int calcKeyTimeAmpSubtraction(Bit8u envTimeKeyfollow, int key) {
	if (envTimeKeyfollow == 0)
		return 0;
	return (key - 60) >> (5 - envTimeKeyfollow);
}

void TVA::reset(const Part *part, const PatchCache *patchCache) {
	this->part = part;
	this->partialParam = patchCache->partialParam;
	this->patchTemp = part->getPatchTemp();
	this->rhythmTemp = part->getRhythmTemp();

	play = true;

	Tables *tables = &partial->getSynth()->tables;

	int key = partial->getPoly()->key;
	int velocity = partial->getPoly()->vel;

	keyTimeAmpSubtraction = calcKeyTimeAmpSubtraction(partialParam->tva.envTimeKeyfollow, key);

	biasAmpSubtraction = calcBiasAmpSubtractions(tables, partialParam, key);
	veloAmpSubtraction = calcVeloAmpSubtraction(tables, partialParam->tva.veloSensitivity, velocity);

	int newTargetAmp = calcBasicAmp(tables, partial, system, partialParam, patchTemp, rhythmTemp, biasAmpSubtraction, veloAmpSubtraction, part->getExpression());

	if (partialParam->tva.envTime[0] == 0) {
		newTargetAmp += partialParam->tva.envLevel[0];
		targetPhase = 1;
	} else {
		targetPhase = 0;
	}
	ampIncrement = (0x80 | 127); // Go downward as quickly as possible. FIXME: Check this is right
	targetAmp = (Bit8u)newTargetAmp;

	currentAmp = 0;
}

void TVA::startDecay() {
	if (targetPhase >= 6)
		return;
	targetPhase = 6;
	ampIncrement = -partialParam->tva.envTime[4];
	if (ampIncrement >= 0) {
		ampIncrement += 1; // I.e. if (ampIncrement == 0) ampIncrement = 1. FIXME: Wtf? Aiui this will raise the volume, not lower it, and therefore never hit the target...
	}
	targetAmp = 0;
}

void TVA::recalcSustain() {
	// We get pinged periodically by the pitch code to recalculate our values when in sustain
	// This is done so that the TVA will respond to things like MIDI expression and volume changes while it's sustaining, which it otherwise wouldn't do.

	if (partial->tva->targetPhase != 5 || partialParam->tva.envLevel[3] == 0)
		return;
	// We're sustaining. Recalculate all the values
	Tables *tables = &partial->getSynth()->tables;
	int newTargetAmp = calcBasicAmp(tables, partial, system, partialParam, patchTemp, rhythmTemp, biasAmpSubtraction, veloAmpSubtraction, part->getExpression());
	newTargetAmp += partialParam->tva.envLevel[3];
	// FIXME: This whole concept seems flawed.  We don't really know what the *actual* amp value is, right? It may well not be targetAmp yet (unless I've missed something). So we could end up going in the wrong direction...
	int ampDelta = newTargetAmp - targetAmp;

	// Calculate an increment to get to the new amp value in a short, more or less consistent amount of time
	int newAmpIncrement;
	if (ampDelta >= 0) {
		newAmpIncrement = tables->envLogarithmicTime[(Bit8u)ampDelta] - 2;
	} else {
		newAmpIncrement = (tables->envLogarithmicTime[(Bit8u)-ampDelta] - 2) | 0x80;
	}
	targetAmp = newTargetAmp;
	ampIncrement = newAmpIncrement;
	// Configure so that once the transition's complete and nextPhase() is called, we'll just re-enter sustain phase (or decay phase, depending on parameters at the time).
	targetPhase = 4;
}

void TVA::nextPhase() {
	Tables *tables = &partial->getSynth()->tables;

	if (targetPhase >= 7 || !play) {
		partial->getSynth()->printDebug("TVA::nextPhase(): Shouldn't have got here with targetPhase %d, play=%s", targetPhase, play ? "true" : "false");
		return;
	}
	int currentPhase = targetPhase;
	targetPhase++;

	if (targetPhase == 7) {
		play = false;
		return;
	}

	bool allLevelsZeroFromNowOn = false;
	if (partialParam->tva.envLevel[3] == 0) {
		if (targetPhase == 4)
			allLevelsZeroFromNowOn = true;
		else if (partialParam->tva.envLevel[2] == 0) {
			if (targetPhase == 3)
				allLevelsZeroFromNowOn = true;
			else if (partialParam->tva.envLevel[1] == 0) {
				if (targetPhase == 2)
					allLevelsZeroFromNowOn = true;
				else if (partialParam->tva.envLevel[0] == 0) {
					if (targetPhase == 1) // this line added, missing in ROM - FIXME: Add description of repercussions
						allLevelsZeroFromNowOn = true;
				}
			}
		}
	}

	int newTargetAmp;
	int newAmpIncrement;

	if (!allLevelsZeroFromNowOn) {
		newTargetAmp = calcBasicAmp(tables, partial, system, partialParam, patchTemp, rhythmTemp, biasAmpSubtraction, veloAmpSubtraction, part->getExpression());

		if (targetPhase == 5 || targetPhase == 6) {
			if (partialParam->tva.envLevel[3] == 0) {
				play = false;
				return;
			}
			if (!partial->getPoly()->sustain) {
				targetPhase = 6;
				newTargetAmp = 0;
				newAmpIncrement = -partialParam->tva.envTime[4];
				if (newAmpIncrement >= 0) {
					// FIXME: This must mean newAmpIncrement was 0, and we're now making it 1, which makes us go in the wrong direction. WTF?
					newAmpIncrement++;
				}
			} else {
				newTargetAmp += partialParam->tva.envLevel[3];
				newAmpIncrement = 0;
			}
		} else {
			newTargetAmp += partialParam->tva.envLevel[currentPhase];
		}
	} else {
		newTargetAmp = 0;
	}

	if ((targetPhase != 5 && targetPhase != 6) || allLevelsZeroFromNowOn) {
		int envTimeSetting  = partialParam->tva.envTime[currentPhase];

		if (targetPhase == 1) {
			envTimeSetting -= (partial->getPoly()->vel - 64) >> (6 - partialParam->tva.envTimeVeloSensitivity);

			if (envTimeSetting <= 0 && partialParam->tva.envTime[currentPhase] != 0) {
					envTimeSetting = 1;
			}
		} else {
			envTimeSetting -= keyTimeAmpSubtraction;
		}
		if (envTimeSetting > 0)
		{
			int ampDelta = newTargetAmp - targetAmp;
			if (ampDelta <= 0) {
				if (ampDelta == 0) {
					// targetAmp and newTargetAmp are the same.
					// We'd never get anywhere if we used these parameters, so instead make targetAmp one less than it really should be and set ampDelta accordingly
					ampDelta--; // i.e. ampDelta = -1;
					newTargetAmp--;
					if (newTargetAmp < 0) {
						// Oops, newTargetAmp is less than zero now, so let's do it the other way:
						// Make newTargetAmp one more than it really should've been and set ampDelta accordingly
						// FIXME: This means ampDelta will be positive just below here where it's inverted, and we'll end up using envLogarithmicTime[-1], and we'll be setting newAmpIncrement to be descending later on, etc..
						ampDelta = -ampDelta; // i.e. ampDelta = 1;
						newTargetAmp = -newTargetAmp;
					}
				}
				ampDelta = -ampDelta;
				newAmpIncrement = tables->envLogarithmicTime[(Bit8u)ampDelta] - envTimeSetting;
				if (newAmpIncrement <= 0) {
					newAmpIncrement = 1;
				}
				newAmpIncrement = newAmpIncrement | 0x80;
			} else {
				// FIXME: The last 22 or so entries in this table are 128 - surely that fucks things up, since that ends up being -128 signed?
				newAmpIncrement = tables->envLogarithmicTime[(Bit8u)ampDelta] - envTimeSetting;
				if (newAmpIncrement <= 0) {
					newAmpIncrement = 1;
				}
			}
		}
		else
		{
			// FIXME: Shouldn't we be ensuring that targetAmp != newTargetAmp here?
			newAmpIncrement = newTargetAmp >= targetAmp ? (0x80 | 127) : 127;
		}

		// FIXME: What's the point of this? It's checked or set to non-zero everywhere above
		if (newAmpIncrement == 0) {
			newAmpIncrement = 1;
		}
	}

	targetAmp = (Bit8u)newTargetAmp;
	ampIncrement = (Bit8u)newAmpIncrement;
}

}
