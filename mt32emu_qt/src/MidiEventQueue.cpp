/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
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

/**
 * Simple ringbuffer for MidiEvents.
 */

#include "MidiEventQueue.h"

using namespace MT32Emu;

MidiEventQueue::MidiEventQueue() {
	startPos = 0;
	endPos = 0;
}

bool MidiEventQueue::pushEvent(SynthTimestamp timestamp, Bit32u msg, Bit8u *sysexData, unsigned int sysexLen) {
	unsigned int newEndPos = endPos;
	bool result = true;
	mutex.lock();
	newEndPos++;
	// check for buffer wrap
	newEndPos %= maxPos;
	// check for buffer full
	if (startPos == newEndPos) {
		result = false;
	} else {
		if (sysexData != NULL) {
			ringBuffer[endPos].assignSysex(timestamp, sysexData, sysexLen);
		} else {
			ringBuffer[endPos].assignShortMessage(timestamp, msg);
		}
		endPos = newEndPos;
	}
	mutex.unlock();
	return result;
}

const MidiEvent *MidiEventQueue::popEvent() {
	MidiEvent *event = NULL;
	mutex.lock();
	// check for buffer empty
	if (startPos != endPos) {
		event = &ringBuffer[startPos];
		startPos++;
		// check for buffer wrap
		startPos %= maxPos;
	}
	mutex.unlock();
	return event;
}

const MidiEvent *MidiEventQueue::peekEvent() {
	const MidiEvent *event = NULL;
	mutex.lock();
	// check for buffer empty
	if (startPos != endPos) {
		event = &ringBuffer[startPos];
	}
	mutex.unlock();
	return event;
}

const MidiEvent *MidiEventQueue::peekEventAt(unsigned int pos) {
	const MidiEvent *event = NULL;
	mutex.lock();
	// check for buffer empty
	if (startPos != endPos) {
		unsigned int validLen = (endPos < startPos) ? (endPos + maxPos - startPos) : (endPos - startPos);
		// check for pos outside of valid range
		if (pos <= validLen - 1) {
			unsigned int peekPos = (startPos + pos) % maxPos;
			event = &ringBuffer[peekPos];
		}
	}
	mutex.unlock();
	return event;
}
