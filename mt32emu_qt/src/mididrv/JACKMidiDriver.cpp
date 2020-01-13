/* Copyright (C) 2011-2019 Jerome Fisher, Sergey V. Mikayev
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

#include "JACKMidiDriver.h"

#include <QtCore>

#include "../MasterClock.h"
#include "../MidiSession.h"
#include "../JACKClient.h"

static inline quint32 midiBufferToShortMessage(size_t midiBufferSize, uchar *midiBuffer) {
	switch (midiBufferSize) {
	case 1:
		return *midiBuffer;
	case 2:
		return qFromLittleEndian<quint16>(midiBuffer);
	default:
		return qFromLittleEndian<quint32>(midiBuffer) & 0xFFFFFF;
	}
}

MasterClockNanos JACKMidiDriver::jackFrameTimeToMasterClockNanos(MasterClockNanos refNanos, quint64 eventJackTime, quint64 refJackTime) {
	return refNanos + (eventJackTime - refJackTime) * MasterClock::NANOS_PER_MICROSECOND;
}

bool JACKMidiDriver::pushMIDIMessage(MidiSession *midiSession, MasterClockNanos eventTimestamp, size_t midiBufferSize, uchar *midiBuffer) {
	SynthRoute *synthRoute = midiSession->getSynthRoute();
	if (*midiBuffer == 0xF0) {
		return synthRoute->pushMIDISysex(midiBuffer, midiBufferSize, eventTimestamp);
	}
	quint32 message = midiBufferToShortMessage(midiBufferSize, midiBuffer);
	return synthRoute->pushMIDIShortMessage(message, eventTimestamp);
}

bool JACKMidiDriver::playMIDIMessage(MidiSession *midiSession, quint64 eventTimestamp, size_t midiBufferSize, uchar *midiBuffer) {
	SynthRoute *synthRoute = midiSession->getSynthRoute();
	if (*midiBuffer == 0xF0) {
		return synthRoute->playMIDISysex(midiBuffer, midiBufferSize, eventTimestamp);
	}
	quint32 message = midiBufferToShortMessage(midiBufferSize, midiBuffer);
	return synthRoute->playMIDIShortMessage(message, eventTimestamp);
}

JACKMidiDriver::JACKMidiDriver(Master *master) : MidiDriver(master) {
	name = "JACK MIDI Driver";
	disconnect(SIGNAL(midiSessionInitiated(MidiSession **, MidiDriver *, QString)));
	connect(this, SIGNAL(midiSessionInitiated(MidiSession **, MidiDriver *, QString)), master, SLOT(createMidiSession(MidiSession **, MidiDriver *, QString)), Qt::DirectConnection);
}

JACKMidiDriver::~JACKMidiDriver() {
	stop();
}

void JACKMidiDriver::start() {
	JACKClient *jackClient = new JACKClient;
	MidiSession *midiSession = NULL;
	emit midiSessionInitiated(&midiSession, this, "JACK MIDI In");
	jackClient->open(midiSession, NULL);
	qDebug() << "JACK MIDI Driver started";
}

void JACKMidiDriver::stop() {
	qDebug() << "JACK MIDI Driver stopped";
}
