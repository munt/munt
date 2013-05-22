/* Copyright (C) 2011, 2012, 2013 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MidiEvent.h"

#include <cstring>

using namespace MT32Emu;

MidiEvent::MidiEvent() {
	type = SHORT_MESSAGE;
	msg = 0;
	sysexLen = 0;
	sysexData = NULL;
};

MidiEvent::MidiEvent(const MidiEvent &copyOf) {
	timestamp = copyOf.timestamp;
	type = copyOf.type;
	msg = copyOf.msg;
	if (type == SYSEX && copyOf.sysexData != NULL) {
		sysexLen = copyOf.sysexLen;
		sysexData = new uchar[sysexLen];
		memcpy(sysexData, copyOf.sysexData, sysexLen);
	} else {
		sysexLen = 0;
		sysexData = NULL;
	}
};

MidiEvent::~MidiEvent() {
	if (sysexData != 0) {
		delete[] sysexData;
	}
}

SynthTimestamp MidiEvent::getTimestamp() const {
	return timestamp;
}

MidiEventType MidiEvent::getType() const {
	return type;
}

unsigned char *MidiEvent::getSysexData() const {
	return sysexData;
}

Bit32u MidiEvent::getShortMessage() const {
	return msg;
}

Bit32u MidiEvent::getSysexLen() const {
	return sysexLen;
}

void MidiEvent::setTimestamp(SynthTimestamp newTimestamp) {
	timestamp = newTimestamp;
}

void MidiEvent::assignShortMessage(SynthTimestamp newTimestamp, Bit32u newMsg) {
	timestamp = newTimestamp;
	type = SHORT_MESSAGE;
	msg = newMsg;
	sysexLen = 0;
	delete[] sysexData;
	sysexData = NULL;
}

void MidiEvent::assignSysex(SynthTimestamp newTimestamp, unsigned char const * const newSysexData, Bit32u newSysexLen) {
	timestamp = newTimestamp;
	type = SYSEX;
	msg = 0;
	sysexLen = newSysexLen;
	delete[] sysexData;
	Bit8u *copy = new Bit8u[newSysexLen];
	memcpy(copy, newSysexData, newSysexLen);
	sysexData = copy;
}

void MidiEvent::assignSetTempoMessage(SynthTimestamp newTimestamp, MT32Emu::Bit32u newTempo) {
	timestamp = newTimestamp;
	type = SET_TEMPO;
	msg = newTempo;
	sysexLen = 0;
	delete[] sysexData;
	sysexData = NULL;
}

void MidiEvent::assignSyncMessage(SynthTimestamp newTimestamp) {
	timestamp = newTimestamp;
	type = SYNC;
	msg = 0;
	sysexLen = 0;
	delete[] sysexData;
	sysexData = NULL;
}

MidiEvent &MidiEventList::newMidiEvent() {
	resize(size() + 1);
	return last();
}
