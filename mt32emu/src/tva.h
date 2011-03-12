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

namespace MT32Emu {

class Part;

// When entering nextPhase, targetPhase is immediately incremented, and the descriptions/names below represent
// their use after the increment.
enum {
	// When this is the target phase, level[0] is targeted within time[0], and velocity potentially affects time
	TVA_PHASE_ATTACK = 1,

	// When this is the target phase, level[1] is targeted within time[1]
	TVA_PHASE_2 = 2,

	// When this is the target phase, level[2] is targeted within time[2]
	TVA_PHASE_3 = 3,

	// When this is the target phase, level[3] is targeted within time[3]
	TVA_PHASE_4 = 4,

	// When this is the target phase, immediately goes to PHASE_RELEASE unless the poly is set to sustain.
	// Aborts the partial if level[3] is 0.
	// Otherwise level[3] is continued, no phase change will occur until some external influence (like pedal release)
	TVA_PHASE_SUSTAIN = 5,

	// 0 is targeted within time[4] (the time calculation is quite different from the other phases)
	TVA_PHASE_RELEASE = 6,

	// It's PHASE_DEAD, Jim.
	TVA_PHASE_DEAD = 7
};

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

	int targetPhase;
	Bit32u currentAmp;
	// AFAICT: Lower 7 bits indicate how quickly currentAmp should be changed, most significant bit indicates change direction (set=downward)
	Bit8u targetAmp;
	Bit8u ampIncrement;
	unsigned int largeAmpInc;
	void setAmpIncrement(Bit8u ampIncrement);
	void nextPhase();

public:
	// FIXME: This should probably have a getter
	bool play;

	TVA(const Partial *partial);
	void reset(const Part *part, const PatchCache *patchCache, const MemParams::RhythmTemp *rhythmTemp);
	float nextAmp();
	void recalcSustain();
	void startDecay();

	int getPhase() const;
};

}

#endif /* TVA_H_ */
