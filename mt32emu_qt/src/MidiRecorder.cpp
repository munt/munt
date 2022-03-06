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

#include "MidiRecorder.h"

#include "MidiEventLayout.h"
#include "QAtomicHelper.h"
#include "RealtimeLocker.h"

using MidiEventLayout::MidiEventHeader;

typedef MidiEventLayout::ShortMessageEntry<MasterClockNanos> ShortMessageEntry;
typedef MidiEventLayout::SysexMessageHeader<MasterClockNanos> SysexMessageHeader;

enum MidiRecorderStatus {
	MidiRecorderStatus_EMPTY = 0,
	MidiRecorderStatus_RECORDING = 1,
	MidiRecorderStatus_HAS_DATA_PENDING_WRITE = 2
};

static const char headerID[] = "MThd\x00\x00\x00\x06";
static const char trackID[] = "MTrk";
static const MasterClockNanos DEFAULT_NANOS_PER_QUARTER_NOTE = 500000000;

static const uint DATA_CHUNK_SIZE = 32768;
static const int DATA_CHUNK_ALLOCATION_PERIOD_MILLIS = 4000;
static const quint32 MESSAGE_DATA_ALIGNMENT = quint32(sizeof(MasterClockNanos));

// A chunk of byte data. Chunks can be bound into a semi-lock-free linked list.
class DataChunk {
private:
	uchar * const byteBuffer;
	QAtomicPointer<DataChunk> nextChunk;

public:
	DataChunk(uint byteBufferSize) : byteBuffer(new uchar[byteBufferSize]) {
		// Touch the newly allocated chuck, potentially to trigger a page fault early.
		volatile uchar *vBuffer = byteBuffer;
		vBuffer[0];
	}

	~DataChunk() {
		delete nextChunk.fetchAndStoreRelaxed(NULL);
		delete[] byteBuffer;
	}

	uchar *getByteBuffer() {
		return byteBuffer;
	}

	DataChunk *getNextChunk() {
		return QAtomicHelper::loadRelaxed(nextChunk);
	}

	DataChunk *setNextChunk(DataChunk *useNextChunk) {
		return nextChunk.fetchAndStoreRelease(useNextChunk);
	}
};

MidiRecorder::MidiRecorder() : startNanos(), endNanos(), allocationTimer(this) {
	allocationTimer.setInterval(DATA_CHUNK_ALLOCATION_PERIOD_MILLIS);
	connect(&allocationTimer, SIGNAL(timeout()), SLOT(handleAllocationTimer()));
}

MidiRecorder::~MidiRecorder() {
	reset();
}

void MidiRecorder::reset() {
	status.fetchAndStoreOrdered(MidiRecorderStatus_EMPTY);
	while (!midiTrackRecorders.isEmpty()) {
		delete midiTrackRecorders.takeLast();
	}
	allocationTimer.stop();
}

void MidiRecorder::startRecording() {
	startNanos = MasterClock::getClockNanos();
	if (!status.testAndSetOrdered(MidiRecorderStatus_EMPTY, MidiRecorderStatus_RECORDING)) {
		qWarning() << "MidiRecorder: Attempted to start recording while was in status" << int(status) << "-> resetting";
		reset();
		return;
	}
	allocationTimer.start();
}

bool MidiRecorder::stopRecording() {
	MidiRecorderStatus newStatus = midiTrackRecorders.isEmpty() ? MidiRecorderStatus_EMPTY : MidiRecorderStatus_HAS_DATA_PENDING_WRITE;
	if (!status.testAndSetOrdered(MidiRecorderStatus_RECORDING, newStatus)) {
		qWarning() << "MidiRecorder: Attempted to stop recording while was in status" << int(status) << "-> resetting";
		reset();
		return false;
	}

	// Wait for every track recorder to finish with the current event.
	for (int i = 0; i < midiTrackRecorders.size(); i++) {
		QMutexLocker locker(&midiTrackRecorders.at(i)->trackMutex);
	}

	endNanos = MasterClock::getClockNanos();
	allocationTimer.stop();
	return newStatus == MidiRecorderStatus_HAS_DATA_PENDING_WRITE;
}

bool MidiRecorder::isEmpty() const {
	return MidiRecorderStatus_EMPTY == QAtomicHelper::loadRelaxed(status);
}

bool MidiRecorder::isRecording() const {
	return MidiRecorderStatus_RECORDING == QAtomicHelper::loadRelaxed(status);
}

bool MidiRecorder::hasPendingData() const {
	return MidiRecorderStatus_HAS_DATA_PENDING_WRITE == QAtomicHelper::loadRelaxed(status);
}

MidiTrackRecorder *MidiRecorder::addTrack() {
	MidiTrackRecorder *midiTrackRecorder = new MidiTrackRecorder(*this);
	midiTrackRecorders << midiTrackRecorder;
	return midiTrackRecorder;
}

bool MidiRecorder::saveSMF(QString fileName, MasterClockNanos midiTick) {
	if (!hasPendingData()) {
		qWarning() << "MidiRecorder: Attempted to save SMF while was in status" << int(status) << "-> resetting";
		reset();
		return false;
	}
	if (fileName.isEmpty()) {
		qDebug() << "MidiRecorder: User refused to save recorded data -> resetting";
		reset();
		return true;
	}

	uint division = uint(DEFAULT_NANOS_PER_QUARTER_NOTE / midiTick);

	// Clamp division to fit to 16-bit signed integer
	if (division > 32767) {
		division = 32767;
		midiTick = DEFAULT_NANOS_PER_QUARTER_NOTE / division;
	}

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly)) return false;
	if (!writeHeader(file, midiTrackRecorders.size(), division)) return false;
	while (!midiTrackRecorders.isEmpty()) {
		MidiTrackRecorder *trackRecorder = midiTrackRecorders.takeFirst();
		bool result = writeTrack(file, trackRecorder, midiTick);
		delete trackRecorder;
		if (!result) {
			reset();
			return false;
		}
	}
	QAtomicHelper::storeRelease(status, MidiRecorderStatus_EMPTY);
	return true;
}

bool MidiRecorder::writeHeader(QFile &file, const int numberOfTracks, uint division) {
	if (!writeFile(file, headerID, 8)) return false;
	char header[6];
	qToBigEndian<quint16>(numberOfTracks > 1 ? 1 : 0, (uchar *)&header[0]); // format
	qToBigEndian<quint16>(numberOfTracks, (uchar *)&header[2]); // number of tracks
	qToBigEndian<qint16>(division, (uchar *)&header[4]); // division
	return writeFile(file, header, 6);
}

bool MidiRecorder::writeTrack(QFile &file, MidiTrackRecorder *midiTrackRecorder, const MasterClockNanos midiTick) {
	// Writing track header, we'll fill length field later
	if (!writeFile(file, trackID, 4)) return false;
	quint64 trackLenPos = file.pos();
	quint32 trackLen = 0;
	if (!writeFile(file, (char *)&trackLen, 4)) return false;

	// Writing actual MIDI events
	uint runningStatus = 0;
	quint32 eventsProcessed = 0;
	quint32 eventTicks = 0; // Number of ticks from start of track
	uchar eventData[16]; // Buffer for single short event / sysex header
	DataChunk *chunk = midiTrackRecorder->firstChunk;
	DataChunk *lastChunk = QAtomicHelper::loadRelaxed(midiTrackRecorder->currentChunk);
	quint32 readPosition = 0;
	while (chunk != lastChunk || readPosition < midiTrackRecorder->writePosition) {
		const uchar *readBuffer = chunk->getByteBuffer() + readPosition;
		const MidiEventHeader *eventHeader = reinterpret_cast<const MidiEventHeader *>(readBuffer);
		if (eventHeader->eventType == MidiEventLayout::MidiEventType_PAD) {
			readPosition = 0;
			chunk = chunk->getNextChunk();
			if (chunk == NULL) break;
			continue;
		}

		uchar *data = eventData;
		if (eventHeader->eventType == MidiEventLayout::MidiEventType_SYSEX_MESSAGE) {
			// Process SysEx message
			const SysexMessageHeader *sysexMessageHeader = reinterpret_cast<const SysexMessageHeader *>(readBuffer);
			quint32 sysexLength = sysexMessageHeader->sysexDataLength;
			readPosition += quint32(sizeof(SysexMessageHeader)) + MidiEventLayout::alignSysexDataLength(sysexLength, MESSAGE_DATA_ALIGNMENT);
			const uchar *sysexData = reinterpret_cast<const uchar *>(sysexMessageHeader + 1);
			if (sysexLength < 4 || sysexData[0] != 0xF0 || sysexData[sysexLength - 1] != 0xF7) {
				// Invalid sysex, skipping
				qDebug() << "MidiRecorder: wrong sysex skipped at:" << (sysexMessageHeader->timestamp - startNanos) * 1e-6 << "millis, length:" << sysexLength;
				continue;
			}
			writeMessageTimestamp(data, eventTicks, sysexMessageHeader->timestamp, midiTick);
			*(data++) = sysexData[0];
			writeVarLenInt(data, sysexLength - 1);
			if (!writeFile(file, (char *)eventData, data - eventData)) return false;
			if (!writeFile(file, (char *)&sysexData[1], sysexLength - 1)) return false;
			runningStatus = 0;
		} else {
			// Process short message
			readPosition += quint32(sizeof(ShortMessageEntry));
			const ShortMessageEntry *shortMessageEntry = reinterpret_cast<const ShortMessageEntry *>(readBuffer);
			quint32 message = shortMessageEntry->shortMessageData;
			uint newStatus = message & 0xFF;
			if (0xF0 <= newStatus) {
				// No support for escaping System messages, ignore
				qDebug() << "MidiRecorder: unsupported System message skipped at:" << (shortMessageEntry->timestamp - startNanos) * 1e-6 << "millis, code:" << newStatus;
				continue;
			}
			writeMessageTimestamp(data, eventTicks, shortMessageEntry->timestamp, midiTick);
			if (newStatus == runningStatus) {
				message >>= 8;
				if ((newStatus & 0xE0) == 0xC0) {
					// It's a short message with one data byte
					*(data++) = uchar(message);
				} else {
					// It's a short message with two data bytes
					qToLittleEndian<quint16>(message, data);
					data += 2;
				}
			} else {
				if ((newStatus & 0xE0) == 0xC0) {
					// It's a short message with one data byte
					qToLittleEndian<quint16>(message, data);
					data += 2;
				} else {
					// It's a short message with two data bytes
					qToLittleEndian<quint32>(message, data);
					data += 3;
				}
				runningStatus = newStatus;
			}
			if (!writeFile(file, (char *)eventData, data - eventData)) return false;
		}
		eventsProcessed++;
	}

	// Writing end-of-track meta-event
	uchar *data = eventData;
	writeMessageTimestamp(data, eventTicks, endNanos, midiTick);
	qToLittleEndian<quint32>(0x002FFF, data);
	data += 3;
	if (!writeFile(file, (char *)eventData, data - eventData)) return false;

	// Writing track length
	quint64 trackEndPos = file.pos();
	trackLen = quint32(trackEndPos - trackLenPos - 4);
	qToBigEndian<quint32>(trackLen, eventData);
	file.seek(trackLenPos);
	if (!writeFile(file, (char *)eventData, 4)) return false;
	file.seek(trackEndPos);
	qDebug() << "MidiRecorder: Processed" << eventsProcessed << "MIDI events, written" << trackLen << "bytes";
	return true;
}

bool MidiRecorder::writeFile(QFile &file, const char *data, qint64 len) {
	qint64 writtenLen = file.write(data, len);
	if (writtenLen == len) return true;
	qDebug() << "MidiRecorder: Error writing file";
	return false;
}

void MidiRecorder::writeMessageTimestamp(uchar * &data, quint32 &lastEventTicks, const MasterClockNanos timestamp, const MasterClockNanos midiTick) {
	quint32 thisEventTicks = qMax(quint32((timestamp - startNanos) / midiTick), lastEventTicks);
	quint32 deltaTicks = thisEventTicks - lastEventTicks;
	lastEventTicks = thisEventTicks;
	writeVarLenInt(data, deltaTicks);
}

void MidiRecorder::writeVarLenInt(uchar * &data, quint32 value) {
	// Since we have only 28 bit, clamp the value
	if (value > 0x0FFFFFFF) value = 0x0FFFFFFF;
	quint32 res = 0;
	for (int i = 0; i < 4; i++) {
		res <<= 8;
		res |= value & 0x7F;
		value >>= 7;
		if (i > 0) res |= 0x80;
	}
	bool somethingWritten = false;
	for (int i = 0; i < 4; i++) {
		if (somethingWritten || ((res & 0xFF) != 0x80)) {
			*(data++) = (uchar)res;
			somethingWritten = true;
		}
		res >>= 8;
	}
}

void MidiRecorder::handleAllocationTimer() {
	for (int i = 0; i < midiTrackRecorders.size(); i++) {
		MidiTrackRecorder *midiTrackRecorder = midiTrackRecorders.at(i);
		DataChunk *chunk = QAtomicHelper::loadRelaxed(midiTrackRecorder->currentChunk);
		if (chunk->getNextChunk() == NULL) {
			// This track needs allocation of a new chunk.
			chunk->setNextChunk(new DataChunk(DATA_CHUNK_SIZE));
		}
	}
}

MidiTrackRecorder::MidiTrackRecorder(MidiRecorder &useMidiRecorder) :
	midiRecorder(useMidiRecorder),
	firstChunk(new DataChunk(DATA_CHUNK_SIZE)),
	currentChunk(firstChunk),
	writePosition()
{}

MidiTrackRecorder::~MidiTrackRecorder() {
	delete firstChunk;
}

bool MidiTrackRecorder::recordShortMessage(quint32 shortMessageData, MasterClockNanos midiNanos) {
	RealtimeLocker trackLocker(trackMutex);
	if (!trackLocker.isLocked() || !midiRecorder.isRecording()) return false;
	quint32 entryLength = quint32(sizeof(ShortMessageEntry));
	uchar *byteBuffer = requireBufferSpace(entryLength);
	if (byteBuffer == NULL) return false;
	ShortMessageEntry *shortMessageEntry = reinterpret_cast<ShortMessageEntry *>(byteBuffer + writePosition);
	shortMessageEntry->eventType = MidiEventLayout::MidiEventType_SHORT_MESSAGE;
	shortMessageEntry->shortMessageData = shortMessageData;
	shortMessageEntry->timestamp = midiNanos;
	writePosition += entryLength;
	return true;
}

bool MidiTrackRecorder::recordSysex(const uchar *sysexData, quint32 sysexDataLength, MasterClockNanos midiNanos) {
	RealtimeLocker trackLocker(trackMutex);
	if (!trackLocker.isLocked() || !midiRecorder.isRecording()) return false;
	quint32 entryLength = quint32(sizeof(SysexMessageHeader)) + MidiEventLayout::alignSysexDataLength(sysexDataLength, MESSAGE_DATA_ALIGNMENT);
	uchar *byteBuffer = requireBufferSpace(entryLength);
	if (byteBuffer == NULL) return false;
	SysexMessageHeader *sysexMessageHeader = reinterpret_cast<SysexMessageHeader *>(byteBuffer + writePosition);
	sysexMessageHeader->eventType = MidiEventLayout::MidiEventType_SYSEX_MESSAGE;
	sysexMessageHeader->sysexDataLength = sysexDataLength;
	sysexMessageHeader->timestamp = midiNanos;
	memcpy(sysexMessageHeader + 1, sysexData, sysexDataLength);
	writePosition += entryLength;
	return true;
}

uchar *MidiTrackRecorder::requireBufferSpace(quint32 byteLength) {
	DataChunk *chunk = QAtomicHelper::loadRelaxed(currentChunk);
	if (writePosition + byteLength + sizeof(MidiEventHeader) <= DATA_CHUNK_SIZE) return chunk->getByteBuffer();
	if (byteLength + sizeof(MidiEventHeader) > DATA_CHUNK_SIZE) return NULL;

	DataChunk *nextChunk = chunk->getNextChunk();
	if (nextChunk == NULL) return NULL;

	reinterpret_cast<MidiEventHeader *>(chunk->getByteBuffer() + writePosition)->eventType = MidiEventLayout::MidiEventType_PAD;
	writePosition = 0;
	QAtomicHelper::storeRelease(currentChunk, nextChunk);
	return nextChunk->getByteBuffer();
}
