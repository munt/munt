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

#include "MidiSession.h"

using namespace MT32Emu;

MidiSession::MidiSession(QObject *parent, MidiDriver *useMidiDriver, QString useName, SynthRoute *useSynthRoute) :
	QObject(parent), midiDriver(useMidiDriver), name(useName), synthRoute(useSynthRoute), qMidiStreamParser(NULL)
{}

MidiSession::~MidiSession() {
	if (qMidiStreamParser != NULL) delete qMidiStreamParser;
}

QMidiStreamParser *MidiSession::getQMidiStreamParser() {
	if (qMidiStreamParser == NULL) {
		qMidiStreamParser = new QMidiStreamParser(*synthRoute);
	}
	return qMidiStreamParser;
}

SynthRoute *MidiSession::getSynthRoute() {
	return synthRoute;
}

QString MidiSession::getName() {
	return name;
}

void MidiSession::setName(const QString &newName) {
	name = newName;
}

QMidiStreamParser::QMidiStreamParser(SynthRoute &useSynthRoute) : synthRoute(useSynthRoute) {}

void QMidiStreamParser::setTimestamp(MasterClockNanos newTimestamp) {
	timestamp = newTimestamp;
}

void QMidiStreamParser::handleShortMessage(const Bit32u message) {
	synthRoute.pushMIDIShortMessage(message, timestamp);
}

void QMidiStreamParser::handleSysex(const Bit8u stream[], const Bit32u length) {
	synthRoute.pushMIDISysex(stream, length, timestamp);
}

void QMidiStreamParser::handleSystemRealtimeMessage(const Bit8u realtime) {
	// Unsupported yet
	Q_UNUSED(realtime);
}

void QMidiStreamParser::printDebug(const char *debugMessage) {
	qDebug() << debugMessage;
}
