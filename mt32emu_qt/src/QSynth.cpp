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
 * This is a wrapper for MT32Emu::Synth that adds thread-safe queueing of MIDI messages
 * and audio output via Qt.
 *
 * Thread safety:
 * pushMIDIShortMessage() and pushMIDISysex() can be called by any number of threads safely.
 * All other functions may only be called by a single thread.
 */

#include <cstdio>

#include <QtMultimediaKit/QAudioOutput>

#include "QSynth.h"

using namespace MT32Emu;

static const int SAMPLE_RATE = 32000;

class WaveGenerator : public QIODevice {
private:
	QSynth *midiSynth;

public:
	WaveGenerator(QSynth *pMidiSynth) {
		midiSynth = pMidiSynth;
		open(QIODevice::ReadOnly);
	}

	qint64 readData(char *data, qint64 len) {
		return midiSynth->render((Bit16s *)data, (unsigned int)(len >> 2)) << 2;
	}

	qint64 writeData(const char *data, qint64 len) {
		Q_UNUSED(data);
		Q_UNUSED(len);
		return 0;
	}
};

int MT32_Report(void *userData, ReportType type, const void *reportData) {
	Q_UNUSED(userData);
	Q_UNUSED(type);
	Q_UNUSED(reportData);
	return 0;
}

void QSynth::handleReport(ReportType type, const void *reportData) {
}

bool QSynth::pushMIDIShortMessage(Bit32u msg) {
	return midiEventQueue->pushEvent(getTimestamp(), msg, NULL, 0);
}

bool QSynth::pushMIDISysex(Bit8u *sysexData, unsigned int sysexLen) {
	Bit8u *copy = new Bit8u[sysexLen];
	memcpy(copy, sysexData, sysexLen);
	bool result = midiEventQueue->pushEvent(getTimestamp(), 0, copy, sysexLen);
	if (!result) {
		delete[] copy;
	}
	return result;
}

unsigned int QSynth::render(Bit16s *buf, unsigned int len) {
	unsigned int renderedLen = 0;

		// Set timestamp of buffer start
		synthMutex->lock();
		bufferStartSampleIx = sampleIx;
		bufferStartClock = clock();
		synthMutex->unlock();

		while (renderedLen < len) {
		unsigned int renderThisPass = len - renderedLen;
		// This loop processes any events that are due before or at this sample position,
		// and potentially reduces the renderThisPass length so that the next event can occur on time on the next pass.
		bool closed = false;
		for (;;) {
			const MidiEvent *event = midiEventQueue->peekEvent();
			if (event == NULL) {
				break;
			}
			if (event->getTimestamp() <= sampleIx) {
				if (event->getTimestamp() != sampleIx) {
					printf("Overdue timestamp: %lld\n", sampleIx - event->getTimestamp());
				}
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
					synth->playMsg(event->getShortMessage());
				}
				synthMutex->unlock();
				midiEventQueue->popEvent();
			} else {
				unsigned int samplesUntilNextEvent = event->getTimestamp() - sampleIx;
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
		sampleIx += renderThisPass;
		buf += 2 * renderThisPass;
	}
	return renderedLen;
}

QSynth::QSynth() {
	sampleRate = 32000;
	latency = 8000; // In samples
	reverbEnabled = true;
	emuDACInputMode = DACInputMode_GENERATION2;
	// FIXME: Awful
#ifndef __WIN32__
	romDir = "/usr/local/share/munt/roms/";
#endif
	isOpen = false;

	synthMutex = new QMutex(QMutex::Recursive);
	midiEventQueue = new MidiEventQueue;
	synth = new Synth();

	QAudioFormat format;
	format.setFrequency(SAMPLE_RATE);
	format.setChannels(2);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	// libmt32emu produces samples in native byte order
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
	format.setByteOrder(QAudioFormat::LittleEndian);
#else
	format.setByteOrder(QAudioFormat::BigEndian);
#endif
	format.setSampleType(QAudioFormat::SignedInt);

	waveGenerator = new WaveGenerator(this);
	audioOutput = new QAudioOutput(format, waveGenerator);
}

QSynth::~QSynth() {
	delete synth;
	delete midiEventQueue;
	delete audioOutput;
	delete waveGenerator;
}

QDir QSynth::getROMDir() {
	return romDir;
}

int QSynth::setROMDir(QDir newROMDir) {
	romDir = newROMDir;
	return reset();
}

bool QSynth::openSynth() {
	QString pathName = QDir::toNativeSeparators(romDir.absolutePath());
	pathName += QDir::separator();
	SynthProperties synthProp = {SAMPLE_RATE, true, true, 0, 0, 0, pathName.toUtf8(), NULL, MT32_Report, NULL, NULL, NULL};
	if(synth->open(synthProp)) {
		synth->setReverbEnabled(reverbEnabled);
		synth->setDACInputMode(emuDACInputMode);
	}
}

int QSynth::open() {
	if (isOpen) {
		return 1;
	}

	if (openSynth()) {
		sampleIx = 0;
		bufferStartSampleIx = 0;
		bufferStartClock = clock() - latency;
		audioOutput->start(waveGenerator);
		isOpen = true;
		return 0;
	}
	return 1;
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

void QSynth::setLatency(unsigned int newLatency) {
	latency = newLatency;
}

int QSynth::reset() {
	if (!isOpen)
		return 1;

	audioOutput->suspend();

	synthMutex->lock();
	synth->close();
	delete synth;
	synth = new Synth();
	if (!openSynth()) {
		// We're now in a partially-open state - better to properly close.
		close();
		synthMutex->unlock();
		return 1;
	}
	synthMutex->unlock();

	audioOutput->resume();
	return 0;
}

SynthTimestamp QSynth::getTimestamp() {
	clock_t c = clock();
	synthMutex->lock();
	SynthTimestamp timestamp = bufferStartSampleIx + latency;
	timestamp += (SynthTimestamp)((double)SAMPLE_RATE * (c - bufferStartClock) / CLOCKS_PER_SEC);
	synthMutex->unlock();
	return timestamp;
}

void QSynth::close() {
	synthMutex->lock();
	isOpen = false;
	audioOutput->stop();
	synth->close();
	synthMutex->unlock();
}
