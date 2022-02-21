/* Copyright (C) 2011-2022 Jerome Fisher, Sergey V. Mikayev
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

#include "MidiEventLayout.h"

using MidiEventLayout::MidiEventHeader;

typedef MidiEventLayout::ShortMessageEntry<quint64> ShortMessageEntry;
typedef MidiEventLayout::SysexMessageHeader<quint64> SysexMessageHeader;

static const quint32 BUFFER_SIZE = 32768;
static const quint32 MESSAGE_DATA_ALIGNMENT = quint32(sizeof(quint32));

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
	ShortMessageEntry *shortMessageEntry = static_cast<ShortMessageEntry *>(writePointer);
	shortMessageEntry->eventType = MidiEventLayout::MidiEventType_SHORT_MESSAGE;
	shortMessageEntry->shortMessageData = data;
	shortMessageEntry->timestamp = timestamp;
	shortMessageEntry++;
	writePointer = shortMessageEntry;
	bytesWritten += entrySize;
	bytesToWrite -= entrySize;
	return true;
}

bool QMidiBuffer::pushSysexMessage(quint64 timestamp, quint32 dataLength, const uchar *data) {
	const quint32 alignedDataLength = MidiEventLayout::alignSysexDataLength(dataLength, MESSAGE_DATA_ALIGNMENT);
	const quint32 entrySize = quint32(sizeof(SysexMessageHeader)) + alignedDataLength;
	if (!requestSpace(entrySize)) return false;
	SysexMessageHeader *sysexMessageHeader = static_cast<SysexMessageHeader *>(writePointer);
	sysexMessageHeader->eventType = MidiEventLayout::MidiEventType_SYSEX_MESSAGE;
	sysexMessageHeader->sysexDataLength = dataLength;
	sysexMessageHeader->timestamp = timestamp;
	sysexMessageHeader++;
	uchar *sysexMessageData = reinterpret_cast<uchar *>(sysexMessageHeader);
	memcpy(sysexMessageData, data, dataLength);
	sysexMessageData += alignedDataLength;
	writePointer = sysexMessageData;
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

bool QMidiBuffer::retrieveEvents() {
	popEvents();
	readPointer = ringBuffer.readPointer(bytesToRead);
	if (bytesToRead == 0) return false;
	MidiEventHeader *eventHeader = static_cast<MidiEventHeader *>(readPointer);
	if (eventHeader->eventType != MidiEventLayout::MidiEventType_PAD) return true;
	bytesRead += bytesToRead;
	return retrieveEvents();
}

quint64 QMidiBuffer::getEventTimestamp() const {
	MidiEventHeader *eventHeader = static_cast<MidiEventHeader *>(readPointer);
	if (eventHeader->eventType == MidiEventLayout::MidiEventType_SYSEX_MESSAGE) {
		return static_cast<SysexMessageHeader *>(eventHeader)->timestamp;
	}
	return static_cast<ShortMessageEntry *>(eventHeader)->timestamp;
}

quint32 QMidiBuffer::getEventData(const uchar *&sysexData) const {
	MidiEventHeader *eventHeader = static_cast<MidiEventHeader *>(readPointer);
	if (eventHeader->eventType == MidiEventLayout::MidiEventType_SYSEX_MESSAGE) {
		SysexMessageHeader *sysexMessageHeader = static_cast<SysexMessageHeader *>(eventHeader);
		quint32 sysexDataLength = sysexMessageHeader->sysexDataLength;
		sysexMessageHeader++;
		sysexData = reinterpret_cast<uchar *>(sysexMessageHeader);
		return sysexDataLength;
	}
	sysexData = NULL;
	return static_cast<ShortMessageEntry *>(eventHeader)->shortMessageData;
}

bool QMidiBuffer::nextEvent() {
	MidiEventHeader *eventHeader = static_cast<MidiEventHeader *>(readPointer);
	quint32 entrySize = quint32(sizeof(ShortMessageEntry));
	if (eventHeader->eventType == MidiEventLayout::MidiEventType_SYSEX_MESSAGE) {
		SysexMessageHeader *sysexMessageHeader = static_cast<SysexMessageHeader *>(eventHeader);
		entrySize += MidiEventLayout::alignSysexDataLength(sysexMessageHeader->sysexDataLength, MESSAGE_DATA_ALIGNMENT);
	}
	readPointer = static_cast<uchar *>(readPointer) + entrySize;
	bytesRead += entrySize;
	bytesToRead -= entrySize;
	if (bytesToRead > 0) {
		if (eventHeader->eventType != MidiEventLayout::MidiEventType_PAD) return true;
		bytesRead += bytesToRead;
	}
	return retrieveEvents();
}

void QMidiBuffer::discardEvents() {
	bytesRead += bytesToRead;
	popEvents();
}

void QMidiBuffer::popEvents() {
	if (readPointer == NULL) return;
	ringBuffer.advanceReadPointer(bytesRead);
	readPointer = NULL;
	bytesRead = 0;
	bytesToRead = 0;
}

bool QMidiBuffer::requestSpace(quint32 eventLength) {
	if (writePointer == NULL) {
		writePointer = ringBuffer.writePointer(bytesToWrite, freeSpaceContiguous);
	}
	while (bytesToWrite < (eventLength + sizeof(MidiEventHeader))) {
		if (freeSpaceContiguous) return eventLength <= bytesToWrite;
		// When the free space isn't contiguous, pad the rest of the ring buffer
		// and restart from the beginning.
		MidiEventHeader *padHeader = static_cast<MidiEventHeader *>(writePointer);
		padHeader->eventType = MidiEventLayout::MidiEventType_PAD;
		bytesWritten += bytesToWrite;
		flush();
		writePointer = ringBuffer.writePointer(bytesToWrite, freeSpaceContiguous);
	}
	return true;
}
