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

/**
 * SynthRoute is responsible for:
 * - Managing the audio output
 *  - Initial setup
 *  - Sample rate changes
 *  - Pausing/unpausing when the QSynth becomes unavailable (NYI)
 * - Maintaining a list of MIDI sessions for the synth
 * - Merging MIDI streams coming from several MIDI sessions
 */

#include <limits>

#include "SynthRoute.h"
#include "MidiSession.h"
#include "QMidiBuffer.h"
#include "RealtimeReadLocker.h"
#include "audiodrv/AudioDriver.h"

using namespace MT32Emu;

SynthRoute::SynthRoute(QObject *parent) :
	QObject(parent),
	state(SynthRouteState_CLOSED),
	qSynth(this),
	exclusiveMidiMode(),
	multiMidiMode(),
	audioDevice(NULL),
	audioStream(NULL),
	debugLastEventTimestamp(0)
{
	connect(&qSynth, SIGNAL(stateChanged(SynthState)), SLOT(handleQSynthState(SynthState)));
}

SynthRoute::~SynthRoute() {
	deleteAudioStream();
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

bool SynthRoute::open(AudioStreamFactory audioStreamFactory) {
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

			AudioStream *newAudioStream = exclusiveMidiMode && audioStreamFactory != NULL
				? audioStreamFactory(audioDevice, *this, sampleRate, midiSessions.first())
				: audioDevice->startAudioStream(*this, sampleRate);
			if (newAudioStream != NULL) {
				setState(SynthRouteState_OPEN);
				QWriteLocker audioStreamLocker(&audioStreamLock);
				audioStream = newAudioStream;
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
	deleteAudioStream();
	qSynth.close();
	disableExclusiveMidiMode();
	discardMidiBuffers();
	return true;
}

bool SynthRoute::enableExclusiveMidiMode(MidiSession *midiSession) {
	if (exclusiveMidiMode || hasMIDISessions()) return false;
	addMidiSession(midiSession);
	exclusiveMidiMode = true;
	qDebug() << "SynthRoute: exclusiveMidiMode enabled";
	return true;
}

void SynthRoute::disableExclusiveMidiMode() {
	if (exclusiveMidiMode && hasMIDISessions()) {
		MidiSession *midiSession = midiSessions.first();
		removeMidiSession(midiSession);
		exclusiveMidiMode = false;
		qDebug() << "SynthRoute: exclusiveMidiMode disabled";
		emit exclusiveMidiSessionRemoved(midiSession);
	}
}

bool SynthRoute::isExclusiveMidiModeEnabled() {
	return exclusiveMidiMode;
}

void SynthRoute::enableMultiMidiMode() {
	if (exclusiveMidiMode || multiMidiMode) return;
	multiMidiMode = true;
	qDebug() << "SynthRoute: started merging MIDI stream buffers";
}

SynthRouteState SynthRoute::getState() const {
	return state;
}

void SynthRoute::addMidiSession(MidiSession *midiSession) {
	if (exclusiveMidiMode) return;
	if (hasMIDISessions() && !multiMidiMode) enableMultiMidiMode();
	QMutexLocker midiSessionsLocker(&midiSessionsMutex);
	midiSessions.append(midiSession);
	if (midiRecorder.isRecording()) midiSession->setMidiTrackRecorder(midiRecorder.addTrack());
	emit midiSessionAdded(midiSession);
}

void SynthRoute::removeMidiSession(MidiSession *midiSession) {
	QMutexLocker midiSessionsLocker(&midiSessionsMutex);
	midiSessions.removeOne(midiSession);
	emit midiSessionRemoved(midiSession);
	if (!hasMIDISessions() && multiMidiMode) {
		multiMidiMode = false;
		qDebug() << "SynthRoute: stopped merging MIDI stream buffers";
	}
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
		deleteAudioStream();
		setState(SynthRouteState_CLOSED);
		disableExclusiveMidiMode();
		break;
	}
}

bool SynthRoute::connectSynth(const char *signal, const QObject *receiver, const char *slot) const {
	return QObject::connect(&qSynth, signal, receiver, slot);
}

bool SynthRoute::disconnectSynth(const char *signal, const QObject *receiver, const char *slot) const {
	return QObject::disconnect(&qSynth, signal, receiver, slot);
}

bool SynthRoute::connectReportHandler(const char *signal, const QObject *receiver, const char *slot) const {
	return QObject::connect(qSynth.getReportHandler(), signal, receiver, slot);
}

bool SynthRoute::disconnectReportHandler(const char *signal, const QObject *receiver, const char *slot) const {
	return QObject::disconnect(qSynth.getReportHandler(), signal, receiver, slot);
}

bool SynthRoute::pushMIDIShortMessage(MidiSession &midiSession, Bit32u msg, MasterClockNanos refNanos) {
	if (midiRecorder.isRecording()) midiSession.getMidiTrackRecorder()->recordShortMessage(msg, refNanos);
	quint64 timestamp;
	{
		RealtimeReadLocker audioStreamLocker(audioStreamLock);
		if (!audioStreamLocker.isLocked() || audioStream == NULL) return false;
		timestamp = audioStream->estimateMIDITimestamp(refNanos);
	}
	if (msg == 0) {
		// This is a special event sent by the test driver
		qint64 delta = qint64(timestamp) - qint64(debugLastEventTimestamp);
		MasterClockNanos debugEventNanoOffset = MasterClock::getClockNanos() - refNanos;
		if ((delta < debugDeltaLowerLimit) || (debugDeltaUpperLimit < delta) || ((15 * MasterClock::NANOS_PER_MILLISECOND) < debugEventNanoOffset)) {
			qDebug() << "M" << delta << timestamp << 1e-6 * debugEventNanoOffset;
		}
		debugLastEventTimestamp = timestamp;
		return false;
	}
	return playMIDIShortMessage(midiSession, msg, timestamp);
}

bool SynthRoute::pushMIDISysex(MidiSession &midiSession, const Bit8u *sysexData, unsigned int sysexLen, MasterClockNanos refNanos) {
	if (midiRecorder.isRecording()) midiSession.getMidiTrackRecorder()->recordSysex(sysexData, sysexLen, refNanos);
	quint64 timestamp;
	{
		RealtimeReadLocker audioStreamLocker(audioStreamLock);
		if (!audioStreamLocker.isLocked() || audioStream == NULL) return false;
		timestamp = audioStream->estimateMIDITimestamp(refNanos);
	}
	return playMIDISysex(midiSession, sysexData, sysexLen, timestamp);
}

bool SynthRoute::playMIDIShortMessage(MidiSession &midiSession, Bit32u msg, quint64 timestamp) {
	if (multiMidiMode) {
		QMidiBuffer *qMidiBuffer = midiSession.getQMidiBuffer();
		if (qMidiBuffer->pushShortMessage(timestamp, msg)) {
			qMidiBuffer->flush();
			return true;
		}
		return false;
	} else {
		return qSynth.playMIDIShortMessage(msg, timestamp);
	}
}

bool SynthRoute::playMIDISysex(MidiSession &midiSession, const Bit8u *sysex, Bit32u sysexLen, quint64 timestamp) {
	if (multiMidiMode) {
		QMidiBuffer *qMidiBuffer = midiSession.getQMidiBuffer();
		if (qMidiBuffer->pushSysexMessage(timestamp, sysexLen, sysex)) {
			qMidiBuffer->flush();
			return true;
		}
		return false;
	} else {
		return qSynth.playMIDISysex(sysex, sysexLen, timestamp);
	}
}

void SynthRoute::discardMidiBuffers() {
	if (multiMidiMode) {
		QMutexLocker midiSessionsLocker(&midiSessionsMutex);

		for (int i = 0; i < midiSessions.size(); i++) {
			QMidiBuffer *midiBuffer = midiSessions[i]->getQMidiBuffer();
			while (midiBuffer->retrieveEvents()) {
				midiBuffer->discardEvents();
			}
		}
	}
	qSynth.flushMIDIQueue();
}

void SynthRoute::flushMIDIQueue() {
	if (multiMidiMode) mergeMidiStreams(0);
	qSynth.flushMIDIQueue();
}

// When renderingPassFrameLength == 0, all pending messages are merged.
void SynthRoute::mergeMidiStreams(uint renderingPassFrameLength) {
	quint64 renderingPassEndTimestamp;
	if (renderingPassFrameLength > 0) {
		RealtimeReadLocker audioStreamLocker(audioStreamLock);
		// Occasionally, audioStream may appear NULL during startup.
		if (!audioStreamLocker.isLocked() || audioStream == NULL) return;
		renderingPassEndTimestamp = audioStream->computeMIDITimestamp(renderingPassFrameLength);
	} else {
		renderingPassEndTimestamp = std::numeric_limits<quint64>::max();
	}

	QMutexLocker midiSessionsLocker(&midiSessionsMutex);
	QVarLengthArray<QMidiBuffer *, 16> streamBuffers;
	for (int i = 0; i < midiSessions.size(); i++) {
		QMidiBuffer *midiBuffer = midiSessions[i]->getQMidiBuffer();
		if (midiBuffer->retrieveEvents() && midiBuffer->getEventTimestamp() < renderingPassEndTimestamp) {
			streamBuffers.append(midiBuffer);
		}
	}

	while (!streamBuffers.isEmpty()) {
		int nextEventBufferIx = 0;
		quint64 nextEventTimestamp = streamBuffers[nextEventBufferIx]->getEventTimestamp();
		for (int i = 1; i < streamBuffers.size(); i++) {
			quint64 eventTimestamp = streamBuffers[i]->getEventTimestamp();
			if (eventTimestamp < nextEventTimestamp) {
				nextEventBufferIx = i;
				nextEventTimestamp = eventTimestamp;
			}
		}
		QMidiBuffer * const midiBuffer = streamBuffers[nextEventBufferIx];
		do {
			const uchar *sysexData;
			quint32 eventData = midiBuffer->getEventData(sysexData);
			if (sysexData == NULL) {
				qSynth.playMIDIShortMessage(eventData, nextEventTimestamp);
			} else {
				qSynth.playMIDISysex(sysexData, eventData, nextEventTimestamp);
			}
			if (!(midiBuffer->nextEvent() && midiBuffer->getEventTimestamp() < renderingPassEndTimestamp)) {
#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
				if (nextEventBufferIx < streamBuffers.count() - 1) {
					streamBuffers[nextEventBufferIx] = streamBuffers[streamBuffers.count() - 1];
				}
				streamBuffers.removeLast();
#else
				streamBuffers.remove(nextEventBufferIx);
#endif
				break;
			}
		} while (midiBuffer->getEventTimestamp() <= nextEventTimestamp);
	}
}

void SynthRoute::deleteAudioStream() {
	QWriteLocker audioStreamLocker(&audioStreamLock);
	delete audioStream;
	audioStream = NULL;
}

void SynthRoute::render(MT32Emu::Bit16s *buffer, uint length) {
	if (multiMidiMode) mergeMidiStreams(length);
	qSynth.render(buffer, length);
}

void SynthRoute::render(float *buffer, uint length) {
	if (multiMidiMode) mergeMidiStreams(length);
	qSynth.render(buffer, length);
}

void SynthRoute::audioStreamFailed() {
	qSynth.close();
}

// QSynth delegation

void SynthRoute::enableRealtimeMode() {
	qSynth.enableRealtime();
}

void SynthRoute::playMIDIShortMessageNow(Bit32u msg) {
	qSynth.playMIDIShortMessageNow(msg);
}

void SynthRoute::playMIDISysexNow(const Bit8u *sysex, Bit32u sysexLen) {
	qSynth.playMIDISysexNow(sysex, sysexLen);
}

void SynthRoute::reset() {
	qSynth.reset();
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

void SynthRoute::setPartVolumeOverride(uint partNumber, uint volumeOverride) {
	qSynth.setPartVolumeOverride(partNumber, volumeOverride);
}

void SynthRoute::setReversedStereoEnabled(bool enabled) {
	qSynth.setReversedStereoEnabled(enabled);
}

void SynthRoute::setNiceAmpRampEnabled(bool enabled) {
	qSynth.setNiceAmpRampEnabled(enabled);
}

void SynthRoute::setNicePanningEnabled(bool enabled) {
	qSynth.setNicePanningEnabled(enabled);
}

void SynthRoute::setNicePartialMixingEnabled(bool enabled) {
	qSynth.setNicePartialMixingEnabled(enabled);
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

void SynthRoute::setDisplayCompatibilityMode(DisplayCompatibilityMode displayCompatibilityMode) {
	qSynth.setDisplayCompatibilityMode(displayCompatibilityMode);
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

uint SynthRoute::getPartialCount() const {
	return qSynth.getPartialCount();
}

const QString SynthRoute::getPatchName(int partNum) const {
	return qSynth.getPatchName(partNum);
}

void SynthRoute::setTimbreOnPart(uint partNumber, uint timbreGroup, uint timbreNumber) {
	qSynth.setTimbreOnPart(partNumber, timbreGroup, timbreNumber);
}

void SynthRoute::getSoundGroups(QVector<SoundGroup> &groups) const {
	qSynth.getSoundGroups(groups);
}

void SynthRoute::getPartialStates(PartialState *partialStates) const {
	qSynth.getPartialStates(partialStates);
}

uint SynthRoute::getPlayingNotes(unsigned int partNumber, MT32Emu::Bit8u *keys, MT32Emu::Bit8u *velocities) const {
	return qSynth.getPlayingNotes(partNumber, keys, velocities);
}

bool SynthRoute::getDisplayState(char *targetBuffer) const {
	return qSynth.getDisplayState(targetBuffer);
}

void SynthRoute::setMainDisplayMode() {
	qSynth.setMainDisplayMode();
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

void SynthRoute::startRecordingMidi() {
	for (int i = 0; i < midiSessions.size(); i++) {
		midiSessions.at(i)->setMidiTrackRecorder(midiRecorder.addTrack());
	}
	midiRecorder.startRecording();
}

bool SynthRoute::stopRecordingMidi() {
	return midiRecorder.stopRecording();
}

void SynthRoute::saveRecordedMidi(const QString &fileName, MasterClockNanos midiTick) {
	if (!midiRecorder.saveSMF(fileName, midiTick)) {
		qWarning() << "SynthRoute: Failed to write recorded MIDI data to file" << fileName;
	}
	for (int i = 0; i < midiSessions.size(); i++) {
		midiSessions.at(i)->setMidiTrackRecorder(NULL);
	}
}

bool SynthRoute::isRecordingMidi() const {
	return midiRecorder.isRecording();
}
