/* Copyright (C) 2003, 2004, 2005 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
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

#ifndef MIDI_EVENT_H
#define MIDI_EVENT_H

#include <QtGlobal>

#include <mt32emu/mt32emu.h>

enum MidiEventType {
	SHORT_MESSAGE,
	SYSEX
};

typedef qint64 SynthTimestamp;

class MidiEvent {
private:
	SynthTimestamp timestamp;
	MidiEventType type;
	MT32Emu::Bit32u msg;
	MT32Emu::Bit32u sysexLen;
	unsigned char *sysexData;

public:
	MidiEvent();
	~MidiEvent();

	SynthTimestamp getTimestamp() const;
	MidiEventType getType() const;
	MT32Emu::Bit32u getShortMessage() const;
	MT32Emu::Bit32u getSysexLen() const;
	unsigned char *getSysexData() const;

	void assignShortMessage(SynthTimestamp newTimestamp, MT32Emu::Bit32u newMsg);
	void assignSysex(SynthTimestamp newTimestamp, unsigned char *newSysexData, MT32Emu::Bit32u newSysexLen);
};

#endif
