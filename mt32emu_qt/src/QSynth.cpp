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

#include <cstdio>

#include <QtGlobal>

#include "QSynth.h"
#include "Master.h"
#include "MasterClock.h"

using namespace MT32Emu;

static const int DEFAULT_SAMPLE_RATE = 32000;

int MT32_Report(void *userData, ReportType type, const void *reportData) {
	QSynth *qSynth = (QSynth *)userData;
	return qSynth->handleReport(type, reportData);
}

QSynth::QSynth(QObject *parent) : QObject(parent), state(SynthState_CLOSED) {
	sampleRate = DEFAULT_SAMPLE_RATE;
	reverbEnabled = true;
	emuDACInputMode = DACInputMode_NICE;
	isOpen = false;
	synthMutex = new QMutex(QMutex::Recursive);
	midiEventQueue = new MidiEventQueue;
	synth = new Synth();
}

QSynth::~QSynth() {
	delete synth;
	delete midiEventQueue;
	delete synthMutex;
}

int QSynth::handleReport(ReportType type, const void *reportData) {
	Q_UNUSED(type);
	Q_UNUSED(reportData);
	return 0;
}

bool QSynth::pushMIDIShortMessage(Bit32u msg, SynthTimestamp timestamp) {
	return midiEventQueue->pushEvent(timestamp, msg, NULL, 0);
}

bool QSynth::pushMIDISysex(Bit8u *sysexData, unsigned int sysexLen, SynthTimestamp timestamp) {
	Bit8u *copy = new Bit8u[sysexLen];
	memcpy(copy, sysexData, sysexLen);
	bool result = midiEventQueue->pushEvent(timestamp, 0, copy, sysexLen);
	if (!result) {
		delete[] copy;
	}
	return result;
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
					qDebug() << "M" << debugSampleIx << debugEventOffset << (debugSampleIx - debugLastEventSampleIx);
					debugLastEventSampleIx = debugSampleIx;
				} else if (debugEventOffset < -1000000) {
					// The MIDI event is playing significantly later than it was scheduled to play.
					qDebug() << "L" << debugSampleIx << 1e-6 * debugEventOffset;
				}
				midiEventQueue->popEvent();
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
		if (!isOpen)
			break;
		synth->render(buf, renderThisPass);
		synthMutex->unlock();
		renderedLen += renderThisPass;
		debugSampleIx += renderThisPass;
		buf += 2 * renderThisPass;
	}
	return renderedLen;
}

bool QSynth::openSynth() {
	QString pathName = QDir::toNativeSeparators(Master::getInstance()->getROMDir().absolutePath());
	pathName += QDir::separator();
	QByteArray pathNameBytes = pathName.toUtf8();
	SynthProperties synthProp = {sampleRate, true, true, 0, 0, 0, pathNameBytes, NULL, MT32_Report, NULL, NULL, NULL};
	if(synth->open(synthProp)) {
		synth->setReverbEnabled(reverbEnabled);
		synth->setDACInputMode(emuDACInputMode);
		return true;
	}
	return false;
}

bool QSynth::open() {
	if (isOpen) {
		return true;
	}

	if (openSynth()) {
		debugSampleIx = 0;
		debugLastEventSampleIx = 0;
		isOpen = true;
		setState(SynthState_OPEN);
		return true;
	}
	return false;
}

void QSynth::setMasterVolume(Bit8u masterVolume) {
	if (!isOpen) {
		return;
	}
	Bit8u sysex[] = {0x10, 0x00, 0x16, 0x01};
	sysex[3] = masterVolume;

	synthMutex->lock();
	synth->writeSysex(16, sysex, 4);
	synthMutex->unlock();
}

void QSynth::setReverbEnabled(bool newReverbEnabled) {
	reverbEnabled = newReverbEnabled;
	if (!isOpen) {
		return;
	}

	synthMutex->lock();
	synth->setReverbEnabled(reverbEnabled);
	synthMutex->unlock();
}

void QSynth::setDACInputMode(DACInputMode newEmuDACInputMode) {
	emuDACInputMode = newEmuDACInputMode;
	if (!isOpen) {
		return;
	}
	synthMutex->lock();
	synth->setDACInputMode(emuDACInputMode);
	synthMutex->unlock();
}

bool QSynth::reset() {
	if (!isOpen)
		return true;

	setState(SynthState_CLOSING);

	synthMutex->lock();
	synth->close();
	delete synth;
	synth = new Synth();
	if (!openSynth()) {
		// We're now in a partially-open state - better to properly close.
		close();
		synthMutex->unlock();
		return false;
	}
	synthMutex->unlock();

	setState(SynthState_OPEN);
	return true;
}

void QSynth::setState(SynthState newState) {
	if (state == newState) {
		return;
	}
	state = newState;
	emit stateChanged(newState);
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
