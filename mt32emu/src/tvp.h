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

#ifndef MT32EMU_TVP_H
#define MT32EMU_TVP_H

#include "mt32emu.h"

namespace MT32Emu {

class TVP {
private:
	const Partial * const partial;
	const MemParams::System * const system; // FIXME: Necessary?

	const Part *part;
	const TimbreParam::PartialParam *partialParam;
	const MemParams::PatchTemp *patchTemp;

	int counter;
	Bit32u timeElapsed;
	// FIXME: Names
	Bit32u basePitch;
	Bit32u targetPitchOffsetWithoutLFO;
	Bit32u currentPitchOffset; // FIXME: Mysterious


	// FIXME: Names
	// FIXME: Not initialised
	Bit16s pitchOffsetChangePerBigTick;
	int pitch_modulation_value;
	int targetPitchOffsetReachedBigTick;
	unsigned int shifts;
	Bit16u pitch;

	// FIXME: All these names
	void calcPitch();
	void calcPitchOffsetChangePerTick(int targetPitchOffset, Bit8u changeDuration);
	void targetPitchOffsetReached();
	void nextPhase();
	void process();
public:
	int phase;
	Bit8s lfoPitchOffset; // FIXME
	// In range -12 - 36
	Bit8s timeKeyfollowValue; // FIXME

	TVP(const Partial *partial);
	void reset(const Part *part, const PatchCache *patchCache);
	Bit16u nextPitch();
	void startDecay();
};

}

#endif
