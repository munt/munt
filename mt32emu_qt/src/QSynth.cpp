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

QReportHandler::QReportHandler(QObject *parent) : QObject(parent)
{
	connect(this, SIGNAL(balloonMessageAppeared(const QString &, const QString &)), Master::getInstance(), SLOT(showBalloon(const QString &, const QString &)));
}

void QReportHandler::showLCDMessage(const char *message) {
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

void QReportHandler::onPartStateChanged(int partNum, bool isActive) {
	emit partStateChanged(partNum, isActive);
}

void QReportHandler::onPolyStateChanged(int partNum) {
	emit polyStateChanged(partNum);
}

void QReportHandler::onPartialStateChanged(int partialNum, int oldPartialPhase, int newPartialPhase) {
	static const int partialPhaseToState[8] = {
		PartialState_ATTACK, PartialState_ATTACK, PartialState_ATTACK, PartialState_ATTACK,
		PartialState_SUSTAINED, PartialState_SUSTAINED, PartialState_RELEASED, PartialState_DEAD
	};
	int newState = partialPhaseToState[newPartialPhase];
	if (partialPhaseToState[oldPartialPhase] != newState) emit partialStateChanged(partialNum, newState);
}

void QReportHandler::onProgramChanged(int partNum, char patchName[]) {
	emit programChanged(partNum, QString().fromAscii(patchName, 10));
}

QSynth::QSynth(QObject *parent) : QObject(parent), state(SynthState_CLOSED), controlROMImage(NULL), pcmROMImage(NULL) {
	isOpen = false;
	synthMutex = new QMutex(QMutex::Recursive);
	midiEventQueue = new MidiEventQueue;
	reportHandler = new QReportHandler(this);
	synth = new Synth(reportHandler);
}

QSynth::~QSynth() {
	Master::freeROMImages(controlROMImage, pcmROMImage);
	delete synth;
	delete reportHandler;
	delete midiEventQueue;
	delete synthMutex;
}

bool QSynth::pushMIDIShortMessage(Bit32u msg, SynthTimestamp timestamp) {
	return isOpen && midiEventQueue->pushEvent(timestamp, msg, NULL, 0);
}

bool QSynth::pushMIDISysex(Bit8u *sysexData, unsigned int sysexLen, SynthTimestamp timestamp) {
	return isOpen && midiEventQueue->pushEvent(timestamp, 0, sysexData, sysexLen);
}

// Note that the actualSampleRate given here only affects the timing of MIDI messages.
// It does not affect the emulator's internal rendering sample rate, which is fixed at the nominal sample rate.
unsigned int QSynth::render(Bit16s *buf, unsigned int len, SynthTimestamp firstSampleTimestamp, double actualSampleRate) {
	unsigned int renderedLen = 0;

//	qDebug() << "P" << debugSampleIx << firstSampleTimestamp << actualSampleRate << len;
	while (renderedLen < len) {
		unsigned int renderThisPass = len - renderedLen;
		// This loop processes any events that are due before or at this sample position,
		// and potentially reduces the renderThisPass length so that the next event can occur on time on the next pass.
		bool closed = false;
		qint64 nanosNow = firstSampleTimestamp + renderedLen * MasterClock::NANOS_PER_SECOND / actualSampleRate;
		for (;;) {
			const MidiEvent *event = midiEventQueue->peekEvent();
			if (event == NULL) {
				// Queue empty
//				qDebug() << "Q" << debugSampleIx;
				break;
			}
			if (event->getTimestamp() <= nanosNow) {
				qint64 debugEventOffset = event->getTimestamp() - nanosNow;
				bool debugSpecialEvent = false;
				unsigned char *sysexData = event->getSysexData();
				synthMutex->lock();
				if (!isOpen) {
					closed = true;
					synthMutex->unlock();
					break;
				}
				if (sysexData != NULL) {
					synth->playSysex(sysexData, event->getSysexLen());
				} else {
					if(event->getShortMessage() == 0) {
						// This is a special event sent by the test driver
						debugSpecialEvent = true;
					} else {
						synth->playMsg(event->getShortMessage());
					}
				}
				synthMutex->unlock();
				if (debugSpecialEvent) {
					quint64 delta = (debugSampleIx - debugLastEventSampleIx);
					if (delta < 253 || 259 < delta || debugEventOffset < -1000000)
						qDebug() << "M" << debugSampleIx << debugEventOffset << delta;
					debugLastEventSampleIx = debugSampleIx;
				} else if (debugEventOffset < -1000000) {
					// The MIDI event is playing significantly later than it was scheduled to play.
					qDebug() << "L" << debugSampleIx << 1e-6 * debugEventOffset;
				}
				midiEventQueue->popEvent();

				// This ensures minimum 1-sample delay between sequential MIDI events
				// Without this, a sequence of NoteOn and immediately succeeding NoteOff messages is always silent
				// Technically, it's also impossible to send events through the MIDI interface faster than about each millisecond
				renderThisPass = 1;
				break;
			} else {
				qint64 nanosUntilNextEvent = event->getTimestamp() - nanosNow;
				unsigned int samplesUntilNextEvent = qMax((qint64)1, (qint64)(nanosUntilNextEvent * actualSampleRate / MasterClock::NANOS_PER_SECOND));
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
		if (!isOpen) {
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

bool QSynth::open() {
	if (isOpen) {
		return true;
	}

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
		isOpen = true;
		setState(SynthState_OPEN);
		reportHandler->onDeviceReconfig();
		setSynthProfile(synthProfile, synthProfileName);
		return true;
	}
	return false;
}

void QSynth::setMasterVolume(int masterVolume) {
	if (!isOpen) {
		return;
	}
	Bit8u sysex[] = {0x10, 0x00, 0x16, masterVolume};

	synthMutex->lock();
	synth->writeSysex(16, sysex, 4);
	synthMutex->unlock();
}

void QSynth::setOutputGain(float outputGain) {
	this->outputGain = outputGain;
	if (!isOpen) {
		return;
	}

	synthMutex->lock();
	synth->setOutputGain(outputGain);
	synthMutex->unlock();
}

void QSynth::setReverbOutputGain(float reverbOutputGain) {
	this->reverbOutputGain = reverbOutputGain;
	if (!isOpen) {
		return;
	}

	synthMutex->lock();
	synth->setReverbOutputGain(/* Dry / wet gain ratio */ 0.68f * reverbOutputGain);
	synthMutex->unlock();
}

void QSynth::setReverbEnabled(bool reverbEnabled) {
	if (!isOpen) {
		return;
	}

	synthMutex->lock();
	synth->setReverbEnabled(reverbEnabled);
	synthMutex->unlock();
}

void QSynth::setReverbOverridden(bool reverbOverridden) {
	if (!isOpen) {
		return;
	}

	synthMutex->lock();
	synth->setReverbOverridden(reverbOverridden);
	synthMutex->unlock();
}

void QSynth::setReverbSettings(int reverbMode, int reverbTime, int reverbLevel) {
	this->reverbMode = reverbMode;
	this->reverbTime = reverbTime;
	this->reverbLevel = reverbLevel;
	if (!isOpen) {
		return;
	}
	Bit8u sysex[] = {0x10, 0x00, 0x01, reverbMode, reverbTime, reverbLevel};

	synthMutex->lock();
	synth->setReverbOverridden(false);
	synth->writeSysex(16, sysex, 6);
	synth->setReverbOverridden(true);
	synthMutex->unlock();
}

void QSynth::setDACInputMode(DACInputMode emuDACInputMode) {
	this->emuDACInputMode = emuDACInputMode;
	if (!isOpen) {
		return;
	}
	synthMutex->lock();
	synth->setDACInputMode(emuDACInputMode);
	synthMutex->unlock();
}

QString QSynth::getPatchName(int partNum) {
	if (isOpen) return QString().fromAscii(synth->getPart(partNum)->getCurrentInstr(), 10);
	return QString("Channel %1").arg(partNum + 1);
}

const Partial *QSynth::getPartial(int partialNum) const {
	return synth->getPartial(partialNum);
}

bool QSynth::isActive() const {
	return synth->isActive();
}

bool QSynth::reset() {
	if (!isOpen)
		return true;

	setState(SynthState_CLOSING);

	synthMutex->lock();
	delete synth;
	synth = new Synth(reportHandler);
	if (!synth->open(*controlROMImage, *pcmROMImage)) {
		// We're now in a partially-open state - better to properly close.
		delete synth;
		synth = new Synth(reportHandler);
		isOpen = false;
		synthMutex->unlock();
		setState(SynthState_CLOSED);
		return false;
	}
	synthMutex->unlock();
	reportHandler->onDeviceReconfig();

	setState(SynthState_OPEN);
	return true;
}

void QSynth::setState(SynthState newState) {
	if (state == newState) {
		return;
	}
	state = newState;
	emit stateChanged(newState);
	if (isOpen) emit partStateReset();
}

void QSynth::close() {
	setState(SynthState_CLOSING);
	synthMutex->lock();
	isOpen = false;
	//audioOutput->stop();
	synth->close();
	synthMutex->unlock();
	setState(SynthState_CLOSED);
}

void QSynth::getSynthProfile(SynthProfile &synthProfile) const {
	synthProfile.romDir = romDir;
	synthProfile.controlROMFileName = controlROMFileName;
	synthProfile.pcmROMFileName = pcmROMFileName;
	synthProfile.controlROMImage = controlROMImage;
	synthProfile.pcmROMImage = pcmROMImage;
	synthProfile.emuDACInputMode = emuDACInputMode;
	synthProfile.outputGain = outputGain;
	synthProfile.reverbOutputGain = reverbOutputGain;
	synthProfile.reverbEnabled = synth->isReverbEnabled();
	synthProfile.reverbOverridden = synth->isReverbOverridden();
	synthProfile.reverbMode = reverbMode;
	synthProfile.reverbTime = reverbTime;
	synthProfile.reverbLevel = reverbLevel;
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
		controlROMImage = synthProfile.controlROMImage;
		pcmROMImage = synthProfile.pcmROMImage;
	} else if (synthProfile.controlROMImage != NULL && synthProfile.pcmROMImage != NULL) {
		bool controlROMChanged = strcmp((char *)controlROMImage->getROMInfo()->sha1Digest, (char *)synthProfile.controlROMImage->getROMInfo()->sha1Digest) != 0;
		bool pcmROMChanged = strcmp((char *)pcmROMImage->getROMInfo()->sha1Digest, (char *)synthProfile.pcmROMImage->getROMInfo()->sha1Digest) != 0;
		if (controlROMChanged || pcmROMChanged) {
			Master::freeROMImages(controlROMImage, pcmROMImage);
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
