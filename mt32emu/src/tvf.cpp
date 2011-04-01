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

#include "mt32emu.h"
#include "mmath.h"

namespace MT32Emu {

// FIXME: Need to confirm that this is correct.
const unsigned int TVF_TARGET_MULT = 0x800000;
const unsigned int MAX_CURRENT = 0xFF * TVF_TARGET_MULT;

// When entering nextPhase, targetPhase is immediately incremented, and the descriptions/names below represent
// their use after the increment.
enum {
	// When this is the target phase, level[0] is targeted within time[0]
	// Note that this phase is always set up in reset(), not nextPhase()
	PHASE_ATTACK = 1,

	// When this is the target phase, level[1] is targeted within time[1]
	PHASE_2 = 2,

	// When this is the target phase, level[2] is targeted within time[2]
	PHASE_3 = 3,

	// When this is the target phase, level[3] is targeted within time[3]
	PHASE_4 = 4,

	// When this is the target phase, immediately goes to PHASE_RELEASE unless the poly is set to sustain.
	// Otherwise level[3] is continued with increment 0 - no phase change will occur until some external influence (like pedal release)
	PHASE_SUSTAIN = 5,

	// 0 is targeted within time[4] (the time calculation is quite different from the other phases)
	PHASE_RELEASE = 6,

	// 0 is targeted with increment 0 (thus theoretically staying that way forever)
	PHASE_DONE = 7
};

static int calcBaseCutoff(const TimbreParam::PartialParam *partialParam, Bit32u basePitch, unsigned int key) {
	// This table matches the values used by a real LAPC-I.
	static const Bit8s biasLevelToBiasMult[] = {85, 42, 21, 16, 10, 5, 2, 0, -2, -5, -10, -16, -21, -74, -85};
	// These values represent unique options with no consistent pattern, so we have to use something like a table in any case.
	// The table entries, when divided by 21, match approximately what the manual claims:
	// -1, -1/2, -1/4, 0, 1/8, 1/4, 3/8, 1/2, 5/8, 3/4, 7/8, 1, 5/4, 3/2, 2, s1, s2
	// Note that the entry for 1/8 is rounded to 2 (from 1/8 * 21 = 2.625), which seems strangely inaccurate compared to the others.
	static const Bit8s keyfollowMult21[] = {-21, -10, -5, 0, 2, 5, 8, 10, 13, 16, 18, 21, 26, 32, 42, 21, 21};
	int baseCutoff = keyfollowMult21[partialParam->tvf.keyfollow] - keyfollowMult21[partialParam->wg.pitchKeyfollow]; // baseCutoff range here: -63 to 63
	baseCutoff *= (int)key - 60;
	int biasPoint = partialParam->tvf.biasPoint;
	if ((biasPoint & 0x40) == 0) {
		int bias = biasPoint + 33 - key;
		if (bias > 0) {
			bias = -bias;
			baseCutoff += bias * biasLevelToBiasMult[partialParam->tvf.biasLevel];
		}
	} else {
		int bias = biasPoint - 31 - key;
		if (bias < 0) {
			baseCutoff += bias * biasLevelToBiasMult[partialParam->tvf.biasLevel];
		}
	}
	baseCutoff += ((partialParam->tvf.cutoff << 4) - 800);
	if (baseCutoff >= 0) {
		// FIXME: Potentially bad if initialCutoff ends up below -2056?
		int pitchDeltaThing = (basePitch >> 4) + baseCutoff - 3584;
		if (pitchDeltaThing > 0) {
			baseCutoff -= pitchDeltaThing;
		}
	} else if (baseCutoff < -2048) {
		baseCutoff = -2048;
	}
	baseCutoff += 2056;
	baseCutoff >>= 4; // PORTABILITY NOTE: Hmm... Depends whether it could've been below -2056, but maybe arithmetic shift assumed?
	if (baseCutoff > 255) {
		baseCutoff = 255;
	}
	return (Bit8u)baseCutoff;
}

TVF::TVF(const Partial *partial) :
	partial(partial) {
}

void TVF::setIncrement(Bit8u increment) {
	// FIXME: This is just a guess - absolutely no idea whether this is the same as for TVA::setAmpIncrement(), which it copies.
	this->increment = increment;

	bigIncrement = increment & 0x7F;
	// FIXME: We could use a table for this in future
	bigIncrement = (unsigned int)(EXP10F((bigIncrement - 1) / 26.0f) * 256.0f);
}

void TVF::reset(const TimbreParam::PartialParam *partialParam, unsigned int basePitch) {
	this->partialParam = partialParam;

	unsigned int key = partial->getPoly()->getKey();
	unsigned int velocity = partial->getPoly()->getVelocity();

	Tables *tables = &partial->getSynth()->tables;

	baseCutoff = calcBaseCutoff(partialParam, basePitch, key);

	int newLevelMult = velocity * partialParam->tvf.envVeloSensitivity;
	newLevelMult >>= 6;
	newLevelMult += 109 - partialParam->tvf.envVeloSensitivity;
	newLevelMult += ((signed)key - 60) >> (4 - partialParam->tvf.envDepthKeyfollow);
	if (newLevelMult < 0) {
		newLevelMult = 0;
	}
	newLevelMult *= partialParam->tvf.envDepth;
	newLevelMult >>= 6;
	if (newLevelMult > 255) {
		newLevelMult = 255;
	}
	levelMult = newLevelMult;

	if (partialParam->tvf.envTimeKeyfollow != 0) {
		keyTimeSubtraction = ((signed)key - 60) >> (5 - partialParam->tvf.envTimeKeyfollow);
	} else {
		keyTimeSubtraction = 0;
	}

	int newTarget = (newLevelMult * partialParam->tvf.envLevel[0]) >> 8;
	int envTimeSetting = partialParam->tvf.envTime[0] - keyTimeSubtraction;
	int newIncrement;
	if (envTimeSetting <= 0) {
		newIncrement = (0x80 | 127);
	} else {
		newIncrement = tables->envLogarithmicTime[newTarget] - envTimeSetting;
		if (newIncrement <= 0) {
			newIncrement = 1;
		}
	}
	setIncrement(newIncrement);
	target = newTarget;
	targetPhase = PHASE_2 - 1;

	current = 0;
}

Bit8u TVF::getBaseCutoff() const {
	return baseCutoff;
}

float TVF::nextCutoffModifier() {
	// FIXME: This whole method is basically a copy of TVA::nextAmp(), which may be completely inappropriate for TVF.
	Bit32u bigTarget = target * TVF_TARGET_MULT;
	if (increment == 0) {
		current = bigTarget;
	} else {
		if ((increment & 0x80) != 0) {
			// Lowering
			if (bigIncrement > current) {
				current = bigTarget;
				nextPhase();
			} else {
				current -= bigIncrement;
				if (current <= bigTarget) {
					current = bigTarget;
					nextPhase();
				}
			}
		} else {
			// Raising
			if (MAX_CURRENT - current < bigIncrement) {
				current = bigTarget;
				nextPhase();
			} else {
				current += bigIncrement;
				if (current >= bigTarget) {
					current = bigTarget;
					nextPhase();
				}
			}
		}
	}
	return (float)current / TVF_TARGET_MULT;
}

void TVF::startDecay() {
	if (targetPhase >= PHASE_RELEASE) {
		return;
	}
	targetPhase = PHASE_DONE - 1;
	if (partialParam->tvf.envTime[4] == 0) {
		setIncrement(1);
	} else {
		setIncrement(-partialParam->tvf.envTime[4]);
	}
	target = 0;
}

void TVF::nextPhase() {
	Tables *tables = &partial->getSynth()->tables;
	targetPhase++;

	switch (targetPhase) {
	case PHASE_DONE:
		setIncrement(0);
		target = 0;
		return;
	case PHASE_SUSTAIN:
	case PHASE_RELEASE:
		// FIXME: Afaict targetPhase should never be PHASE_RELEASE here. And if it were, this is an odd way to handle it.
		if (!partial->getPoly()->canSustain()) {
			startDecay(); // FIXME: This should actually start decay even if phase is already 6. Does that matter?
			return;
		}
		setIncrement(0);
		target = (levelMult * partialParam->tvf.envLevel[3]) >> 8;
		return;
	}

	int envPointIndex = targetPhase - 1;
	// FIXME: Should the result perhaps be cast to Bit8s?
	int envTimeSetting = (int)partialParam->tvf.envTime[envPointIndex] - keyTimeSubtraction;

	int newTarget = (levelMult * partialParam->tvf.envLevel[envPointIndex]) >> 8;
	int newIncrement;
	if (envTimeSetting > 0) {
		int targetDelta = newTarget - target;
		if (targetDelta == 0) {
			if (newTarget == 0) {
				targetDelta = 1;
				newTarget = 1;
			} else {
				targetDelta = -1;
				newTarget--;
			}
		}
		// FIXME: Can't newIncrement be negative or over 127 here? Casting to Bit8u to emulate...
		newIncrement = (Bit8u)(tables->envLogarithmicTime[targetDelta < 0 ? -targetDelta : targetDelta] - envTimeSetting);
		if (newIncrement <= 0) {
			newIncrement = 1;
		}
		if (targetDelta < 0) {
			newIncrement |= 0x80;
		}
	} else {
		newIncrement = newTarget >= target ? (0x80 | 127) : 127;
	}
	setIncrement(newIncrement);
	target = newTarget;
}

}
