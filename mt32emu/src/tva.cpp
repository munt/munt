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

#include <math.h>

namespace MT32Emu {

// FIXME: Need to confirm that this is correct. Sounds about right.
const int TVA_TARGET_AMP_MULT = 0x800000;
const int MAX_CURRENT_AMP = 0xFF * TVA_TARGET_AMP_MULT;

// CONFIRMED: Matches a table in ROM - haven't got around to coming up with a formula for it yet.
static Bit8u biasLevelToAmpSubtractionCoeff[13] = {255, 187, 137, 100, 74, 54, 40, 29, 21, 15, 10, 5, 0};

// When entering nextPhase, targetPhase is immediately incremented, and the descriptions/names below represent
// their use after the increment.
enum {
	// When this is the target phase, level[0] is targeted within time[0] are used and velocity is applied
	PHASE_ATTACK = 1,

	// When this is the target phase, level[1] is targeted within time[1]
	PHASE_2 = 2,

	// When this is the target phase, level[2] is targeted within time[2]
	PHASE_3 = 3,

	// When this is the target phase, level[3] is targeted within time[3]
	PHASE_4 = 4,

	// When this is the target phase, immediately goes to PHASE_RELEASE unless sustain pedal is held.
	// Aborts if level[3] is 0.
	// level[3] is continued, no phase change will occur until some external influence (like pedal release)
	PHASE_SUSTAIN = 5,

	// 0 is targeted within time[4] (the time calculation is quite different from the other phases)
	PHASE_RELEASE = 6,

	// It's PHASE_DEAD, Jim.
	PHASE_DEAD = 7
};

TVA::TVA(const Partial *partial) :
	partial(partial), system(&partial->getSynth()->mt32ram.system) {
}

void TVA::setAmpIncrement(Bit8u ampIncrement) {
	this->ampIncrement = ampIncrement;

	largeAmpInc = ampIncrement & 0x7F;
	// FIXME: We could use a table for this in future
	largeAmpInc = (unsigned int)((powf(10.0f, (float)((largeAmpInc - 1.0f) / 26.0f))) * 256.0f);
}

Bit32u TVA::nextAmp() {
	// FIXME: This whole method is based on guesswork
	Bit32u target = targetAmp * TVA_TARGET_AMP_MULT;
	if (ampIncrement == 0) {
		currentAmp = target;
		return currentAmp / TVA_TARGET_AMP_MULT;
	}
	if ((ampIncrement & 0x80) != 0) {
		// Lowering amp
		if (largeAmpInc > currentAmp) {
			currentAmp = target;
			nextPhase();
		} else {
			currentAmp -= largeAmpInc;
			if (currentAmp <= target) {
				currentAmp = target;
				nextPhase();
			}
		}
	} else {
		// Raising amp
		if (MAX_CURRENT_AMP - currentAmp < largeAmpInc ) {
			currentAmp = target;
			nextPhase();
		} else {
			currentAmp += largeAmpInc;
			if(currentAmp >= target) {
				currentAmp = target;
				nextPhase();
			}
		}
	}
	return currentAmp / TVA_TARGET_AMP_MULT;
}

static int multBias(const Tables *tables, Bit8u biasLevel, int bias) {
	return (bias * biasLevelToAmpSubtractionCoeff[biasLevel]) >> 5;
}

static int calcBiasAmpSubtraction(const Tables *tables, Bit8u biasPoint, Bit8u biasLevel, int key) {
	if ((biasPoint & 0x40) == 0) {
		int bias = biasPoint + 33 - key;
		if (bias > 0) {
			return multBias(tables, biasLevel, bias);
		}
	} else {
		int bias = biasPoint - 31 - key;
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

static int calcVeloAmpSubtraction(const Tables *tables, Bit8u veloSensitivity, int velocity) {
	// FIXME:KG: Better variable names
	int velocityMult = veloSensitivity - 50;
	int absVelocityMult = velocityMult < 0 ? -velocityMult : velocityMult;
	velocityMult = (velocityMult * (velocity - 64)) << 2;
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

int calcKeyTimeSubtraction(Bit8u envTimeKeyfollow, int key) {
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

	keyTimeSubtraction = calcKeyTimeSubtraction(partialParam->tva.envTimeKeyfollow, key);

	biasAmpSubtraction = calcBiasAmpSubtractions(tables, partialParam, key);
	veloAmpSubtraction = calcVeloAmpSubtraction(tables, partialParam->tva.veloSensitivity, velocity);

	int newTargetAmp = calcBasicAmp(tables, partial, system, partialParam, patchTemp, rhythmTemp, biasAmpSubtraction, veloAmpSubtraction, part->getExpression());

	if (partialParam->tva.envTime[0] == 0) {
		newTargetAmp += partialParam->tva.envLevel[0];
		targetPhase = PHASE_2 - 1; // The first target used in nextPhase() will be PHASE_2
	} else {
		targetPhase = PHASE_ATTACK - 1; // The first target used in nextPhase() will be PHASE_ATTACK
	}
	setAmpIncrement(0x80 | 127); // Go downward as quickly as possible.
	targetAmp = (Bit8u)newTargetAmp;

	currentAmp = 0;
}

void TVA::startDecay() {
	if (targetPhase >= PHASE_RELEASE)
		return;
	targetPhase = PHASE_RELEASE; // The next time nextPhase() is called, it will think PHASE_RELEASE has finished and the partial will be aborted
	Bit8u newAmpIncrement = -partialParam->tva.envTime[4];
	// FIXME: Obvious bug here (should be checking newAmpIncrement signed, now decay is always 1!)
	if (newAmpIncrement >= 0) {
		newAmpIncrement++; // FIXME: Wtf? Aiui this will raise the volume, not lower it, and therefore never hit the target...
	}
	setAmpIncrement(newAmpIncrement);
	targetAmp = 0;
}

void TVA::recalcSustain() {
	// We get pinged periodically by the pitch code to recalculate our values when in sustain.
	// This is done so that the TVA will respond to things like MIDI expression and volume changes while it's sustaining, which it otherwise wouldn't do.

	// The check for envLevel[3] == 0 strikes me as slightly dumb. FIXME: Explain why
	if (targetPhase != PHASE_SUSTAIN || partialParam->tva.envLevel[3] == 0)
		return;
	// We're sustaining. Recalculate all the values
	Tables *tables = &partial->getSynth()->tables;
	int newTargetAmp = calcBasicAmp(tables, partial, system, partialParam, patchTemp, rhythmTemp, biasAmpSubtraction, veloAmpSubtraction, part->getExpression());
	newTargetAmp += partialParam->tva.envLevel[3];
	// FIXME: This whole concept seems flawed.  We don't really know what the *actual* amp value is, right? It may well not be targetAmp yet (unless I've missed something). So we could end up going in the wrong direction...
	int ampDelta = newTargetAmp - targetAmp;

	// Calculate an increment to get to the new amp value in a short, more or less consistent amount of time
	if (ampDelta >= 0) {
		setAmpIncrement(tables->envLogarithmicTime[(Bit8u)ampDelta] - 2);
	} else {
		setAmpIncrement((tables->envLogarithmicTime[(Bit8u)-ampDelta] - 2) | 0x80);
	}
	targetAmp = newTargetAmp;
	// Configure so that once the transition's complete and nextPhase() is called, we'll just re-enter sustain phase (or decay phase, depending on parameters at the time).
	targetPhase = PHASE_SUSTAIN - 1;
}

void TVA::nextPhase() {
	Tables *tables = &partial->getSynth()->tables;

	if (targetPhase >= PHASE_DEAD || !play) {
		partial->getSynth()->printDebug("TVA::nextPhase(): Shouldn't have got here with targetPhase %d, play=%s", targetPhase, play ? "true" : "false");
		return;
	}
	targetPhase++;

	if (targetPhase == PHASE_DEAD) {
		play = false;
		return;
	}

	bool allLevelsZeroFromNowOn = false;
	if (partialParam->tva.envLevel[3] == 0) {
		if (targetPhase == PHASE_4)
			allLevelsZeroFromNowOn = true;
		else if (partialParam->tva.envLevel[2] == 0) {
			if (targetPhase == PHASE_3)
				allLevelsZeroFromNowOn = true;
			else if (partialParam->tva.envLevel[1] == 0) {
				if (targetPhase == PHASE_2)
					allLevelsZeroFromNowOn = true;
				else if (partialParam->tva.envLevel[0] == 0) {
					if (targetPhase == PHASE_ATTACK) // this line added, missing in ROM - FIXME: Add description of repercussions
						allLevelsZeroFromNowOn = true;
				}
			}
		}
	}

	int newTargetAmp;
	int newAmpIncrement;
	int envPointIndex = targetPhase - 1;

	if (!allLevelsZeroFromNowOn) {
		newTargetAmp = calcBasicAmp(tables, partial, system, partialParam, patchTemp, rhythmTemp, biasAmpSubtraction, veloAmpSubtraction, part->getExpression());

		if (targetPhase == PHASE_SUSTAIN || targetPhase == PHASE_RELEASE) {
			if (partialParam->tva.envLevel[3] == 0) {
				play = false;
				return;
			}
			if (!partial->getPoly()->sustain) {
				targetPhase = PHASE_RELEASE;
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
			newTargetAmp += partialParam->tva.envLevel[envPointIndex];
		}
	} else {
		newTargetAmp = 0;
	}

	if ((targetPhase != PHASE_SUSTAIN && targetPhase != PHASE_RELEASE) || allLevelsZeroFromNowOn) {
		int envTimeSetting  = partialParam->tva.envTime[envPointIndex];

		if (targetPhase == 1) {
			envTimeSetting -= (partial->getPoly()->vel - 64) >> (6 - partialParam->tva.envTimeVeloSensitivity);

			if (envTimeSetting <= 0 && partialParam->tva.envTime[envPointIndex] != 0) {
					envTimeSetting = 1;
			}
		} else {
			envTimeSetting -= keyTimeSubtraction;
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
	setAmpIncrement((Bit8u)newAmpIncrement);
}

}
