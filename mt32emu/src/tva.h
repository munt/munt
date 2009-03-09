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

class TVA {
private:
	const Partial * const partial;
	const MemParams::System * const system;

	const Part *part;
	const TimbreParam::PartialParam *partialParam;
	const MemParams::PatchTemp *patchTemp;
	const MemParams::RhythmTemp *rhythmTemp;

	bool play;

	int biasAmpSubtraction;
	int veloAmpSubtraction;
	int keyTimeSubtraction;

public:
	// FIXME: These should probably be private
	int targetPhase;
	int targetAmp;
	int timeToTarget;

	TVA(const Partial *partial);
	void reset(const Part *part, const PatchCache *patchCache);
	void nextPhase();
};

}

#endif /* TVA_H_ */
