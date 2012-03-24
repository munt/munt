/* Copyright (C) 2011, 2012 Jerome Fisher, Sergey V. Mikayev
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

#include "MidiParser.h"

static const char headerID[] = "MThd\x00\x00\x00\x06";
static const char trackID[] = "MTrk";
static const uint MAX_SYSEX_LENGTH = 1024;

MidiParser::MidiParser(QString fileName) : file(fileName) {
	file.open(QIODevice::ReadOnly);
}

MidiParser::~MidiParser() {
}

bool MidiParser::readFile(char *data, qint64 len) {
	qint64 readLen = file.read(data, len);
	if (readLen == len) return true;
	qDebug() << "MidiParser: Error reading file";
	return false;
}

bool MidiParser::parseHeader() {
	char header[8];
	if (!readFile(header, 8)) return false;
	if (memcmp(header, headerID, 8) != 0) {
		qDebug() << "MidiParser: Wrong MIDI header";
		return false;
	}
	if (!readFile(header, 6)) return false;
	format = qFromBigEndian<quint16>((uchar *)&header[0]);
	numberOfTracks = qFromBigEndian<quint16>((uchar *)&header[2]);
	division = qFromBigEndian<qint16>((uchar *)&header[4]);
	return true;
}

bool MidiParser::parseTrack() {
	char header[8];
	for(;;) {
		if (!readFile(header, 8)) return false;
		if (memcmp(header, trackID, 4) == 0) {
			break;
		} else {
			qDebug() << "MidiParser: Wrong MIDI track signature, skipping unknown data chunk";
			quint32 dataLen = qFromBigEndian<quint32>((uchar *)&header[4]);
			if (file.seek(file.pos() + dataLen)) {
				qDebug() << "MidiParser: Error in data chunk";
				return false;
			}
		}
	}
	quint32 trackLen = qFromBigEndian<quint32>((uchar *)&header[4]);
	char *trackData = new char[trackLen];
	if (!readFile(trackData, trackLen)) return false;

	// Parsing actual MIDI events
	unsigned int runningStatus = 0;
	uchar *data = (uchar *)trackData;
	SynthTimestamp time = 0;
	uchar sysexBuffer[MAX_SYSEX_LENGTH];
	while(data < (uchar *)trackData + trackLen) {
		MidiEvent midiEvent;
		time = parseVarLenInt(data);
		quint32 message = 0;
		if (*data & 0x80) {
			// It's normal status byte
			if ((*data & 0xF0) == 0xF0) {
				// It's a special event
				if (*data == 0xF0) {
					// It's a sysex event
					sysexBuffer[0] = *data;
					data++;
					quint32 sysexLength = parseVarLenInt(data);
					if (MAX_SYSEX_LENGTH <= sysexLength) {
						qDebug() << "MidiParser: too long sysex encountered:" << sysexLength;
					}
					for (uint i = 1; i <= sysexLength; i++) {
						sysexBuffer[i] = *(data++);
					}
					midiEvent.assignSysex(time, data, sysexLength);
					midiEventList.append(midiEvent);
				} else if (*data == 0xF7) {
					qDebug() << "MidiParser: Fragmented sysex, unsupported";
					data++;
					quint32 len = parseVarLenInt(data);
					data += len;
				} else if (*data == 0xFF) {
					data++;
					if (*data == 0x2F) {
						qDebug() << "MidiParser: End-of-track Meta-event";
						return true;
					}
					qDebug() << "MidiParser: Meta-event, unsupported";
					data++;
					quint32 len = parseVarLenInt(data);
					data += len;
				} else {
					qDebug() << "MidiParser: Unsupported event";
					data++;
				}
				continue;
			} else if ((*data & 0xE0) == 0xC0) {
				// It's a short message with one data byte
				message = qFromLittleEndian<quint16>(data);
				data += 2;
			} else {
				// It's a short message with two data bytes
				message = qFromLittleEndian<quint32>(data) & 0xFFFFFF;
				data += 3;
			}
			runningStatus = message & 0xFF;
		} else {
			// Handle running status
			if ((runningStatus & 0x80) == 0) {
				qDebug() << "MidiParser: First MIDI event must has status byte";
				data++;
				continue;
			}
			if ((*data & 0xE0) == 0xC0) {
				// It's a short message with one data byte
				message = runningStatus | ((quint32)*data << 8);
				data++;
			} else {
				// It's a short message with two data bytes
				message = runningStatus | ((quint32)qFromLittleEndian<quint16>(data) << 8);
				data += 2;
			}
		}
		midiEvent.assignShortMessage(time, message);
		midiEventList.append(midiEvent);
	}
	qDebug() << "MidiParser: End-of-file discovered before End-of-track Meta-event";
	return true;
}

quint32 MidiParser::parseVarLenInt(uchar * &data) {
	quint32 value = 0;
	for (int i = 0; i < 3; i++) {
		value = (value << 7) | (*data & 0x7F);
		if ((*data & 0x80) == 0) break;
		data++;
	}
	if (*(data++) & 0x80) qDebug() << "MidiParser: Variable length entity must be no more than 4 bytes long";
	return value;
}

bool MidiParser::parse() {
	if (!parseHeader()) return false;
	if (!parseTrack()) return false;
	return true;
}

int MidiParser::getDivision() {
	return division;
}

QList<MidiEvent> MidiParser::getMIDIEventList() {
	return midiEventList;
}
