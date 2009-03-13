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

void Poly::startDecay() {
	if (isDecay) {
		return;
	}
	isDecay = true;

	for (int t = 0; t < 4; t++) {
		Partial *partial = partials[t];
		if (partial == NULL)
			continue;
		partial->startDecayAll();
	}
	isPlaying = false;
}

}
