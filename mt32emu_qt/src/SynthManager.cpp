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
 * SynthManager is responsible for:
 * - Managing the audio output
 *  - Initial setup
 *  - Sample rate changes
 *  - Pausing/unpausing when the QSynth becomes unavailable.
 * - Maintaining a list of MIDI input drivers (and in future outputs) for the synth
 * - Synchronising timestamps between (potentially multiple) MIDI inputs and a single audio output
 *  - Giving Synth the correct monotonically increasing timestamp for MIDI messages
 *  - Giving Synth the correct timestamp for the first sample in render() calls
 */

#include "SynthManager.h"

#include "audiodrv/PortAudioDriver.h"

#include "mididrv/TestDriver.h"
#ifdef WITH_WIN32_MIDI_DRIVER
#include "mididrv/Win32Driver.h"
#endif

using namespace MT32Emu;

// In samples per second.
static const int SAMPLE_RATE = 32000;

static const qint64 NANOS_PER_SECOND = 1000000000;

// This value defines when the difference between our current idea of the offset between MIDI timestamps
// and audio timestamps is so bad that we just start using the new offset immediately.
// This can be regularly hit around stream start time by badly-behaved drivers that return bogus timestamps
// for a while when first starting.
static const qint64 EMERGENCY_RESYNC_THRESHOLD_NANOS = 500000000;

SynthManager::SynthManager() : state(SynthManagerState_CLOSED), midiDriver(NULL), audioDeviceIndex(0), midiNanosOffsetValid(false), midiNanosOffset(0) {
	sampleRate = SAMPLE_RATE;

	audioDriver = new PortAudioDriver(&qSynth, sampleRate);
	connect(&qSynth, SIGNAL(stateChanged(SynthState)), SLOT(handleQSynthState(SynthState)));
}

SynthManager::~SynthManager() {
	delete midiDriver;
	delete audioDriver;
}

void SynthManager::setAudioDeviceIndex(int newAudioDeviceIndex) {
	audioDeviceIndex = newAudioDeviceIndex;
}

void SynthManager::setState(SynthManagerState newState) {
	if (state == newState) {
		return;
	}
	state = newState;
	emit stateChanged(newState);
}

bool SynthManager::open() {
	switch (state) {
	case SynthManagerState_OPENING:
	case SynthManagerState_OPEN:
		return true;
	case SynthManagerState_CLOSING:
		return false;
	default:
		break;
	}

	setState(SynthManagerState_OPENING);
	if (qSynth.open()) {
		if (audioDriver->start(audioDeviceIndex)) {
			startMIDI();
			setState(SynthManagerState_OPEN);
			return true;
		}
	}
	setState(SynthManagerState_CLOSED);
	return false;
}

bool SynthManager::close() {
	switch (state) {
	case SynthManagerState_CLOSING:
	case SynthManagerState_CLOSED:
		return true;
	case SynthManagerState_OPENING:
		return false;
	default:
		break;
	}
	setState(SynthManagerState_CLOSING);
	stopMIDI();
	audioDriver->close();
	qSynth.close();
	return true;
}

void SynthManager::handleQSynthState(SynthState synthState) {
	// Should really only stopAudio on CLOSED, and startAudio() on OPEN after init or CLOSED.
	// For CLOSING, it should suspend, and for OPEN after CLOSING, it should resume
	//audioOutput->suspend();
	//audioOutput->resume();

	switch (synthState) {
	case SynthState_OPEN:
		break;
	case SynthState_CLOSING:
	case SynthState_CLOSED:
		audioDriver->close();
		stopMIDI();
		setState(SynthManagerState_CLOSED);
		break;
	}
}

SynthTimestamp SynthManager::midiNanosToAudioNanos(qint64 midiNanos) {
	if (midiNanos == 0) {
		// Special value meaning "no timestamp, play immediately"
		return 0;
	}
	if (!midiNanosOffsetValid) {
		// FIXME: Basically assumes that the first MIDI event received is intended to play immediately.
		SynthTimestamp currentAudioNanos = audioDriver->getPlayedAudioNanosPlusLatency();
		midiNanosOffset = currentAudioNanos - midiNanos;
		qDebug() << "Sync:" << currentAudioNanos << midiNanosOffset;
		midiNanosOffsetValid = true;
	} else {
		// FIXME: Basically assumes that the first MIDI event received is intended to play immediately.
		// Correct for clock skew.
		// FIXME: Only emergencies are handled at the moment - need to use a proper sync algorithm.
		SynthTimestamp currentAudioNanos = audioDriver->getPlayedAudioNanosPlusLatency();
		qint64 newMidiNanosOffset = currentAudioNanos - midiNanos;
		if(qAbs(newMidiNanosOffset - midiNanosOffset) > EMERGENCY_RESYNC_THRESHOLD_NANOS) {
			qDebug() << "Emergency resync:" << currentAudioNanos << midiNanosOffset << newMidiNanosOffset;
			midiNanosOffset = newMidiNanosOffset;
		}
	}
	return midiNanos + midiNanosOffset;
}

bool SynthManager::pushMIDIShortMessage(Bit32u msg, qint64 midiNanos) {
	return qSynth.pushMIDIShortMessage(msg, midiNanosToAudioNanos(midiNanos));
}

bool SynthManager::pushMIDISysex(Bit8u *sysexData, unsigned int sysexLen, qint64 midiNanos) {
	return qSynth.pushMIDISysex(sysexData, sysexLen, midiNanosToAudioNanos(midiNanos));
}

void SynthManager::startMIDI() {
	stopMIDI();
#ifdef WITH_WIN32_MIDI_DRIVER
	midiDriver = new Win32MidiDriver(this);
#else
	midiDriver = new TestMidiDriver(this);
#endif
}

void SynthManager::stopMIDI() {
	delete midiDriver;
	midiDriver = NULL;
}
