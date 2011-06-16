/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SynthRoute is responsible for:
 * - Managing the audio output
 *  - Initial setup
 *  - Sample rate changes
 *  - Pausing/unpausing when the QSynth becomes unavailable.
 * - Maintaining a list of MIDI input drivers (and in future outputs) for the synth
 * - Synchronising timestamps between (potentially multiple) MIDI inputs and a single audio output
 *  - Giving Synth the correct monotonically increasing timestamp for MIDI messages
 *  - Giving Synth the correct timestamp for the first sample in render() calls
 */

#include "SynthRoute.h"

#ifdef WIN32
	#define USE_WINMM_AUDIO_DRIVER
	#include "audiodrv/WinMMAudioDriver.h"
#endif

#include "audiodrv/PortAudioDriver.h"

using namespace MT32Emu;

// In samples per second.
static const int SAMPLE_RATE = 32000;

// This value defines when the difference between our current idea of the offset between MIDI timestamps
// and audio timestamps is so bad that we just start using the new offset immediately.
// This can be regularly hit around stream start time by badly-behaved drivers that return bogus timestamps
// for a while when first starting.
static const qint64 EMERGENCY_RESYNC_THRESHOLD_NANOS = 500000000;

SynthRoute::SynthRoute(QObject *parent) : QObject(parent), state(SynthRouteState_CLOSED), qSynth(this), audioDeviceIndex(0), refNanosOffsetValid(false), refNanosOffset(0), audioDriver(NULL) {
	sampleRate = SAMPLE_RATE;

	connect(&qSynth, SIGNAL(stateChanged(SynthState)), SLOT(handleQSynthState(SynthState)));
}

SynthRoute::~SynthRoute() {
	delete audioDriver;
}

void SynthRoute::setAudioDeviceIndex(int newAudioDeviceIndex) {
	audioDeviceIndex = newAudioDeviceIndex;
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
	if (audioDriver == NULL) {
#ifdef USE_WINMM_AUDIO_DRIVER
		audioDriver = new WinMMAudioDriver(&qSynth, sampleRate);
#else
		audioDriver = new PortAudioDriver(&qSynth, sampleRate);
#endif
	}
	setState(SynthRouteState_OPENING);
	if (qSynth.open()) {
		if (audioDriver->start(audioDeviceIndex)) {
			setState(SynthRouteState_OPEN);
			return true;
		}
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
	audioDriver->close();
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
		//audioDriver->suspend();
		break;
	case SynthState_CLOSED:
		audioDriver->close();
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
