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

Poly::Poly() {
	key = 255;
	velocity = 255;
	sustain = false;
	for (int i = 0; i < 4; i++) {
		partials[i] = NULL;
	}
	state = POLY_Inactive;
}

void Poly::reset(unsigned int key, unsigned int velocity, bool canSustain, Partial **partials) {
	if(isActive()) {
		// FIXME: Throw out some big ugly debug output with a lot of exclamation marks - we should never get here
		abort();
	}

	this->key = key;
	this->velocity = velocity;
	this->sustain = canSustain;

	for (int i = 0; i < 4; i++) {
		this->partials[i] = partials[i];
		if (partials[i] != NULL) {
			state = POLY_Playing;
		}
	}
}

// DEPRECATED: This will die when the new pitch stuff lands
void Poly::setBend(float bend) {
	for (int i = 0; i < 4; i++) {
		if (partials[i] != NULL) {
			partials[i]->setBend(bend);
		}
	}
}

bool Poly::noteOff(bool pedalHeld) {
	// Generally, non-sustaining instruments ignore note off. They die away eventually anyway.
	// Key 0 (only used by special cases on rhythm part) reacts to note off even if non-sustaining or pedal held.
	if (state == POLY_Inactive || state == POLY_Releasing || (key != 0 && !sustain)) {
		return false;
	}
	if (pedalHeld && key != 0) {
		state = POLY_Held;
	} else {
		startDecay();
	}
	return true;
}

void Poly::stopPedalHold() {
	if (state != POLY_Held)
		return;
	startDecay();
}

void Poly::startDecay() {
	if (state == POLY_Inactive || state == POLY_Releasing) {
		return;
	}
	state = POLY_Releasing;

	for (int t = 0; t < 4; t++) {
		Partial *partial = partials[t];
		if (partial != NULL) {
			partial->startDecayAll();
		}
	}
}

void Poly::abort() {
	if (state == POLY_Inactive) {
		return;
	}
	for (int t = 0; t < 4; t++) {
		Partial *partial = partials[t];
		if (partial != NULL) {
			partial->deactivate();
		}
	}
	if (state != POLY_Inactive) {
		// FIXME: Throw out lots of debug output - this should never happen
		// (Deactivating the partials above should've made them each call partialDeactivated(), ultimately changing the state to POLY_Inactive)
		state = POLY_Inactive;
	}
}

void Poly::backupCacheToPartials(PatchCache cache[4]) {
	for (int partialNum = 0; partialNum < 4; partialNum++) {
		Partial *partial = partials[partialNum];
		if (partial != NULL && partial->patchCache == &cache[partialNum]) {
			partial->cachebackup = cache[partialNum];
			partial->patchCache = &partial->cachebackup;
		}
	}
}

unsigned int Poly::getKey() const {
	return key;
}

unsigned int Poly::getVelocity() const {
	return velocity;
}

bool Poly::canSustain() const {
	return sustain;
}

PolyState Poly::getState() const {
	return state;
}

bool Poly::isActive() const {
	return state != POLY_Inactive;
}

Bit32u Poly::getAge() const {
	for (int i = 0; i < 4; i++) {
		if (partials[i] != NULL) {
			return partials[i]->age;
		}
	}
	return 0;
}

// This is called by Partial to inform the poly that the Partial has deactivated
void Poly::partialDeactivated(Partial *partial) {
	bool allNull = true;
	for (int i = 0; i < 4; i++) {
		if (partials[i] == partial) {
			partials[i] = NULL;
		} else if (partials[i] != NULL) {
			allNull = false;
		}
	}
	if (allNull) {
		state = POLY_Inactive;
	}
}

}
