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

#ifndef MT32EMU_POLY_H
#define MT32EMU_POLY_H

enum PolyState {
	POLY_Playing,
	POLY_Held, // This marks keys that have been released on the keyboard, but are being held by the pedal
	POLY_Releasing,
	POLY_Inactive
};

namespace MT32Emu {

class Poly {
private:
	unsigned int key;
	unsigned int velocity;
	bool sustain;

	bool pedalhold;

	Partial *partials[4];

public:

	bool isPlaying;
	bool isDecay;

	Poly();
	void reset(unsigned int key, unsigned int velocity, bool sustain, Partial **partials);
	// DEPRECATED: setBend() will die when the new pitch stuff lands
	void setBend(float bend);
	void noteOff(bool pedalHeld);
	void stopPedalHold();
	void startDecay();
	void abort();

	void backupCacheToPartials(PatchCache cache[4]);

	unsigned int getKey() const;
	unsigned int getVelocity() const;
	bool canSustain() const;
	PolyState getState() const;
	bool isActive() const;
	Bit32u getAge() const;

	void partialDeactivated(Partial *partial);
};

}

#endif /* POLY_H_ */
