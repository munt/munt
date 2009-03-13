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

namespace MT32Emu {

struct Poly {
	bool isPlaying;

	unsigned int key;
	int freqnum;
	int vel;

	bool isDecay;

	const Bit32u *volumeptr;

	Partial *partials[4];

	bool pedalhold; // This marks keys that have been released on the keyboard, but are being held by the pedal
	bool sustain;

	bool isActive() const;
	void startDecay();
	Bit32u getAge() const;
};

}

#endif /* POLY_H_ */
