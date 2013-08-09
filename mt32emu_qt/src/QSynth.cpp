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
 * This is a wrapper for MT32Emu::Synth that adds thread-safe queueing of MIDI messages.
 *
 * Thread safety:
 * pushMIDIShortMessage() and pushMIDISysex() can be called by any number of threads safely.
 * All other functions may only be called by a single thread.
 */

#include <QtGlobal>
#include <QMessageBox>

#include "QSynth.h"
#include "Master.h"
#include "MasterClock.h"

using namespace MT32Emu;

QReportHandler::QReportHandler(QObject *parent) : QObject(parent) {
	connect(this, SIGNAL(balloonMessageAppeared(const QString &, const QString &)), Master::getInstance(), SLOT(showBalloon(const QString &, const QString &)));
}

void QReportHandler::showLCDMessage(const char *message) {
	qDebug() << "LCD-Message:" << message;
	if (Master::getInstance()->getSettings()->value("Master/showLCDBalloons", "1").toBool()) {
		emit balloonMessageAppeared("LCD-Message:", message);
	}
	emit lcdMessageDisplayed(message);
}

void QReportHandler::onErrorControlROM() {
	QMessageBox::critical(NULL, "Cannot open Synth", "Control ROM file cannot be opened.");
}

void QReportHandler::onErrorPCMROM() {
	QMessageBox::critical(NULL, "Cannot open Synth", "PCM ROM file cannot be opened.");
}

void QReportHandler::onMIDIMessagePlayed() {
	emit midiMessagePlayed();
}

void QReportHandler::onDeviceReconfig() {
	QSynth *qsynth = (QSynth *)parent();
	Bit8u currentMasterVolume = 0;
	qsynth->synth->readMemory(0x40016, 1, &currentMasterVolume);
	int masterVolume = currentMasterVolume;
	emit masterVolumeChanged(masterVolume);
}

void QReportHandler::onDeviceReset() {
	emit masterVolumeChanged(100);
}

void QReportHandler::onNewReverbMode(Bit8u mode) {
	((QSynth *)parent())->reverbMode = mode;
	emit reverbModeChanged(mode);
}

void QReportHandler::onNewReverbTime(Bit8u time) {
	((QSynth *)parent())->reverbTime = time;
	emit reverbTimeChanged(time);
}

void QReportHandler::onNewReverbLevel(Bit8u level) {
	((QSynth *)parent())->reverbLevel = level;
	emit reverbLevelChanged(level);
}

void QReportHandler::onPolyStateChanged(int partNum) {
	emit polyStateChanged(partNum);
}

void QReportHandler::onProgramChanged(int partNum, int timbreGroup, const char patchName[]) {
	emit programChanged(partNum, timbreGroup, QString().fromAscii(patchName));
}

QSynth::QSynth(QObject *parent) :
	QObject(parent), state(SynthState_CLOSED), midiMutex(QMutex::Recursive),
	controlROMImage(NULL), pcmROMImage(NULL), reportHandler(this)
{
	synthMutex = new QMutex(QMutex::Recursive);
	midiEventQueue = new QMidiEventQueue;
	synth = new Synth(&reportHandler);
}

QSynth::~QSynth() {
	freeROMImages();
	delete synth;
	delete midiEventQueue;
	delete synthMutex;
}

bool QSynth::isOpen() const {
	return state == SynthState_OPEN;
}

void QSynth::flushMIDIQueue() {
	midiMutex.lock();
	synthMutex->lock();
	// Drain synth's queue first
	synth->flushMIDIQueue();
	// Then drain our queue
	forever {
		const QMidiEvent *midiEvent = midiEventQueue->popEvent();
		if (midiEvent == NULL) break;
		if (midiEvent->getType() == SYSEX) {
			synth->playSysexNow(midiEvent->getSysexData(), midiEvent->getSysexLen());
		} else if (midiEvent->getType() == SHORT_MESSAGE) {
			synth->playMsgNow(midiEvent->getShortMessage());
		} else {
			// Special event attempted to play, ignore
		}
	}
	synthMutex->unlock();
	midiMutex.unlock();
}

void QSynth::playMIDIShortMessageNow(Bit32u msg) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->playMsgNow(msg);
	synthMutex->unlock();
}

void QSynth::playMIDISysexNow(Bit8u *sysex, Bit32u sysexLen) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->playSysexNow(sysex, sysexLen);
	synthMutex->unlock();
}

bool QSynth::playMIDIShortMessage(Bit32u msg, Bit32u timestamp) {
	midiMutex.lock();
	if (!isOpen()) {
		midiMutex.unlock();
		return false;
	}
	bool eventPushed = synth->playMsg(msg, timestamp);
	midiMutex.unlock();
	return eventPushed;
}

bool QSynth::playMIDISysex(Bit8u *sysex, Bit32u sysexLen, Bit32u timestamp) {
	midiMutex.lock();
	if (!isOpen()) {
		midiMutex.unlock();
		return false;
	}
	bool eventPushed = synth->playSysex(sysex, sysexLen, timestamp);
	midiMutex.unlock();
	return eventPushed;
}

bool QSynth::pushMIDIShortMessage(Bit32u msg, SynthTimestamp timestamp) {
	return isOpen() && midiEventQueue->pushEvent(timestamp, msg, NULL, 0);
}

bool QSynth::pushMIDISysex(Bit8u *sysexData, unsigned int sysexLen, SynthTimestamp timestamp) {
	return isOpen() && midiEventQueue->pushEvent(timestamp, 0, sysexData, sysexLen);
}

// Note that the actualSampleRate given here only affects the timing of MIDI messages.
// It does not affect the emulator's internal rendering sample rate, which is fixed at the nominal sample rate.
unsigned int QSynth::render(Bit16s *buf, unsigned int len, SynthTimestamp firstSampleTimestamp, double actualSampleRate) {
	unsigned int renderedLen = 0;

	while (renderedLen < len) {
		unsigned int renderThisPass = len - renderedLen;
		// This loop processes any events that are due before or at this sample position,
		// and potentially reduces the renderThisPass length so that the next event can occur on time on the next pass.
		bool closed = false;
		SynthTimestamp nanosNow = firstSampleTimestamp + SynthTimestamp(renderedLen * MasterClock::NANOS_PER_SECOND / actualSampleRate);
		for (;;) {
			const QMidiEvent *event = midiEventQueue->peekEvent();
			if (event == NULL) {
				// Queue empty
				break;
			}
			if (event->getTimestamp() <= nanosNow) {
				SynthTimestamp debugEventOffset = event->getTimestamp() - nanosNow;
				bool debugSpecialEvent = false;
				unsigned char *sysexData = event->getSysexData();
				synthMutex->lock();
				if (!isOpen()) {
					closed = true;
					synthMutex->unlock();
					break;
				}
				bool eventPushed = true;
				if (event->getType() == SYSEX) {
					eventPushed = synth->playSysex(sysexData, event->getSysexLen());
				} else if (event->getType() == SHORT_MESSAGE) {
					if(event->getShortMessage() == 0) {
						// This is a special event sent by the test driver
						debugSpecialEvent = true;
					} else {
						eventPushed = synth->playMsg(event->getShortMessage());
					}
				} else {
					// Special event attempted to play, ignore
					midiEventQueue->popEvent();
					continue;
				}
				synthMutex->unlock();
				if (!eventPushed) {
					// Internal synth's queue is full.
					// Rendering 1 ms looks reasonable in this case as we don't break timing too much and have no performance penalty.
					if (renderThisPass > 32) renderThisPass = 32;
					qDebug() << "QSynth: Synth's internal MIDI queue overflow, subsequent events will be delayed.";
					break;
				}
				if (debugSpecialEvent) {
					quint64 delta = (debugSampleIx - debugLastEventSampleIx);
					if (delta < 253 || 259 < delta || debugEventOffset < -1000000)
						qDebug() << "M" << debugSampleIx << 1e-6 * debugEventOffset << delta;
					debugLastEventSampleIx = debugSampleIx;
				} else if (debugEventOffset < -1000000) {
					// The MIDI event is playing significantly later than it was scheduled to play.
					qDebug() << "L" << debugSampleIx << 1e-6 * debugEventOffset;
				}
				midiEventQueue->popEvent();
				continue;
			} else {
				SynthTimestamp nanosUntilNextEvent = event->getTimestamp() - nanosNow;
				unsigned int samplesUntilNextEvent = qMax((unsigned int)1, unsigned int(nanosUntilNextEvent * actualSampleRate / MasterClock::NANOS_PER_SECOND));
				if (renderThisPass > samplesUntilNextEvent)
					renderThisPass = samplesUntilNextEvent;
				break;
			}
		}
		if (closed) {
			// The synth was found to be closed when processing events, so we break out.
			break;
		}
		synthMutex->lock();
		if (!isOpen()) {
			synthMutex->unlock();
			break;
		}
		synth->render(buf, renderThisPass);
		synthMutex->unlock();
		renderedLen += renderThisPass;
		debugSampleIx += renderThisPass;
		buf += 2 * renderThisPass;
	}
	return renderedLen;
}

bool QSynth::open(const QString useSynthProfileName) {
	if (isOpen()) {
		return true;
	}

	synthProfileName = useSynthProfileName;
	SynthProfile synthProfile;
	getSynthProfile(synthProfile);
	Master::getInstance()->loadSynthProfile(synthProfile, synthProfileName);
	setSynthProfile(synthProfile, synthProfileName);
	if (controlROMImage == NULL || pcmROMImage == NULL) {
		qDebug() << "Missing ROM files. Can't open synth :(";
		return false;
	}
	if (synth->open(*controlROMImage, *pcmROMImage)) {
		debugSampleIx = 0;
		debugLastEventSampleIx = 0;
		setState(SynthState_OPEN);
		reportHandler.onDeviceReconfig();
		setSynthProfile(synthProfile, synthProfileName);
		return true;
	}
	return false;
}

void QSynth::setMasterVolume(int masterVolume) {
	Bit8u sysex[] = {0x10, 0x00, 0x16, (Bit8u)masterVolume};

	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->writeSysex(16, sysex, 4);
	synthMutex->unlock();
}

void QSynth::setOutputGain(float outputGain) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setOutputGain(outputGain);
	synthMutex->unlock();
}

void QSynth::setReverbOutputGain(float reverbOutputGain) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setReverbOutputGain(reverbOutputGain);
	synthMutex->unlock();
}

void QSynth::setReverbEnabled(bool reverbEnabled) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setReverbEnabled(reverbEnabled);
	synthMutex->unlock();
}

void QSynth::setReverbOverridden(bool reverbOverridden) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setReverbOverridden(reverbOverridden);
	synthMutex->unlock();
}

void QSynth::setReverbSettings(int reverbMode, int reverbTime, int reverbLevel) {
	this->reverbMode = reverbMode;
	this->reverbTime = reverbTime;
	this->reverbLevel = reverbLevel;
	Bit8u sysex[] = {0x10, 0x00, 0x01, (Bit8u)reverbMode, (Bit8u)reverbTime, (Bit8u)reverbLevel};

	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setReverbOverridden(false);
	synth->writeSysex(16, sysex, 6);
	synth->setReverbOverridden(true);
	synthMutex->unlock();
}

void QSynth::setDACInputMode(DACInputMode emuDACInputMode) {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return;
	}
	synth->setDACInputMode(emuDACInputMode);
	synthMutex->unlock();
}

const QString QSynth::getPatchName(int partNum) const {
	synthMutex->lock();
	if (isOpen()) {
		const char *name = synth->getPart(partNum)->getCurrentInstr();
		synthMutex->unlock();
		return QString().fromAscii(name);
	}
	synthMutex->unlock();
	return QString("Channel %1").arg(partNum + 1);
}

const Partial *QSynth::getPartial(int partialNum) const {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return NULL;
	}
	const Partial *partial = synth->getPartial(partialNum);
	synthMutex->unlock();
	return partial;
}

const Poly *QSynth::getFirstActivePolyOnPart(unsigned int partNum) const {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return NULL;
	}
	const Poly *poly = synth->getPart(partNum)->getFirstActivePoly();
	synthMutex->unlock();
	return poly;
}

unsigned int QSynth::getPartialCount() const {
	return synth->getPartialCount();
}

bool QSynth::isActive() const {
	synthMutex->lock();
	if (!isOpen()) {
		synthMutex->unlock();
		return false;
	}
	bool result = synth->isActive();
	synthMutex->unlock();
	return result;
}

bool QSynth::reset() {
	if (!isOpen())
		return true;

	setState(SynthState_CLOSING);

	midiMutex.lock();
	synthMutex->lock();
	synth->close();
	if (!synth->open(*controlROMImage, *pcmROMImage)) {
		// We're now in a partially-open state - better to properly close.
		delete synth;
		synth = new Synth(&reportHandler);
		synthMutex->unlock();
		midiMutex.unlock();
		setState(SynthState_CLOSED);
		return false;
	}
	synthMutex->unlock();
	midiMutex.unlock();
	reportHandler.onDeviceReconfig();

	setState(SynthState_OPEN);
	return true;
}

void QSynth::setState(SynthState newState) {
	if (state == newState) {
		return;
	}
	state = newState;
	emit stateChanged(newState);
	if (isOpen()) emit partStateReset();
}

void QSynth::close() {
	setState(SynthState_CLOSING);
	midiMutex.lock();
	synthMutex->lock();
	synth->close();
	synthMutex->unlock();
	midiMutex.unlock();
	setState(SynthState_CLOSED);
	freeROMImages();
}

void QSynth::getSynthProfile(SynthProfile &synthProfile) const {
	synthMutex->lock();
	synthProfile.romDir = romDir;
	synthProfile.controlROMFileName = controlROMFileName;
	synthProfile.pcmROMFileName = pcmROMFileName;
	synthProfile.controlROMImage = controlROMImage;
	synthProfile.pcmROMImage = pcmROMImage;
	synthProfile.emuDACInputMode = synth->getDACInputMode();
	synthProfile.outputGain = synth->getOutputGain();
	synthProfile.reverbOutputGain = synth->getReverbOutputGain();
	synthProfile.reverbEnabled = synth->isReverbEnabled();
	synthProfile.reverbOverridden = synth->isReverbOverridden();
	synthProfile.reverbMode = reverbMode;
	synthProfile.reverbTime = reverbTime;
	synthProfile.reverbLevel = reverbLevel;
	synthMutex->unlock();
}

void QSynth::setSynthProfile(const SynthProfile &synthProfile, QString useSynthProfileName) {
	if (&synthProfile == NULL) {
		synthProfileName = QString();
		close();
		return;
	}
	synthProfileName = useSynthProfileName;
	romDir = synthProfile.romDir;
	controlROMFileName = synthProfile.controlROMFileName;
	pcmROMFileName = synthProfile.pcmROMFileName;
	if (controlROMImage == NULL || pcmROMImage == NULL) {
		freeROMImages();
		controlROMImage = synthProfile.controlROMImage;
		pcmROMImage = synthProfile.pcmROMImage;
	} else if (synthProfile.controlROMImage != NULL && synthProfile.pcmROMImage != NULL) {
		bool controlROMChanged = strcmp((char *)controlROMImage->getROMInfo()->sha1Digest, (char *)synthProfile.controlROMImage->getROMInfo()->sha1Digest) != 0;
		bool pcmROMChanged = strcmp((char *)pcmROMImage->getROMInfo()->sha1Digest, (char *)synthProfile.pcmROMImage->getROMInfo()->sha1Digest) != 0;
		if (controlROMChanged || pcmROMChanged) {
			freeROMImages();
			controlROMImage = synthProfile.controlROMImage;
			pcmROMImage = synthProfile.pcmROMImage;
			reset();
		}
	}
	setDACInputMode(synthProfile.emuDACInputMode);
	setOutputGain(synthProfile.outputGain);
	setReverbOutputGain(synthProfile.reverbOutputGain);
	setReverbSettings(synthProfile.reverbMode, synthProfile.reverbTime, synthProfile.reverbLevel);
	setReverbEnabled(synthProfile.reverbEnabled);
	setReverbOverridden(synthProfile.reverbOverridden);
}

void QSynth::freeROMImages() {
	// Ensure our ROM images get freed even if the synth is still in use
	const ROMImage *cri = controlROMImage;
	controlROMImage = NULL;
	const ROMImage *pri = pcmROMImage;
	pcmROMImage = NULL;
	Master::getInstance()->freeROMImages(cri, pri);
}

PartialState QSynth::getPartialState(int partialPhase) {
	static const PartialState partialPhaseToState[8] = {
		PartialState_ATTACK, PartialState_ATTACK, PartialState_ATTACK, PartialState_ATTACK,
		PartialState_SUSTAINED, PartialState_SUSTAINED, PartialState_RELEASED, PartialState_DEAD
	};
	return partialPhaseToState[partialPhase];
}

const QReportHandler *QSynth::getReportHandler() const {
	return &reportHandler;
}
