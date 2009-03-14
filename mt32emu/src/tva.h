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

#ifndef MT32EMU_TVA_H
#define MT32EMU_TVA_H

#include "mt32emu.h"

namespace MT32Emu {

class Part;

// FIXME:KG: This should only exist while we're experimenting to get the right value
extern const int TVA_TARGET_AMP_MULT;

class TVA {
private:
	const Partial * const partial;
	const MemParams::System * const system;

	const Part *part;
	const TimbreParam::PartialParam *partialParam;
	const MemParams::PatchTemp *patchTemp;
	const MemParams::RhythmTemp *rhythmTemp;

	int biasAmpSubtraction;
	int veloAmpSubtraction;
	int keyTimeSubtraction;

public:
	// FIXME: These should probably be private (only public for testing purposes atm)
	bool play;
	int targetPhase;
	Bit8u targetAmp;
	// AFAICT: Lower 7 bits indicate how quickly the amp level should be changed, most significant bit indicates change direction (set=downward)
	Bit8u ampIncrement;
	Bit32u currentAmp;

	TVA(const Partial *partial);
	void reset(const Part *part, const PatchCache *patchCache);
	Bit32u nextAmp();
	void nextPhase();
	void recalcSustain();
	void startDecay();
};

}

#endif /* TVA_H_ */
