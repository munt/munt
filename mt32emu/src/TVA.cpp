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

/*
Some notes on this class:

la32AmpTarget and la32AmpIncrement represent memory-mapped LA32 registers in the real devices.
The values that we set them to correspond exactly to the values that the real control ROM sets
(according to Mok's specifications, and assuming no bugs in our implementation).

Our interpretation of these values is partly based on guesswork and sample analysis.
Here's what we're pretty confident about:
 - The most significant bit of la32AmpIncrement indicates the direction that the LA32's current internal amp value (currentAmp in our emulation) should change in.
   Set means downward, clear means upward.
 - The lower 7 bits of la32AmpIncrement indicate how quickly currentAmp should be changed.
 - If la32AmpIncrement is 0, no change to currentAmp is made and no interrupt is raised. [SEMI-CONFIRMED by sample analysis]
 - Otherwise, if the MSb is set:
    - If currentAmp already corresponds to a value <= la32AmpTarget, currentAmp is set immediately to the equivalent of la32AmpTarget and an interrupt is raised.
    - Otherwise, currentAmp is gradually reduced (at a rate determined by the lower 7 bits of la32AmpIncrement), and once it reaches the equivalent of la32AmpTarget an interrupt is raised.
 - Otherwise (the MSb is unset):
    - If currentAmp already corresponds to a value >= la32AmpTarget, currentAmp is set immediately to the equivalent of la32AmpTarget and an interrupt is raised.
    - Otherwise, currentAmp is gradually increased (at a rate determined by the lower 7 bits of la32AmpIncrement), and once it reaches the equivalent of la32AmpTarget an interrupt is raised.
We're emulating what happens when the interrupt is raised in "nextPhase()".
*/
#include <cmath>

#include "mt32emu.h"
#include "mmath.h"

namespace MT32Emu {

// SEMI-CONFIRMED from sample analysis.
const int TVA_AMP_TARGET_MULT = 0x40000;
const unsigned int MAX_CURRENT_AMP = 0xFF * TVA_AMP_TARGET_MULT;

// We simulate the delay in handling "target was reached" interrupts by waiting
// this many samples before calling nextPhase().
// FIXME: This should vary with the sample rate, but doesn't.
// SEMI-CONFIRMED: Since this involves asynchronous activity between the LA32
// and the 8095, a good value is hard to pin down.
// This one matches observed behaviour on a few digital captures I had handy,
// and should be double-checked. We may also need a more complicated delay
// scheme eventually.
const int INTERRUPT_TIME = 7;

// CONFIRMED: Matches a table in ROM - haven't got around to coming up with a formula for it yet.
static Bit8u biasLevelToAmpSubtractionCoeff[13] = {255, 187, 137, 100, 74, 54, 40, 29, 21, 15, 10, 5, 0};

TVA::TVA(const Partial *usePartial) :
	partial(usePartial), system(&usePartial->getSynth()->mt32ram.system) {
}

void TVA::startRamp(Bit8u newLA32AmpTarget, Bit8u newLA32AmpIncrement, int newPhase) {
	la32AmpIncrement = newLA32AmpIncrement;

	largeAmpInc = newLA32AmpIncrement & 0x7F;
	// CONFIRMED: From sample analysis, this appears to be very accurate.
	// FIXME: We could use a table for this in future
	largeAmpInc = (unsigned int)(EXP2F((largeAmpInc + 24) / 8.0f) + 0.125f);
	if ((newLA32AmpIncrement & 0x80) != 0) {
		// CONFIRMED: From sample analysis, descending increments are slightly faster
		largeAmpInc++;
	}

	la32AmpTarget = newLA32AmpTarget;
	phase = newPhase;
	interruptCountdown = 0;

#if MT32EMU_MONITOR_TVA >= 1
	partial->getSynth()->printDebug("TVA,ramp,%d,%d,%d,%d,%d", newLA32AmpTarget, (newLA32AmpIncrement & 0x80) ? -1 : 1, (newLA32AmpIncrement & 0x7F), newPhase, currentAmp);
#endif
}

void TVA::end(int newPhase) {
	phase = newPhase;
	playing = false;
#if MT32EMU_MONITOR_TVA >= 1
	partial->getSynth()->printDebug("TVA,end,%d", newPhase);
#endif
}

float TVA::nextAmp() {
	// FIXME: This whole method is based on guesswork
	Bit32u target = la32AmpTarget * TVA_AMP_TARGET_MULT;
	if (interruptCountdown > 0) {
		if (--interruptCountdown == 0) {
			nextPhase();
		}
	} else if (la32AmpIncrement != 0) {
		// CONFIRMED from sample analysis: When la32AmpIncrement is 0, the LA32 does *not* change the amp at all (and of course doesn't fire an interrupt).
		if ((la32AmpIncrement & 0x80) != 0) {
			// Lowering amp
			if (largeAmpInc > currentAmp) {
				currentAmp = target;
				interruptCountdown = INTERRUPT_TIME;
			} else {
				currentAmp -= largeAmpInc;
				if (currentAmp <= target) {
					currentAmp = target;
					interruptCountdown = INTERRUPT_TIME;
				}
			}
		} else {
			// Raising amp
			if (MAX_CURRENT_AMP - currentAmp < largeAmpInc) {
				currentAmp = target;
				interruptCountdown = INTERRUPT_TIME;
			} else {
				currentAmp += largeAmpInc;
				if (currentAmp >= target) {
					currentAmp = target;
					interruptCountdown = INTERRUPT_TIME;
				}
			}
		}
	}
#if MT32EMU_MONITOR_TVA >= 2
	partial->getSynth()->printDebug("TVA,next,%d,%d,%d", currentAmp, target, largeAmpInc);
#endif
	// SEMI-CONFIRMED: From sample analysis, this gives results within +/- 2 at the output (before any DAC bitshifting)
	// when sustaining at levels 156 - 255 with no modifiers.
	// Tested with a single partial playing PCM wave 77 with pitchCoarse 36 and no keyfollow, velocity follow, etc.
	// What isn't yet confirmed is the behaviour when ramping between levels, as well as the timing.
	int cAmp = currentAmp / (TVA_AMP_TARGET_MULT / 128);
	return EXP2F((32792 - cAmp) / -2048.0f);
}

static int multBias(Bit8u biasLevel, int bias) {
	return (bias * biasLevelToAmpSubtractionCoeff[biasLevel]) >> 5;
}

static int calcBiasAmpSubtraction(Bit8u biasPoint, Bit8u biasLevel, int key) {
	if ((biasPoint & 0x40) == 0) {
		int bias = biasPoint + 33 - key;
		if (bias > 0) {
			return multBias(biasLevel, bias);
		}
	} else {
		int bias = biasPoint - 31 - key;
		if (bias < 0) {
			bias = -bias;
			return multBias(biasLevel, bias);
		}
	}
	return 0;
}

static int calcBiasAmpSubtractions(const TimbreParam::PartialParam *partialParam, int key) {
	int biasAmpSubtraction1 = calcBiasAmpSubtraction(partialParam->tva.biasPoint1, partialParam->tva.biasLevel1, key);
	if (biasAmpSubtraction1 > 255) {
		return 255;
	}
	int biasAmpSubtraction2 = calcBiasAmpSubtraction(partialParam->tva.biasPoint2, partialParam->tva.biasLevel2, key);
	if (biasAmpSubtraction2 > 255) {
		return 255;
	}
	int biasAmpSubtraction = biasAmpSubtraction1 + biasAmpSubtraction2;
	if (biasAmpSubtraction > 255) {
		return 255;
	}
	return biasAmpSubtraction;
}

static int calcVeloAmpSubtraction(Bit8u veloSensitivity, unsigned int velocity) {
	// FIXME:KG: Better variable names
	int velocityMult = veloSensitivity - 50;
	int absVelocityMult = velocityMult < 0 ? -velocityMult : velocityMult;
	velocityMult = (signed)((unsigned)(velocityMult * ((signed)velocity - 64)) << 2);
	return absVelocityMult - (velocityMult >> 8); // PORTABILITY NOTE: Assumes arithmetic shift
}

static int calcBasicAmp(const Tables *tables, const Partial *partial, const MemParams::System *system, const TimbreParam::PartialParam *partialParam, const MemParams::PatchTemp *patchTemp, const MemParams::RhythmTemp *rhythmTemp, int biasAmpSubtraction, int veloAmpSubtraction, Bit8u expression) {
	int amp = 155;

	if (!partial->isRingModulatingSlave()) {
		amp -= tables->masterVolToAmpSubtraction[system->masterVol];
		if (amp < 0) {
			return 0;
		}
		amp -= tables->levelToAmpSubtraction[patchTemp->outputLevel];
		if (amp < 0) {
			return 0;
		}
		amp -= tables->levelToAmpSubtraction[expression];
		if (amp < 0) {
			return 0;
		}
		if (rhythmTemp != NULL) {
			amp -= tables->levelToAmpSubtraction[rhythmTemp->outputLevel];
			if (amp < 0) {
				return 0;
			}
		}
	}
	amp -= biasAmpSubtraction;
	if (amp < 0) {
		return 0;
	}
	amp -= tables->levelToAmpSubtraction[partialParam->tva.level];
	if (amp < 0) {
		return 0;
	}
	amp -= veloAmpSubtraction;
	if (amp < 0) {
		return 0;
	}
	if (amp > 155) {
		amp = 155;
	}
	amp -= partialParam->tvf.resonance >> 1;
	if (amp < 0) {
		return 0;
	}
	return amp;
}

int calcKeyTimeSubtraction(Bit8u envTimeKeyfollow, int key) {
	if (envTimeKeyfollow == 0) {
		return 0;
	}
	return (key - 60) >> (5 - envTimeKeyfollow); // PORTABILITY NOTE: Assumes arithmetic shift
}

void TVA::reset(const Part *newPart, const TimbreParam::PartialParam *newPartialParam, const MemParams::RhythmTemp *newRhythmTemp) {
	part = newPart;
	partialParam = newPartialParam;
	patchTemp = newPart->getPatchTemp();
	rhythmTemp = newRhythmTemp;

	playing = true;

	Tables *tables = &partial->getSynth()->tables;

	int key = partial->getPoly()->getKey();
	int velocity = partial->getPoly()->getVelocity();

	keyTimeSubtraction = calcKeyTimeSubtraction(partialParam->tva.envTimeKeyfollow, key);

	biasAmpSubtraction = calcBiasAmpSubtractions(partialParam, key);
	veloAmpSubtraction = calcVeloAmpSubtraction(partialParam->tva.veloSensitivity, velocity);

	int newAmpTarget = calcBasicAmp(tables, partial, system, partialParam, patchTemp, newRhythmTemp, biasAmpSubtraction, veloAmpSubtraction, part->getExpression());
	int newPhase;
	if (partialParam->tva.envTime[0] == 0) {
		// Initially go to the TVA_PHASE_ATTACK target amp, and spend the next phase going from there to the TVA_PHASE_2 target amp
		// Note that this means that velocity never affects time for this partial.
		newAmpTarget += partialParam->tva.envLevel[0];
		newPhase = TVA_PHASE_ATTACK; // The first target used in nextPhase() will be TVA_PHASE_2
	} else {
		// Initially go to the base amp determined by TVA level, part volume, etc., and spend the next phase going from there to the full TVA_PHASE_ATTACK target amp.
		newPhase = TVA_PHASE_BASIC; // The first target used in nextPhase() will be TVA_PHASE_ATTACK
	}

	currentAmp = 0;

	// "Go downward as quickly as possible".
	// Since currentAmp is 0, nextAmp() will notice that we're already at or below the target and trying to go downward,
	// and therefore jump to the target immediately and call nextPhase().
	startRamp((Bit8u)newAmpTarget, 0x80 | 127, newPhase);
}

void TVA::startAbort() {
	startRamp(64, 0x80 | 127, TVA_PHASE_RELEASE);
}

void TVA::startDecay() {
	if (phase >= TVA_PHASE_RELEASE) {
		return;
	}
	Bit8u newAmpIncrement;
	if (partialParam->tva.envTime[4] == 0) {
		newAmpIncrement = 1;
	} else {
		newAmpIncrement = -partialParam->tva.envTime[4];
	}
	// The next time nextPhase() is called, it will think TVA_PHASE_RELEASE has finished and the partial will be aborted
	startRamp(0, newAmpIncrement, TVA_PHASE_RELEASE);
}

void TVA::recalcSustain() {
	// We get pinged periodically by the pitch code to recalculate our values when in sustain.
	// This is done so that the TVA will respond to things like MIDI expression and volume changes while it's sustaining, which it otherwise wouldn't do.

	// The check for envLevel[3] == 0 strikes me as slightly dumb. FIXME: Explain why
	if (phase != TVA_PHASE_SUSTAIN || partialParam->tva.envLevel[3] == 0) {
		return;
	}
	// We're sustaining. Recalculate all the values
	Tables *tables = &partial->getSynth()->tables;
	int newAmpTarget = calcBasicAmp(tables, partial, system, partialParam, patchTemp, rhythmTemp, biasAmpSubtraction, veloAmpSubtraction, part->getExpression());
	newAmpTarget += partialParam->tva.envLevel[3];
	// Since we're in TVA_PHASE_SUSTAIN at this point, we know that la32AmpTarget has been reached and an interrupt fired, so we can rely on it being the current amp.
	int ampDelta = newAmpTarget - la32AmpTarget;

	// Calculate an increment to get to the new amp value in a short, more or less consistent amount of time
	Bit8u newAmpIncrement;
	if (ampDelta >= 0) {
		newAmpIncrement = tables->envLogarithmicTime[(Bit8u)ampDelta] - 2;
	} else {
		newAmpIncrement = (tables->envLogarithmicTime[(Bit8u)-ampDelta] - 2) | 0x80;
	}
	// Configure so that once the transition's complete and nextPhase() is called, we'll just re-enter sustain phase (or decay phase, depending on parameters at the time).
	startRamp(newAmpTarget, newAmpIncrement, TVA_PHASE_SUSTAIN - 1);
}

bool TVA::isPlaying() const {
	return playing;
}

int TVA::getPhase() const {
	return phase;
}

void TVA::nextPhase() {
	Tables *tables = &partial->getSynth()->tables;

	if (phase >= TVA_PHASE_DEAD || !playing) {
		partial->getSynth()->printDebug("TVA::nextPhase(): Shouldn't have got here with phase %d, playing=%s", phase, playing ? "true" : "false");
		return;
	}
	int newPhase = phase + 1;

	if (newPhase == TVA_PHASE_DEAD) {
		end(newPhase);
		return;
	}

	bool allLevelsZeroFromNowOn = false;
	if (partialParam->tva.envLevel[3] == 0) {
		if (newPhase == TVA_PHASE_4) {
			allLevelsZeroFromNowOn = true;
		} else if (partialParam->tva.envLevel[2] == 0) {
			if (newPhase == TVA_PHASE_3) {
				allLevelsZeroFromNowOn = true;
			} else if (partialParam->tva.envLevel[1] == 0) {
				if (newPhase == TVA_PHASE_2) {
					allLevelsZeroFromNowOn = true;
				} else if (partialParam->tva.envLevel[0] == 0) {
					if (newPhase == TVA_PHASE_ATTACK)  { // this line added, missing in ROM - FIXME: Add description of repercussions
						allLevelsZeroFromNowOn = true;
					}
				}
			}
		}
	}

	int newAmpTarget;
	int newAmpIncrement;
	int envPointIndex = phase;

	if (!allLevelsZeroFromNowOn) {
		newAmpTarget = calcBasicAmp(tables, partial, system, partialParam, patchTemp, rhythmTemp, biasAmpSubtraction, veloAmpSubtraction, part->getExpression());

		if (newPhase == TVA_PHASE_SUSTAIN || newPhase == TVA_PHASE_RELEASE) {
			if (partialParam->tva.envLevel[3] == 0) {
				end(newPhase);
				return;
			}
			if (!partial->getPoly()->canSustain()) {
				newPhase = TVA_PHASE_RELEASE;
				newAmpTarget = 0;
				newAmpIncrement = -partialParam->tva.envTime[4];
				if (newAmpIncrement == 0) {
					// We can't let the increment be 0, or there would be no emulated interrupt.
					// So we do an "upward" increment, which should set the amp to 0 extremely quickly
					// and cause an "interrupt" to bring us back to nextPhase().
					newAmpIncrement = 1;
				}
			} else {
				newAmpTarget += partialParam->tva.envLevel[3];
				newAmpIncrement = 0;
			}
		} else {
			newAmpTarget += partialParam->tva.envLevel[envPointIndex];
		}
	} else {
		newAmpTarget = 0;
	}

	if ((newPhase != TVA_PHASE_SUSTAIN && newPhase != TVA_PHASE_RELEASE) || allLevelsZeroFromNowOn) {
		int envTimeSetting = partialParam->tva.envTime[envPointIndex];

		if (newPhase == TVA_PHASE_ATTACK) {
			envTimeSetting -= ((signed)partial->getPoly()->getVelocity() - 64) >> (6 - partialParam->tva.envTimeVeloSensitivity); // PORTABILITY NOTE: Assumes arithmetic shift

			if (envTimeSetting <= 0 && partialParam->tva.envTime[envPointIndex] != 0) {
				envTimeSetting = 1;
			}
		} else {
			envTimeSetting -= keyTimeSubtraction;
		}
		if (envTimeSetting > 0) {
			int ampDelta = newAmpTarget - la32AmpTarget;
			if (ampDelta <= 0) {
				if (ampDelta == 0) {
					// la32AmpTarget and newAmpTarget are the same.
					// We can't have an increment of 0 or we wouldn't get an emulated interrupt.
					// So instead make the target one less than it really should be and set ampDelta accordingly.
					ampDelta = -1;
					newAmpTarget--;
					if (newAmpTarget < 0) {
						// Oops, newAmpTarget is less than zero now, so let's do it the other way:
						// Make newAmpTarget one more than it really should've been and set ampDelta accordingly.
						// FIXME (apparent bug in real firmware):
						// This means ampDelta will be positive just below here where it's inverted, and we'll end up using envLogarithmicTime[-1], and we'll be setting newAmpIncrement to be descending later on, etc..
						ampDelta = 1;
						newAmpTarget = -newAmpTarget;
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
		} else {
			newAmpIncrement = newAmpTarget >= la32AmpTarget ? (0x80 | 127) : 127;
		}

		// FIXME: What's the point of this? It's checked or set to non-zero everywhere above
		if (newAmpIncrement == 0) {
			newAmpIncrement = 1;
		}
	}

	startRamp((Bit8u)newAmpTarget, (Bit8u)newAmpIncrement, newPhase);
}

}
