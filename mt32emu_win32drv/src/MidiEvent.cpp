/* Copyright (C) 2003, 2004, 2005 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

#include <memory.h>

#include "MidiEvent.h"

MidiEvent::MidiEvent() {
	sysexInfo = 0;
	nxtEvent = 0;
};

MidiEvent::~MidiEvent() {
	if (sysexInfo != 0) {
		delete[] sysexInfo;
	}
}

MidiEvent *MidiEvent::getNext() {
	return nxtEvent;
}

long long MidiEvent::getTime() {
	return timemarker;
}

MidiEventType MidiEvent::getType() {
	return type;
}

bool MidiEvent::isFinal() {
	return nxtEvent == 0;
}

void MidiEvent::chainEvent(MidiEvent *newEvent) {
	nxtEvent = newEvent;
}

void MidiEvent::assignSysex(const unsigned char *Sysex, long len, long long setTime) {
	sysexInfo = new unsigned char[len];
	memcpy(sysexInfo, Sysex, len);
	type = SysexData;
	sysexLen = len;
	timemarker = setTime;
}

void MidiEvent::assignMsg(long Msg, long long setTime) {
	type = ShortMsg;
	midiMsg = Msg;
	timemarker = setTime;
}
