/* Copyright (C) 2011, 2012, 2013 Sergey V. Mikayev
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

#include "stdafx.h"

#ifndef MT32EMU_MIDISYNTH_H
#define MT32EMU_MIDISYNTH_H

namespace MT32Emu {

class MidiSynth {
private:
	unsigned int sampleRate;
	unsigned int midiLatency;
	unsigned int bufferSize;
	unsigned int chunkSize;
	bool useRingBuffer;
	bool resetEnabled;

	DACInputMode emuDACInputMode;
	float outputGain;
	float reverbOutputGain;
	bool reverbEnabled;
	bool reverbOverridden;
	Bit8u reverbMode;
	Bit8u reverbTime;
	Bit8u reverbLevel;

	Bit16s *buffer;
	DWORD renderedBufferCount;
	DWORD framesRendered;

	Synth *synth;
	const ROMImage *controlROM;
	const ROMImage *pcmROM;

	unsigned int MillisToFrames(unsigned int millis);
	void LoadSettings();
	void ReloadSettings();
	void ApplySettings();

	MidiSynth();

public:
	static MidiSynth &getInstance();
	int Init();
	void Close();
	int Reset();
	void RenderAvailableSpace();
	void Render(Bit16s *bufpos, DWORD totalFrames);
	Bit32u getMIDIEventTimestamp();
	void PlayMIDI(DWORD msg);
	void PlaySysex(const Bit8u *bufpos, DWORD len);
};

}
#endif
