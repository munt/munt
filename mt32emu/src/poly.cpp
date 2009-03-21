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

	isPlaying = false;
	isDecay = false;
	pedalhold = false;

	for (int i = 0; i < 4; i++) {
		partials[i] = NULL;
	}
}

void Poly::reset(unsigned int key, unsigned int velocity, bool canSustain, Partial **partials) {
	if(isActive()) {
		// FIXME: Throw out some big ugly debug output with a lot of exclamation marks - we should never get here
		abort();
	}

	this->key = key;
	this->velocity = velocity;
	this->sustain = canSustain;

	isPlaying = false;
	isDecay = false;
	pedalhold = false;

	for (int i = 0; i < 4; i++) {
		this->partials[i] = partials[i];
		if (partials[i] != NULL) {
			isPlaying = true;
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

void Poly::noteOff(bool pedalHeld) {
	// Non-sustaining instruments ignore note off.
	// They die away eventually anyway.
	if (!isActive() || !sustain || !isPlaying) {
		return;
	}
	if (pedalHeld) {
		pedalhold = true;
		return;
	}
	startDecay();
}

void Poly::stopPedalHold() {
	if (!isActive() || !pedalhold)
		return;
	startDecay();
}

void Poly::startDecay() {
	if (isDecay) {
		return;
	}

	isDecay = true;
	isPlaying = false;
	pedalhold = false;

	for (int t = 0; t < 4; t++) {
		Partial *partial = partials[t];
		if (partial == NULL)
			continue;
		partial->startDecayAll();
	}
}

void Poly::abort() {
	if (!isActive()) {
		return;
	}
	for (int t = 0; t < 4; t++) {
		Partial *partial = partials[t];
		if (partial != NULL) {
			partial->deactivate();
		}
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

bool Poly::isActive() const {
	return partials[0] != NULL || partials[1] != NULL || partials[2] != NULL || partials[3] != NULL;
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
	for (int i = 0; i < 4; i++) {
		if (partials[i] == partial) {
			partials[i] = NULL;
		}
	}
}

}
