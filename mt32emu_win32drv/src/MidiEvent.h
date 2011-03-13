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

#ifndef __MT32WIN32DRV_MIDIEVENT_H__
#define __MT32WIN32DRV_MIDIEVENT_H__

enum MidiEventType {
	ShortMsg,
	SysexData
};

class MidiEvent {
private:
	long long timemarker;
	MidiEventType type;
	MidiEvent *nxtEvent;

public:
	unsigned char *sysexInfo;
	long sysexLen;
	long midiMsg;

	MidiEvent();
	~MidiEvent();

	long long getTime(void);
	MidiEventType getType(void);
	MidiEvent *getNext(void);
	bool isFinal();
	void chainEvent(MidiEvent *newEvent);
	void assignSysex(const unsigned char *Sysex, long len, long long setTime);
	void assignMsg(long Msg, long long setTime);
};

#endif
