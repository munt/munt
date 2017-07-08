/* Copyright (C) 2011-2017 Jerome Fisher, Sergey V. Mikayev
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

#include "QMidiEvent.h"

#include <cstring>

using namespace MT32Emu;

QMidiEvent::QMidiEvent() {
	type = SHORT_MESSAGE;
	msg = 0;
	sysexLen = 0;
	sysexData = NULL;
}

QMidiEvent::QMidiEvent(const QMidiEvent &copyOf) {
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
}

QMidiEvent::~QMidiEvent() {
	if (sysexData != 0) {
		delete[] sysexData;
	}
}

SynthTimestamp QMidiEvent::getTimestamp() const {
	return timestamp;
}

MidiEventType QMidiEvent::getType() const {
	return type;
}

unsigned char *QMidiEvent::getSysexData() const {
	return sysexData;
}

Bit32u QMidiEvent::getShortMessage() const {
	return msg;
}

Bit32u QMidiEvent::getSysexLen() const {
	return sysexLen;
}

void QMidiEvent::setTimestamp(SynthTimestamp newTimestamp) {
	timestamp = newTimestamp;
}

void QMidiEvent::assignShortMessage(SynthTimestamp newTimestamp, Bit32u newMsg) {
	timestamp = newTimestamp;
	type = SHORT_MESSAGE;
	msg = newMsg;
	sysexLen = 0;
	delete[] sysexData;
	sysexData = NULL;
}

void QMidiEvent::assignSysex(SynthTimestamp newTimestamp, unsigned char const * const newSysexData, Bit32u newSysexLen) {
	timestamp = newTimestamp;
	type = SYSEX;
	msg = 0;
	sysexLen = newSysexLen;
	delete[] sysexData;
	Bit8u *copy = new Bit8u[newSysexLen];
	memcpy(copy, newSysexData, newSysexLen);
	sysexData = copy;
}

void QMidiEvent::assignSetTempoMessage(SynthTimestamp newTimestamp, MT32Emu::Bit32u newTempo) {
	timestamp = newTimestamp;
	type = SET_TEMPO;
	msg = newTempo;
	sysexLen = 0;
	delete[] sysexData;
	sysexData = NULL;
}

void QMidiEvent::assignSyncMessage(SynthTimestamp newTimestamp) {
	timestamp = newTimestamp;
	type = SYNC;
	msg = 0;
	sysexLen = 0;
	delete[] sysexData;
	sysexData = NULL;
}

QMidiEvent &QMidiEventList::newMidiEvent() {
	uint s = size();
	resize(s + 1);
	return last();
}
