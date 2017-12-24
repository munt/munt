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
	audioStream(NULL),
	debugLastEventTimestamp(0)
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
		uint sampleRate = audioDevice->driver.getAudioSettings().sampleRate;
		if (qSynth.open(sampleRate, audioDevice->driver.getAudioSettings().srcQuality)) {
			double debugDeltaMean = sampleRate * (8.0 / MasterClock::MILLIS_PER_SECOND);
			double debugDeltaLimit = debugDeltaMean * 0.01;
			debugDeltaLowerLimit = qint64(floor(debugDeltaMean - debugDeltaLimit));
			debugDeltaUpperLimit = qint64(ceil(debugDeltaMean + debugDeltaLimit));
			qDebug() << "Using sample rate:" << sampleRate;

			audioStream = audioDevice->startAudioStream(qSynth, sampleRate);
			if (audioStream != NULL) {
				setState(SynthRouteState_OPEN);
				return true;
			} else {
				qDebug() << "Failed to start audioStream";
				qSynth.close();
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

MidiRecorder *SynthRoute::getMidiRecorder() {
	return &recorder;
}

bool SynthRoute::connectSynth(const char *signal, const QObject *receiver, const char *slot) const {
	return QObject::connect(&qSynth, signal, receiver, slot);
}

bool SynthRoute::connectReportHandler(const char *signal, const QObject *receiver, const char *slot) const {
	return QObject::connect(qSynth.getReportHandler(), signal, receiver, slot);
}

// QSynth delegation

bool SynthRoute::pushMIDIShortMessage(Bit32u msg, MasterClockNanos refNanos) {
	recorder.recordShortMessage(msg, refNanos);
	AudioStream *stream = audioStream;
	if (stream == NULL) return false;
	quint64 timestamp = stream->estimateMIDITimestamp(refNanos);
	if (msg == 0) {
		// This is a special event sent by the test driver
		qint64 delta = qint64(timestamp - debugLastEventTimestamp);
		MasterClockNanos debugEventNanoOffset = (refNanos == 0) ? 0 : MasterClock::getClockNanos() - refNanos;
		if ((delta < debugDeltaLowerLimit) || (debugDeltaUpperLimit < delta) || ((15 * MasterClock::NANOS_PER_MILLISECOND) < debugEventNanoOffset)) {
			qDebug() << "M" << delta << timestamp << 1e-6 * debugEventNanoOffset;
		}
		debugLastEventTimestamp = timestamp;
		return false;
	}
	return qSynth.playMIDIShortMessage(msg, timestamp);
}

bool SynthRoute::pushMIDISysex(const Bit8u *sysexData, unsigned int sysexLen, MasterClockNanos refNanos) {
	recorder.recordSysex(sysexData, sysexLen, refNanos);
	AudioStream *stream = audioStream;
	if (stream == NULL) return false;
	quint64 timestamp = stream->estimateMIDITimestamp(refNanos);
	return qSynth.playMIDISysex(sysexData, sysexLen, timestamp);
}

void SynthRoute::flushMIDIQueue() {
	qSynth.flushMIDIQueue();
}

void SynthRoute::playMIDIShortMessageNow(Bit32u msg) {
	qSynth.playMIDIShortMessageNow(msg);
}

void SynthRoute::playMIDISysexNow(const Bit8u *sysex, Bit32u sysexLen) {
	qSynth.playMIDISysexNow(sysex, sysexLen);
}

bool SynthRoute::playMIDIShortMessage(Bit32u msg, quint64 timestamp) {
	return qSynth.playMIDIShortMessage(msg, timestamp);
}

bool SynthRoute::playMIDISysex(const Bit8u *sysex, Bit32u sysexLen, quint64 timestamp) {
	return qSynth.playMIDISysex(sysex, sysexLen, timestamp);
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

void SynthRoute::setReversedStereoEnabled(bool enabled) {
	qSynth.setReversedStereoEnabled(enabled);
}

void SynthRoute::setNiceAmpRampEnabled(bool enabled) {
	qSynth.setNiceAmpRampEnabled(enabled);
}

void SynthRoute::resetMIDIChannelsAssignment(bool engageChannel1) {
	qSynth.resetMIDIChannelsAssignment(engageChannel1);
}

void SynthRoute::setInitialMIDIChannelsAssignment(bool engageChannel1) {
	qSynth.setInitialMIDIChannelsAssignment(engageChannel1);
}

void SynthRoute::setReverbCompatibilityMode(ReverbCompatibilityMode reverbCompatibilityMode) {
	qSynth.setReverbCompatibilityMode(reverbCompatibilityMode);
}

void SynthRoute::setMIDIDelayMode(MIDIDelayMode midiDelayMode) {
	qSynth.setMIDIDelayMode(midiDelayMode);
}

void SynthRoute::setDACInputMode(DACInputMode emuDACInputMode) {
	qSynth.setDACInputMode(emuDACInputMode);
}

void SynthRoute::setAnalogOutputMode(AnalogOutputMode analogOutputMode) {
	qSynth.setAnalogOutputMode(analogOutputMode);
}

void SynthRoute::setRendererType(RendererType rendererType) {
	qSynth.setRendererType(rendererType);
}

void SynthRoute::setPartialCount(int partialCount) {
	qSynth.setPartialCount(partialCount);
}

void SynthRoute::getSynthProfile(SynthProfile &synthProfile) const {
	qSynth.getSynthProfile(synthProfile);
}

void SynthRoute::setSynthProfile(const SynthProfile &synthProfile, QString useSynthProfileName) {
	qSynth.setSynthProfile(synthProfile, useSynthProfileName);
}

void SynthRoute::getROMImages(const MT32Emu::ROMImage *&controlROMImage, const MT32Emu::ROMImage *&pcmROMImage) const {
	qSynth.getROMImages(controlROMImage, pcmROMImage);
}

unsigned int SynthRoute::getPartialCount() const {
	return qSynth.getPartialCount();
}

const QString SynthRoute::getPatchName(int partNum) const {
	return qSynth.getPatchName(partNum);
}

void SynthRoute::getPartStates(bool *partStates) const {
	qSynth.getPartStates(partStates);
}

void SynthRoute::getPartialStates(PartialState *partialStates) const {
	qSynth.getPartialStates(partialStates);
}

unsigned int SynthRoute::getPlayingNotes(unsigned int partNumber, MT32Emu::Bit8u *keys, MT32Emu::Bit8u *velocities) const {
	return qSynth.getPlayingNotes(partNumber, keys, velocities);
}

void SynthRoute::startRecordingAudio(const QString &fileName) {
	qSynth.startRecordingAudio(fileName);
}

void SynthRoute::stopRecordingAudio() {
	qSynth.stopRecordingAudio();
}

bool SynthRoute::isRecordingAudio() const {
	return qSynth.isRecordingAudio();
}
