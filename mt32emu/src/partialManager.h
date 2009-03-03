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

#ifndef MT32EMU_PARTIALMANAGER_H
#define MT32EMU_PARTIALMANAGER_H

namespace MT32Emu {

class Synth;

class PartialManager {
private:
	Synth *synth; // Only used for sending debug output

	Partial *partialTable[MT32EMU_MAX_PARTIALS];
	Bit32s partialReserveTable[MT32EMU_MAX_PARTIALS];
	Bit32s partialPart[9]; // The count of partials played per part

public:

	PartialManager(Synth *synth);
	~PartialManager();
	Partial *allocPartial(int partNum);
	unsigned int getFreePartialCount(void);
	bool freePartials(unsigned int needed, int partNum);
	unsigned int setReserve(Bit8u *rset);
	void deactivateAll();
	void ageAll();
	bool produceOutput(int i, Bit16s *buffer, Bit32u bufferLength);
	bool shouldReverb(int i);
	void clearAlreadyOutputed();
	void getPerPartPartialUsage(int usage[9]);
	const Partial *getPartial(unsigned int partialNum) const;
};

}

#endif
