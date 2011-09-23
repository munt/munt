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
 * SynthRoute is responsible for:
 * - Managing the audio output
 *  - Initial setup
 *  - Sample rate changes
 *  - Pausing/unpausing when the QSynth becomes unavailable.
 * - Maintaining a list of MIDI sessions for the synth
 */

#include "SynthRoute.h"
#include "audiodrv/AudioDriver.h"

using namespace MT32Emu;

// In samples per second.
static const int SAMPLE_RATE = 32000;

SynthRoute::SynthRoute(QObject *parent) : QObject(parent), state(SynthRouteState_CLOSED), qSynth(this), audioDevice(NULL), audioStream(NULL) {
	sampleRate = SAMPLE_RATE;

	connect(&qSynth, SIGNAL(stateChanged(SynthState)), SLOT(handleQSynthState(SynthState)));
}

SynthRoute::~SynthRoute() {
	delete audioStream;
}

void SynthRoute::setAudioDevice(AudioDevice *newAudioDevice) {
	audioDevice = newAudioDevice;
	close();
}

void SynthRoute::setState(SynthRouteState newState) {
	if (state == newState) {
		return;
	}
	state = newState;
	emit stateChanged(newState);
}

bool SynthRoute::open() {
	switch (state) {
	case SynthRouteState_OPENING:
	case SynthRouteState_OPEN:
		return true;
	case SynthRouteState_CLOSING:
		return false;
	default:
		break;
	}
	setState(SynthRouteState_OPENING);
	if (audioDevice != NULL) {
		if (qSynth.open()) {
			audioStream = audioDevice->startAudioStream(&qSynth, sampleRate);
			if (audioStream != NULL) {
				setState(SynthRouteState_OPEN);
				return true;
			} else {
				qDebug() << "Failed to start audioStream";
			}
		} else {
			qDebug() << "Failed to open qSynth";
		}
	} else {
		qDebug() << "No audioDevice set";
	}
	setState(SynthRouteState_CLOSED);
	return false;
}

bool SynthRoute::close() {
	switch (state) {
	case SynthRouteState_CLOSING:
	case SynthRouteState_CLOSED:
		return true;
	case SynthRouteState_OPENING:
		return false;
	default:
		break;
	}
	setState(SynthRouteState_CLOSING);
	delete audioStream;
	audioStream = NULL;
	qSynth.close();
	return true;
}

bool SynthRoute::isPinned() const {
	// FIXME: Implement
	return false;
}

SynthRouteState SynthRoute::getState() const {
	return state;
}

void SynthRoute::addMidiSession(MidiSession *midiSession) {
	midiSessions.append(midiSession);
	emit midiSessionAdded(midiSession);
}

void SynthRoute::removeMidiSession(MidiSession *midiSession) {
	midiSessions.removeOne(midiSession);
	emit midiSessionRemoved(midiSession);
}

void SynthRoute::handleQSynthState(SynthState synthState) {
	// Should really only stopAudio on CLOSED, and startAudio() on OPEN after init or CLOSED.
	// For CLOSING, it should suspend, and for OPEN after CLOSING, it should resume
	//audioOutput->suspend();
	//audioOutput->resume();

	switch (synthState) {
	case SynthState_OPEN:
		break;
	case SynthState_CLOSING:
		//audioStream->suspend();
		break;
	case SynthState_CLOSED:
		delete audioStream;
		audioStream = NULL;
		setState(SynthRouteState_CLOSED);
		break;
	}
}

bool SynthRoute::pushMIDIShortMessage(Bit32u msg, qint64 refNanos) {
	return qSynth.pushMIDIShortMessage(msg, refNanos);
}

bool SynthRoute::pushMIDISysex(Bit8u *sysexData, unsigned int sysexLen, qint64 refNanos) {
	return qSynth.pushMIDISysex(sysexData, sysexLen, refNanos);
}
