/* Copyright (C) 2011-2020 Jerome Fisher, Sergey V. Mikayev
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

#include <cstring>

#include "QMidiBuffer.h"

enum MidiEventType {
	MidiEventType_SHORT_MESSAGE,
	MidiEventType_SYSEX_MESSAGE,
	MidiEventType_PAD
};

struct PadHeader {
	MidiEventType eventType;
};

struct ShortMessageEntry {
	MidiEventType eventType;
	quint32 shortMessageData;
	quint64 timestamp;
};

struct SysexMessageHeader {
	MidiEventType eventType;
	quint32 sysexDataLength;
	quint64 timestamp;
};

union MidiEventPointer {
	void *writePointer;
	void *readPointer;
	PadHeader *padHeader;
	ShortMessageEntry *shortMessageEntry;
	SysexMessageHeader *sysexMessageHeader;
	uchar *sysexMessageData;
};

static const quint32 BUFFER_SIZE = 32768;

// The allocated SysEx data length is rounded, so that the events are aligned to the quint32 boundary.
static inline size_t alignSysexDataLength(quint32 dataLength) {
	return ((dataLength + sizeof(quint32) - 1) / sizeof(quint32)) * sizeof(quint32);
}

QMidiBuffer::QMidiBuffer() :
	ringBuffer(BUFFER_SIZE),
	writePointer(),
	bytesWritten(),
	bytesToWrite(),
	readPointer(),
	bytesRead(),
	bytesToRead()
{}

bool QMidiBuffer::pushShortMessage(quint64 timestamp, quint32 data) {
	static const quint32 entrySize = sizeof(ShortMessageEntry);
	if (!requestSpace(entrySize)) return false;
	MidiEventPointer eventPointer;
	eventPointer.writePointer = writePointer;
	eventPointer.shortMessageEntry->eventType = MidiEventType_SHORT_MESSAGE;
	eventPointer.shortMessageEntry->shortMessageData = data;
	eventPointer.shortMessageEntry->timestamp = timestamp;
	eventPointer.shortMessageEntry++;
	writePointer = eventPointer.writePointer;
	bytesWritten += entrySize;
	bytesToWrite -= entrySize;
	return true;
}

bool QMidiBuffer::pushSysexMessage(quint64 timestamp, quint32 dataLength, const uchar *data) {
	const size_t alignedDataLength = alignSysexDataLength(dataLength);
	const quint32 entrySize = quint32(sizeof(SysexMessageHeader) + alignedDataLength);
	if (!requestSpace(entrySize)) return false;
	MidiEventPointer eventPointer;
	eventPointer.writePointer = writePointer;
	eventPointer.sysexMessageHeader->eventType = MidiEventType_SYSEX_MESSAGE;
	eventPointer.sysexMessageHeader->sysexDataLength = dataLength;
	eventPointer.sysexMessageHeader->timestamp = timestamp;
	eventPointer.sysexMessageHeader++;
	memcpy(eventPointer.sysexMessageData, data, dataLength);
	eventPointer.sysexMessageData += alignedDataLength;
	writePointer = eventPointer.writePointer;
	bytesWritten += entrySize;
	bytesToWrite -= entrySize;
	return true;
}

void QMidiBuffer::flush() {
	if (writePointer == NULL) return;
	ringBuffer.advanceWritePointer(bytesWritten);
	writePointer = NULL;
	bytesWritten = 0;
	bytesToWrite = 0;
}

bool QMidiBuffer::retieveEvents() {
	popEvents();
	readPointer = ringBuffer.readPointer(bytesToRead);
	if (bytesToRead == 0) return false;
	MidiEventPointer eventPointer;
	eventPointer.readPointer = readPointer;
	if (eventPointer.padHeader->eventType != MidiEventType_PAD) return true;
	bytesRead += bytesToRead;
	return retieveEvents();
}

quint64 QMidiBuffer::getEventTimestamp() const {
	MidiEventPointer eventPointer;
	eventPointer.readPointer = readPointer;
	if (eventPointer.sysexMessageHeader->eventType == MidiEventType_SYSEX_MESSAGE) {
		return eventPointer.sysexMessageHeader->timestamp;
	}
	return eventPointer.shortMessageEntry->timestamp;
}

quint32 QMidiBuffer::getEventData(const uchar *&sysexData) const {
	MidiEventPointer eventPointer;
	eventPointer.readPointer = readPointer;
	if (eventPointer.sysexMessageHeader->eventType == MidiEventType_SYSEX_MESSAGE) {
		quint32 sysexDataLength = eventPointer.sysexMessageHeader->sysexDataLength;
		eventPointer.sysexMessageHeader++;
		sysexData = eventPointer.sysexMessageData;
		return sysexDataLength;
	}
	sysexData = NULL;
	return eventPointer.shortMessageEntry->shortMessageData;
}

bool QMidiBuffer::nextEvent() {
	MidiEventPointer eventPointer;
	eventPointer.readPointer = readPointer;
	quint32 entrySize;
	if (eventPointer.sysexMessageHeader->eventType == MidiEventType_SYSEX_MESSAGE) {
		size_t alignedDataLength = alignSysexDataLength(eventPointer.sysexMessageHeader->sysexDataLength);
		eventPointer.sysexMessageHeader++;
		eventPointer.sysexMessageData += alignedDataLength;
		entrySize = quint32(sizeof(SysexMessageHeader) + alignedDataLength);
	} else {
		eventPointer.shortMessageEntry++;
		entrySize = sizeof(ShortMessageEntry);
	}
	readPointer = eventPointer.readPointer;
	bytesRead += entrySize;
	bytesToRead -= entrySize;
	if (bytesToRead > 0) {
		if (eventPointer.padHeader->eventType != MidiEventType_PAD) return true;
		bytesRead += bytesToRead;
	}
	return retieveEvents();
}

void QMidiBuffer::popEvents() {
	ringBuffer.advanceReadPointer(bytesRead);
	readPointer = NULL;
	bytesRead = 0;
	bytesToRead = 0;
}

bool QMidiBuffer::requestSpace(quint32 eventLength) {
	if (writePointer == NULL) {
		writePointer = ringBuffer.writePointer(bytesToWrite, freeSpaceContiguous);
	}
	while (bytesToWrite < (eventLength + sizeof(PadHeader))) {
		if (freeSpaceContiguous) return eventLength <= bytesToWrite;
		// When the free space isn't contiguous, pad the rest of the ring buffer
		// and restart from the beginning.
		MidiEventPointer eventPointer;
		eventPointer.writePointer = writePointer;
		eventPointer.padHeader->eventType = MidiEventType_PAD;
		bytesWritten += bytesToWrite;
		flush();
		writePointer = ringBuffer.writePointer(bytesToWrite, freeSpaceContiguous);
	}
	return true;
}
