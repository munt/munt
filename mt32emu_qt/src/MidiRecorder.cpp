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

#include "MidiRecorder.h"

static const char headerID[] = "MThd\x00\x00\x00\x06";
static const char trackID[] = "MTrk";
static const MasterClockNanos DEFAULT_NANOS_PER_QUARTER_NOTE = 500000000;
static const uint MEMORY_RESERVE = 16384;

MidiRecorder::MidiRecorder() : startNanos(0), endNanos(0) {
}

void MidiRecorder::startRecording() {
	startNanos = MasterClock::getClockNanos();
	endNanos = -1;
	midiEventList.clear();
	midiEventList.reserve(MEMORY_RESERVE);
}

void MidiRecorder::recordShortMessage(quint32 msg, MasterClockNanos midiNanos) {
	if (isRecording()) {
		midiEventList.newMidiEvent().assignShortMessage(midiNanos, msg);
	}
}

void MidiRecorder::recordSysex(const uchar *sysexData, quint32 sysexLen, MasterClockNanos midiNanos) {
	if (isRecording()) {
		midiEventList.newMidiEvent().assignSysex(midiNanos, sysexData, sysexLen);
	}
}

void MidiRecorder::stopRecording() {
	endNanos = MasterClock::getClockNanos();
}

bool MidiRecorder::isRecording() {
	return endNanos == -1;
}

bool MidiRecorder::saveSMF(QString fileName, MasterClockNanos midiTick) {
	uint division = uint(DEFAULT_NANOS_PER_QUARTER_NOTE / midiTick);

	// Clamp division to fit to 16-bit signed integer
	if (division > 32767) {
		division = 32767;
		midiTick = DEFAULT_NANOS_PER_QUARTER_NOTE / division;
	}
	file.setFileName(fileName);
	forever {
		if (!file.open(QIODevice::WriteOnly)) break;
		if (!writeHeader(division)) break;
		if (!writeTrack(midiTick)) break;
		file.close();
		return true;
	}
	file.close();
	return false;
}

bool MidiRecorder::writeHeader(uint division) {
	if (!writeFile(headerID, 8)) return false;
	char header[6];
	qToBigEndian<quint16>(0, (uchar *)&header[0]); // format 0
	qToBigEndian<quint16>(1, (uchar *)&header[2]); // number of tracks 1
	qToBigEndian<qint16>(division, (uchar *)&header[4]); // division
	if (!writeFile(header, 6)) return false;
	return true;
}

bool MidiRecorder::writeTrack(MasterClockNanos midiTick) {

	// Writing track header, we'll fill length field later
	if (!writeFile(trackID, 4)) return false;
	quint64 trackLenPos = file.pos();
	quint32 trackLen = 0;
	if (!writeFile((char *)&trackLen, 4)) return false;

	// Writing actual MIDI events
	uint runningStatus = 0;
	quint32 eventTicks = 0; // Number of ticks from start of recording
	uchar eventData[16]; // Buffer for single short event / sysex header
	for (int i = 0; i < midiEventList.count(); i++) {
		// Compute timestamp
		const QMidiEvent &evt = midiEventList.at(i);
		uchar *data = eventData;
		quint32 deltaTicks = (evt.getTimestamp() - startNanos) / midiTick - eventTicks;
		eventTicks += deltaTicks;
		writeVarLenInt(data, deltaTicks);

		uchar *sysexData = evt.getSysexData();
		if (sysexData != NULL) {
			// Process Sysex
			quint32 sysexLen = evt.getSysexLen();
			if (sysexLen < 4 || sysexData[0] != 0xF0 || sysexData[sysexLen - 1] != 0xF7) {
				// Invalid sysex, skipping
				qDebug() << "MidiRecorder: wrong sysex skipped at:" << evt.getTimestamp() - startNanos << "nanos, length:" << sysexLen;
				eventTicks -= deltaTicks;
				continue;
			}
			*(data++) = sysexData[0];
			writeVarLenInt(data, sysexLen - 1);
			if (!writeFile((char *)eventData, data - eventData)) return false;
			if (!writeFile((char *)&sysexData[1], sysexLen - 1)) return false;
			runningStatus = 0;
		} else {
			// Process short message
			quint32 message = evt.getShortMessage();
			uint newStatus = message & 0xFF;
			if (0xF0 <= newStatus) {
				// No support for escaping System messages, ignore
				qDebug() << "MidiRecorder: unsupported System message skipped at:" << evt.getTimestamp() - startNanos << "nanos, code:" << newStatus;
				continue;
			}
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
			if (!writeFile((char *)eventData, data - eventData)) return false;
		}
	}

	// Writing end-of-track meta-event
	uchar *data = eventData;
	quint32 deltaTicks = (endNanos - startNanos) / midiTick - eventTicks;
	writeVarLenInt(data, deltaTicks);
	qToLittleEndian<quint32>(0x002FFF, data);
	data += 3;
	if (!writeFile((char *)eventData, data - eventData)) return false;

	// Writing track length
	quint64 trackEndPos = file.pos();
	trackLen = quint32(trackEndPos - trackLenPos - 4);
	qToBigEndian<quint32>(trackLen, eventData);
	file.seek(trackLenPos);
	if (!writeFile((char *)eventData, 4)) return false;
	qDebug() << "MidiRecorder: Processed" << midiEventList.count() << "MIDI events, written" << trackLen << "bytes";
	return true;
}

bool MidiRecorder::writeFile(const char *data, qint64 len) {
	qint64 writtenLen = file.write(data, len);
	if (writtenLen == len) return true;
	qDebug() << "MidiRecorder: Error writing file";
	return false;
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
