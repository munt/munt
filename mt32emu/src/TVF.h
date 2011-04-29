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

#ifndef MT32EMU_TVF_H
#define MT32EMU_TVF_H

namespace MT32Emu {

class TVF {
private:
	const Partial * const partial;
	const TimbreParam::PartialParam *partialParam;

	Bit8u baseCutoff;
	int keyTimeSubtraction;
	unsigned int levelMult;

	unsigned int targetPhase;
	Bit32u current;
	Bit8u target;
	Bit8u increment;
	unsigned int bigIncrement;
	void setIncrement(Bit8u ampIncrement);
	void nextPhase();

public:
	TVF(const Partial *partial);
	void reset(const TimbreParam::PartialParam *partialParam, Bit32u basePitch);
	// Returns the base cutoff (without envelope modification).
	// The base cutoff is calculated when reset() is called and remains static
	// for the lifetime of the partial.
	// Barring bugs, the number returned is confirmed accurate
	// (based on specs from Mok).
	Bit8u getBaseCutoff() const;
	// This function needs to be called at each sample.
	// It will return a number between 0.0f and 255.0f.
	// Exactly how it should be applied to the cutoff is currently unknown.
	// *Possibly* it needs to be multiplied with the cutoff in some manner,
	// but it may just need to be added.
	float nextCutoffModifier();
	void startDecay();
};

}

#endif
