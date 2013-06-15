/* Copyright (C) 2011, 2012, 2013 Jerome Fisher, Sergey V. Mikayev
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
#include "MidiSession.h"
#include "audiodrv/AudioDriver.h"

using namespace MT32Emu;

SynthRoute::SynthRoute(QObject *parent) :
	QObject(parent),
	state(SynthRouteState_CLOSED),
	qSynth(this),
	audioDevice(NULL),
	audioStream(NULL)
{
	connect(&qSynth, SIGNAL(stateChanged(SynthState)), SLOT(handleQSynthState(SynthState)));
}

SynthRoute::~SynthRoute() {
	delete audioStream;
}

void SynthRoute::setAudioDevice(const AudioDevice *newAudioDevice) {
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
			audioStream = audioDevice->startAudioStream(&qSynth, SAMPLE_RATE);
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

bool SynthRoute::reset() {
	if (state == SynthRouteState_CLOSED)
		return true;
	setState(SynthRouteState_CLOSING);
	if (qSynth.reset()) {
		setState(SynthRouteState_OPEN);
		return true;
	}
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

void SynthRoute::setMidiSessionName(MidiSession *midiSession, QString name) {
	midiSession->setName(name);
	emit midiSessionNameChanged(midiSession);
}

bool SynthRoute::hasMIDISessions() const {
	return !midiSessions.isEmpty();
}

void SynthRoute::handleQSynthState(SynthState synthState) {
	// Should really only stopAudio on CLOSED, and startAudio() on OPEN after init or CLOSED.
	// For CLOSING, it should suspend, and for OPEN after CLOSING, it should resume
	//audioOutput->suspend();
	//audioOutput->resume();

	switch (synthState) {
	case SynthState_OPEN:
		if (audioStream != NULL) {
			setState(SynthRouteState_OPEN);
		}
		break;
	case SynthState_CLOSING:
		//audioStream->suspend();
		setState(SynthRouteState_CLOSING);
		break;
	case SynthState_CLOSED:
		delete audioStream;
		audioStream = NULL;
		setState(SynthRouteState_CLOSED);
		break;
	}
}

bool SynthRoute::pushMIDIShortMessage(Bit32u msg, qint64 refNanos) {
	recorder.recordShortMessage(msg, refNanos);
	return qSynth.pushMIDIShortMessage(msg, refNanos);
}

bool SynthRoute::pushMIDISysex(Bit8u *sysexData, unsigned int sysexLen, qint64 refNanos) {
	recorder.recordSysex(sysexData, sysexLen, refNanos);
	return qSynth.pushMIDISysex(sysexData, sysexLen, refNanos);
}

void SynthRoute::setMasterVolume(int masterVolume) {
	qSynth.setMasterVolume(masterVolume);
}

void SynthRoute::setOutputGain(float outputGain) {
	qSynth.setOutputGain(outputGain);
}

void SynthRoute::setReverbOutputGain(float reverbOutputGain) {
	qSynth.setReverbOutputGain(reverbOutputGain);
}

void SynthRoute::setReverbEnabled(bool reverbEnabled) {
	qSynth.setReverbEnabled(reverbEnabled);
}

void SynthRoute::setReverbOverridden(bool reverbOverridden) {
	qSynth.setReverbOverridden(reverbOverridden);
}

void SynthRoute::setReverbSettings(int reverbMode, int reverbTime, int reverbLevel) {
	qSynth.setReverbSettings(reverbMode, reverbTime, reverbLevel);
}

void SynthRoute::setDACInputMode(DACInputMode emuDACInputMode) {
	qSynth.setDACInputMode(emuDACInputMode);
}

MidiRecorder *SynthRoute::getMidiRecorder() {
	return &recorder;
}

void SynthRoute::getSynthProfile(SynthProfile &synthProfile) const {
	qSynth.getSynthProfile(synthProfile);
}

void SynthRoute::setSynthProfile(const SynthProfile &synthProfile, QString useSynthProfileName) {
	qSynth.setSynthProfile(synthProfile, useSynthProfileName);
}
