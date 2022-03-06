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

#include "MidiSession.h"
#include "QMidiBuffer.h"

using namespace MT32Emu;

MidiSession::MidiSession(QObject *parent, MidiDriver *useMidiDriver, QString useName, SynthRoute *useSynthRoute) :
	QObject(parent), midiDriver(useMidiDriver), name(useName), synthRoute(useSynthRoute),
	qMidiStreamParser(), qMidiBuffer(), midiTrackRecorder()
{}

MidiSession::~MidiSession() {
	delete qMidiStreamParser;
	delete qMidiBuffer;
}

QMidiStreamParser *MidiSession::getQMidiStreamParser() {
	if (qMidiStreamParser == NULL) {
		qMidiStreamParser = new QMidiStreamParser(*this);
	}
	return qMidiStreamParser;
}

QMidiBuffer *MidiSession::getQMidiBuffer() {
	if (qMidiBuffer == NULL) {
		qMidiBuffer = new QMidiBuffer;
	}
	return qMidiBuffer;
}

MidiTrackRecorder * MidiSession::getMidiTrackRecorder() {
	return midiTrackRecorder;
}

MidiTrackRecorder *MidiSession::setMidiTrackRecorder(MidiTrackRecorder *useMidiTrackRecorder) {
	MidiTrackRecorder *oldRecorder = midiTrackRecorder;
	midiTrackRecorder = useMidiTrackRecorder;
	return oldRecorder;
}

SynthRoute *MidiSession::getSynthRoute() const {
	return synthRoute;
}

QString MidiSession::getName() {
	return name;
}

void MidiSession::setName(const QString &newName) {
	name = newName;
}

QMidiStreamParser::QMidiStreamParser(MidiSession &useMidiSession) : midiSession(useMidiSession) {}

void QMidiStreamParser::setTimestamp(MasterClockNanos newTimestamp) {
	timestamp = newTimestamp;
}

void QMidiStreamParser::handleShortMessage(const Bit32u message) {
	midiSession.getSynthRoute()->pushMIDIShortMessage(midiSession, message, timestamp);
}

void QMidiStreamParser::handleSysex(const Bit8u stream[], const Bit32u length) {
	midiSession.getSynthRoute()->pushMIDISysex(midiSession, stream, length, timestamp);
}

void QMidiStreamParser::handleSystemRealtimeMessage(const Bit8u realtime) {
	// Unsupported yet
	Q_UNUSED(realtime);
}

void QMidiStreamParser::printDebug(const char *debugMessage) {
	qDebug() << debugMessage;
}
