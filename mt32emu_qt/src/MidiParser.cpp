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

#include "MidiParser.h"
#include "MasterClock.h"

static const char headerID[] = "MThd\x00\x00\x00\x06";
static const char trackID[] = "MTrk";

bool MidiParser::readFile(char *data, qint64 len) {
	qint64 readLen = file.read(data, len);
	if (readLen == len) return true;
	qDebug() << "MidiParser: Error reading file";
	return false;
}

bool MidiParser::parseHeader() {
	char header[8];
	if (!readFile(header, 8)) return false;
	if ((uchar)header[0] == 0xF0) {
		format = 0xF0;
		numberOfTracks = 1;
		division = 500;
		return true;
	}
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

bool MidiParser::parseTrack(QMidiEventList &midiEventList) {
	char header[8];
	forever {
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
	if (!readFile(trackData, trackLen)) {
		delete[] trackData;
		return false;
	}

	// Reserve memory for MIDI events, approx. 3 bytes per event
	midiEventList.reserve(trackLen / 3);
	qDebug() << "MidiParser: Memory reservation" << trackLen / 3;

	// Parsing actual MIDI events
	unsigned int runningStatus = 0;
	const uchar *data = (uchar *)trackData;
	QVarLengthArray<MT32Emu::Bit8u, MT32Emu::SYSEX_BUFFER_SIZE> sysexBuffer;
	while (data < (uchar *)trackData + trackLen) {
		SynthTimestamp time = parseVarLenInt(data);
		quint32 message = 0;
		const uchar status = *data;
		if (status & 0x80) {
			// It's normal status byte
			if (0xF0 <= status) {
				// It's a System event
				if (status == 0xF0) {
					// It's a SysEx event
					runningStatus = 0; // SysEx clears running status
					sysexBuffer.clear();
					sysexBuffer.append(status);
					quint32 sysexLength = parseVarLenInt(++data);
					if (sysexLength < 1) {
						// No SysEx data, keep the time in sync
						midiEventList.newMidiEvent().assignSyncMessage(time);
						continue;
					}
					if (MT32Emu::SYSEX_BUFFER_SIZE <= sysexLength) {
						qDebug() << "MidiParser: Warning: too long sysex encountered, it may cause problems with real hardware. Sysex length:" << sysexLength + 1;
					}
					sysexBuffer.append(data, sysexLength);
					data += sysexLength - 1;
					if (*(data++) == 0xF7) {
						// Complete SysEx event
						midiEventList.newMidiEvent().assignSysex(time, sysexBuffer.constData(), sysexBuffer.size());
						sysexBuffer.clear();
					} else {
						// SysEx fragment, just keep the time in sync
						midiEventList.newMidiEvent().assignSyncMessage(time);
					}
					continue;
				} else if (status == 0xF7) {
					// It's either a SysEx Continuation event or an escaped System event
					quint32 len = parseVarLenInt(++data);
					if (sysexBuffer.isEmpty() || len < 1) {
						qDebug() << "MidiParser: escaped System event, unsupported";
						data += len;
					} else {
						sysexBuffer.append(data, len);
						data += len - 1;
						if (*(data++) == 0xF7) {
							// Last SysEx fragment
							midiEventList.newMidiEvent().assignSysex(time, sysexBuffer.constData(), sysexBuffer.size());
							sysexBuffer.clear();
						} else {
							// SysEx is still incomplete, just keep the time in sync
							midiEventList.newMidiEvent().assignSyncMessage(time);
						}
						continue;
					}
				} else if (status == 0xFF) {
					// It's a Meta-event
					runningStatus = 0; // Meta-event clears running status
					uint metaType = *(++data);
					quint32 len = parseVarLenInt(++data);
					if (metaType == 0x2F) {
						qDebug() << "MidiParser: End-of-track Meta-event";
						if (time > 0) {
							// Assign a special marker event to end the track in time
							qDebug() << "MidiParser: Adding sync event for" << time << "divisions";
							midiEventList.newMidiEvent().assignSyncMessage(time);
						}
						runningStatus = 0x2F;
						break;
					} else if (metaType == 0x51) {
						uint newTempo = qFromBigEndian<quint32>(data) >> 8;
						midiEventList.newMidiEvent().assignSetTempoMessage(time, newTempo);
						qDebug() << "MidiParser: Meta-event: Set tempo:" << newTempo;
						data += len;
						continue;
					} else {
						qDebug() << "MidiParser: Meta-event code" << metaType << "unsupported";
					}
					data += len;
				} else {
					qDebug() << "MidiParser: Unsupported event" << status;
					data++;
				}
				if (time > 0) {
					// The event is unsupported. Nevertheless, assign a special marker event to retain timing information
					qDebug() << "MidiParser: Adding sync event for" << time << "divisions";
					midiEventList.newMidiEvent().assignSyncMessage(time);
				}
				continue;
			} else if ((status & 0xE0) == 0xC0) {
				// It's a short message with one data byte
				message = qFromLittleEndian<quint16>(data);
				data += 2;
			} else {
				// It's a short message with two data bytes
				message = qFromLittleEndian<quint32>(data) & 0xFFFFFF;
				data += 3;
			}
			runningStatus = status;
		} else {
			// Handle running status
			if ((runningStatus & 0x80) == 0) {
				qDebug() << "MidiParser: First MIDI event must have status byte";
				data++;
				continue;
			}
			if ((runningStatus & 0xE0) == 0xC0) {
				// It's a short message with one data byte
				message = runningStatus | ((quint32)*data << 8);
				data++;
			} else {
				// It's a short message with two data bytes
				message = runningStatus | ((quint32)qFromLittleEndian<quint16>(data) << 8);
				data += 2;
			}
		}
		midiEventList.newMidiEvent().assignShortMessage(time, message);
	}
	if (runningStatus != 0x2F) {
		qDebug() << "MidiParser: End-of-track Meta-event isn't the last event, file is probably corrupted.";
	}
	delete[] trackData;
	qDebug() << "MidiParser: Parsed" << midiEventList.count() << "MIDI events";
	return true;
}

quint32 MidiParser::parseVarLenInt(const uchar * &data) {
	quint32 value = 0;
	for (int i = 0; i < 3; i++) {
		value = (value << 7) | (*data & 0x7F);
		if ((*data & 0x80) == 0) break;
		data++;
	}
	if (*(data++) & 0x80) qDebug() << "MidiParser: Variable length entity must be no more than 4 bytes long";
	return value;
}

void MidiParser::mergeMidiEventLists(QVector<QMidiEventList> &trackList) {
	int totalEventCount = 0;

	// Remove empty tracks & allocate memory exactly needed
	for (int i = trackList.count() - 1; i >=0; i--) {
		int eventCount = trackList.at(i).count();
		if (eventCount == 0) {
			trackList.remove(i);
		} else {
			totalEventCount += eventCount;
		}
	}
	midiEventList.reserve(totalEventCount);
	qDebug() << "MidiParser: Expected" << totalEventCount << "events";

	// Append events from all the tracks to the output list in sequence
	QVarLengthArray<int> currentIx(trackList.count());	            // The index of the event to be added next
	QVarLengthArray<SynthTimestamp> currentTime(trackList.count()); // The time in MIDI ticks of the event to be added next
	for (int i = 0; i < trackList.count(); i++) {
		currentIx[i] = 0;
		currentTime[i] = trackList.at(i).at(0).getTimestamp();
	}
	SynthTimestamp lastEventTime = 0; // Timestamp of the last added event
	forever {
		int trackIx = -1;
		SynthTimestamp nextEventTime = 0x10000000;

		// Find lowest track index with earliest event
		for (int i = 0; i < trackList.count(); i++) {
			if (trackList.at(i).count() <= currentIx[i]) continue;
			if (currentTime[i] < nextEventTime) {
				nextEventTime = currentTime[i];
				trackIx = i;
			}
		}
		if (trackIx == -1) break;
		const QMidiEvent *e = &trackList.at(trackIx).at(currentIx[trackIx]);
		forever {
			midiEventList.append(*e);
			midiEventList.last().setTimestamp(nextEventTime - lastEventTime);
			lastEventTime = nextEventTime;
			if (trackList.at(trackIx).count() <= ++currentIx[trackIx]) break;
			e = &trackList.at(trackIx).at(currentIx[trackIx]);
			SynthTimestamp nextDeltaTime = e->getTimestamp();
			if (nextDeltaTime != 0) {
				currentTime[trackIx] += nextDeltaTime;
				break;
			}
		}
	}
	qDebug() << "MidiParser: Actually" << midiEventList.count() << "events";
}

bool MidiParser::parseSysex() {
	qint64 fileSize = file.size();
	file.seek(0);
	char *fileData = new char[fileSize];
	if (!readFile(fileData, fileSize)) {
		delete[] fileData;
		return false;
	}
	int sysexBeginIx = -1;
	uchar *data = (uchar *)fileData;
	for (int i = 0; i < fileSize; i++) {
		if (data[i] == 0xF0) {
			sysexBeginIx = i;
		}
		if (sysexBeginIx != -1 && data[i] == 0xF7) {
			int sysexLen = i - sysexBeginIx + 1;
			midiEventList.newMidiEvent().assignSysex(1, &data[sysexBeginIx], sysexLen);
			sysexBeginIx = -1;
		}
	}
	qDebug() << "MidiParser: Loaded sysex events:" << midiEventList.count();
	delete[] fileData;
	return true;
}

bool MidiParser::doParse() {
	if (!parseHeader()) return false;
	if (format == 0xF0) return parseSysex();
	qDebug() << "MidiParser: MIDI file format" << format;
	switch(format) {
		case 0:
			if (numberOfTracks != 1) {
				qDebug() << "MidiParser: MIDI file format error: MIDI files format 0 must have 1 MIDI track, not" << numberOfTracks;
				return false;
			}
			return parseTrack(midiEventList);
		case 1:
			if (numberOfTracks > 0) {
				QVector<QMidiEventList> trackList(numberOfTracks);
				for (uint i = 0; i < numberOfTracks; i++) {
					qDebug() << "MidiParser: Parsing & merging MIDI track" << i + 1;
					if (!parseTrack(trackList[i])) return false;
				}
				mergeMidiEventLists(trackList);
				return true;
			}
			qDebug() << "MidiParser: MIDI file format error: MIDI files format 1 must have at least 1 MIDI track";
			return false;
		case 2:
			for (uint i = 0; i < numberOfTracks; i++) {
				qDebug() << "MidiParser: Parsing & appending MIDI track" << i + 1;
				QMidiEventList list;
				if (!parseTrack(list)) return false;
				midiEventList += list;
			}
			return true;
		default:
			qDebug() << "MidiParser: MIDI file format error: unknown MIDI file format" << format;
			return false;
	}
}

bool MidiParser::parse(const QString fileName) {
	midiEventList.clear();
	file.setFileName(fileName);
	file.open(QIODevice::ReadOnly);
	bool parseResult = doParse();
	file.close();
	return parseResult;
}

const QMidiEventList &MidiParser::getMIDIEvents() {
	return midiEventList;
}

SynthTimestamp MidiParser::getMidiTick(uint tempo) {
	if (division & 0x8000) {
		// SMPTE timebase
		uint framesPerSecond = -(division >> 8);
		uint subframesPerFrame = division & 0xFF;
		return MasterClock::NANOS_PER_SECOND / (framesPerSecond * subframesPerFrame);
	} else {
		// PPQN
		return tempo * MasterClock::NANOS_PER_MICROSECOND / division;
	}
}

void MidiParser::addChannelsReset() {
	for (quint8 i = 0; i < 16; i++) {
		// All notes off
		quint32 msg = 0x7FB0 | i;
		midiEventList.newMidiEvent().assignShortMessage(0, msg);

		// Reset all controllers
		msg = 0x79B0 | i;
		midiEventList.newMidiEvent().assignShortMessage(0, msg);
	}
}
