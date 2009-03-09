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

#include "tva.h"

namespace MT32Emu {

class Part;

// CONFIRMED: Matches a table in ROM - haven't got around to coming up with a formula for it yet.
static Bit8u biasLevelToAmpSubtractionCoeff[13] = {255, 187, 137, 100, 74, 54, 40, 29, 21, 15, 10, 5, 0};

TVA::TVA(const Partial *partial) :
	partial(partial), system(&partial->getSynth()->mt32ram.system) {
}

static int multBias(const Tables *tables, Bit8u biasLevel, int bias) {
	return (bias * biasLevelToAmpSubtractionCoeff[biasLevel]) >> 5;
}

static int calcBiasAmpSubtraction(const Tables *tables, Bit8u biasPoint, Bit8u biasLevel, int key) {
	if (biasPoint & 0x40 == 0) {
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
	velocityMult  = (velocityMult * (key - 64)) << 2;
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
		return 155;
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

	targetPhase = 0;
	play = true;

	Tables *tables = &partial->getSynth()->tables;

	int key = partial->getPoly()->key;
	int velocity = partial->getPoly()->vel;

	keyTimeSubtraction = calcKeyTimeSubtraction(partialParam->tva.envTimeKeyfollow, key);

	biasAmpSubtraction = calcBiasAmpSubtractions(tables, partialParam, key);
	veloAmpSubtraction = calcVeloAmpSubtraction(tables, partialParam->tva.veloSensitivity, velocity);

	int newTargetAmp = calcBasicAmp(tables, partial, system, partialParam, patchTemp, rhythmTemp, biasAmpSubtraction, veloAmpSubtraction, part->getExpression());
	newTargetAmp -= partialParam->tvf.resonance >> 1;
	if (newTargetAmp < 0) {
		newTargetAmp = 0;
	}

	if (partialParam->tva.envTime[0] == 0) {
		newTargetAmp += partialParam->tva.envLevel[0];
		targetPhase = 1;
	} else {
		targetPhase = 0;
	}
	timeToTarget = 255;
	targetAmp = newTargetAmp;
}

void TVA::nextPhase() {
	Tables *tables = &partial->getSynth()->tables;

	if (targetPhase >= 7) {
		partial->getSynth()->printDebug("TVA::nextPhase(): Shouldn't have got here with targetPhase %d", targetPhase);
		return;
	}
	int currentPhase = targetPhase;
	targetPhase++;

	if (targetPhase == 7) {
		play = false;
		return;
	}

	int newTargetAmp;
	int newTimeToTarget;
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

	int newTargetamp;
	if (!allLevelsZeroFromNowOn) {
		newTargetAmp = calcBasicAmp(tables, partial, system, partialParam, patchTemp, rhythmTemp, biasAmpSubtraction, veloAmpSubtraction, part->getExpression());
		newTargetAmp -= partialParam->tvf.resonance >> 1;
		if (newTargetAmp < 0)
			newTargetAmp = 0;

		if (targetPhase == 5 || targetPhase == 6) {
			if (partialParam->tva.envLevel[3] == 0) {
				play = false;
				return;
			}
			if (!partial->getPoly()->sustain) {
				targetPhase = 6;
				newTargetAmp = 0;
				newTimeToTarget = 0 - partialParam->tva.envTime[4];
				if (newTimeToTarget >= 0) {
					newTimeToTarget++;
				}
			} else {
				newTargetAmp += partialParam->tva.envLevel[3];
				newTimeToTarget = 0;
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
			envTimeSetting -= keyTimeSubtraction;
		}
		if (envTimeSetting > 0)
		{
			newTimeToTarget = newTargetAmp - targetAmp;
			if (newTimeToTarget <= 0) {
				if (newTimeToTarget == 0) {
					newTimeToTarget--; // i.e. newTimeToTarget = -1;
					newTargetAmp--;
					if (newTargetAmp < 0) {
						newTimeToTarget = -newTimeToTarget;
						newTargetAmp = -newTargetAmp;
					}
				}
				newTimeToTarget = -newTimeToTarget;
				newTimeToTarget = tables->envLogarithmicTime[(Bit8u)newTimeToTarget] - envTimeSetting;
				if (newTimeToTarget <= 0) {
					newTimeToTarget = 1;
				}
				newTimeToTarget = newTimeToTarget | 0x80;
			} else {
				newTimeToTarget = tables->envLogarithmicTime[(Bit8u)newTimeToTarget] - envTimeSetting;
				if (newTimeToTarget <= 0) {
					newTimeToTarget = 1;
				}
			}
		}
		else
			newTimeToTarget = newTargetAmp >= targetAmp ? -1 : 127;

		if (newTimeToTarget == 0) {
			newTimeToTarget = 1;
		}
	}

	targetAmp = newTargetAmp;
	timeToTarget = newTimeToTarget;
}

}
