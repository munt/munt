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

#ifndef MT32EMU_PART_H
#define MT32EMU_PART_H

namespace MT32Emu {

class PartialManager;
class Synth;

class Part {
private:
	// Pointers to the areas of the MT-32's memory dedicated to this part (for parts 1-8)
	MemParams::PatchTemp *patchTemp;
	TimbreParam *timbreTemp;

	// 0=Part 1, .. 7=Part 8, 8=Rhythm
	unsigned int partNum;

	bool holdpedal;

	StereoVolume volumesetting;

	PatchCache patchCache[4];

	float bend; // -1.0 .. +1.0

	Poly polyTable[MT32EMU_MAX_POLY];

	void abortPoly(Poly *poly);

	static int fixKeyfollow(int srckey);
	static int fixBiaslevel(int srcpnt, int *dir);

	void setPatch(const PatchParam *patch);

protected:
	Synth *synth;
	char name[8]; // "Part 1".."Part 8", "Rhythm"
	char currentInstr[11];
	int expression;
	Bit32u volumeMult;

	void updateVolume();
	void backupCacheToPartials(PatchCache cache[4]);
	void cacheTimbre(PatchCache cache[4], const TimbreParam *timbre);
	void playPoly(const PatchCache cache[4], unsigned int key, int freqNum, int vel);
	const char *getName() const;

public:
	Part(Synth *synth, unsigned int usePartNum);
	virtual void playNote(unsigned int key, int vel);
	void stopNote(unsigned int key);
	void allNotesOff();
	void allSoundOff();
	int getVolume() const;
	void setVolume(int midiVolume);
	void setExpression(int midiExpression);
	virtual void setPan(unsigned int midiPan);
	virtual void setBend(unsigned int midiBend);
	virtual void setModulation(unsigned int midiModulation);
	virtual void setProgram(unsigned int midiProgram);
	void setHoldPedal(bool pedalval);
	void stopPedalHold();
	virtual void refresh();
	virtual void refreshTimbre(unsigned int absTimbreNum);
	virtual void setTimbre(TimbreParam *timbre);
	virtual unsigned int getAbsTimbreNum() const;
	const char *getCurrentInstr() const;
};

class RhythmPart: public Part {
	// Pointer to the area of the MT-32's memory dedicated to rhythm
	const MemParams::RhythmTemp *rhythmTemp;

	// This caches the timbres/settings in use by the rhythm part
	PatchCache drumCache[85][4];
	StereoVolume drumPan[85];
public:
	RhythmPart(Synth *synth, unsigned int usePartNum);
	void refresh();
	void refreshTimbre(unsigned int timbreNum);
	void setTimbre(TimbreParam *timbre);
	void playNote(unsigned int key, int vel);
	unsigned int getAbsTimbreNum() const;
	void setPan(unsigned int midiPan);
	void setBend(unsigned int midiBend);
	void setModulation(unsigned int midiModulation);
	void setProgram(unsigned int patchNum);
};

}
#endif
