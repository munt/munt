/* Copyright (c) 2003-2004 Various contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
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
