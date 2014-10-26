/* Copyright (C) 2011, 2012, 2013, 2014 Jerome Fisher, Sergey V. Mikayev
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

#include <QMessageBox>

#include "AudioFileWriter.h"
#include "Master.h"

static const unsigned int FRAME_SIZE = 4; // Stereo, 16-bit
static const unsigned char WAVE_HEADER[] = {
	0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
	0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x44, 0xAC, 0x00, 0x00, 0x10, 0xB1, 0x02, 0x00,
	0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00, 0x00
};

AudioFileWriter::AudioFileWriter() : synth(NULL), buffer(NULL), parsers(NULL) {
	connect(this, SIGNAL(parsingFailed(const QString &, const QString &)), Master::getInstance(), SLOT(showBalloon(const QString &, const QString &)));
}

AudioFileWriter::~AudioFileWriter() {
	stop();
	if (!realtimeMode && synth != NULL) {
		synth->close();
		delete synth;
	}
	delete[] parsers;
	delete[] buffer;
}

bool AudioFileWriter::convertMIDIFiles(QString useOutFileName, QStringList midiFileNameList, QString synthProfileName, unsigned int useBufferSize) {
	if (useOutFileName.isEmpty() || midiFileNameList.isEmpty()) return false;
	delete[] parsers;
	parsersCount = midiFileNameList.size();
	parsers = new MidiParser[parsersCount];
	for (uint i = 0; i < parsersCount; i++) {
		if (!parsers[i].parse(midiFileNameList.at(i))) {
			qDebug() << "AudioFileWriter: Error parsing MIDI files";
			const QMidiEventList &midiEvents = parsers[i].getMIDIEvents();
			if (midiEvents.count() == 0) {
				QMessageBox::critical(NULL, "Error", "Error occured while parsing MIDI files. No MIDI events to process.");
				delete[] parsers;
				parsers = NULL;
				return false;
			}
			emit parsingFailed("Warning", "Error occured while parsing MIDI files. Processing available MIDI events.");
		}
	}
	parsers[parsersCount - 1].addChannelsReset();
	if (synth != NULL) {
		synth->close();
		delete synth;
	}
	synth = new QSynth(this);
	if (!synth->open(0, synthProfileName)) {
		synth->close();
		delete synth;
		synth = NULL;
		delete[] parsers;
		parsers = NULL;
		qDebug() << "AudioFileWriter: Can't open synth";
		QMessageBox::critical(NULL, "Error", "Failed to open synth");
		return false;
	}
	Master::getInstance()->setAudioFileWriterSynth(synth);
	sampleRate = synth->getSynthSampleRate();
	bufferSize = useBufferSize;
	outFileName = useOutFileName;
	realtimeMode = false;
	stopProcessing = false;
	delete[] buffer;
	buffer = new qint16[2 * bufferSize];
	QThread::start();
	return true;
}

void AudioFileWriter::startRealtimeProcessing(QSynth *useSynth, unsigned int useSampleRate, QString useOutFileName, unsigned int useBufferSize) {
	if (useOutFileName.isEmpty()) return;
	synth = useSynth;
	sampleRate = useSampleRate;
	bufferSize = useBufferSize;
	outFileName = useOutFileName;
	delete[] buffer;
	buffer = new qint16[2 * bufferSize];
	realtimeMode = true;
	stopProcessing = false;
	QThread::start();
}

void AudioFileWriter::stop() {
	stopProcessing = true;
	wait();
}

void AudioFileWriter::run() {
	QFile file(outFileName);
	bool waveMode = false;
	if (outFileName.endsWith(".wav")) waveMode = true;
	if (!file.open(QIODevice::WriteOnly)) {
		qDebug() << "AudioFileWriter: Can't open file for writing:" << outFileName;
		synth->close();
		if (!realtimeMode) {
			Master::getInstance()->setAudioFileWriterSynth(NULL);
		}
		return;
	}
	if (waveMode) file.seek(44);
	MasterClockNanos startNanos = MasterClock::getClockNanos();
	MasterClockNanos firstSampleNanos = 0;
	MasterClockNanos midiTick = 0;
	MasterClockNanos midiNanos = 0;
	QMidiEventList midiEvents;
	int midiEventIx = 0;
	uint parserIx = 0;
	bool skipSilence = false;
	if (realtimeMode) {
		firstSampleNanos = startNanos;
	} else {
		midiEvents = parsers[parserIx].getMIDIEvents();
		midiTick = parsers[parserIx].getMidiTick();
		skipSilence = true;
	}
	qDebug() << "AudioFileWriter: Rendering started";
	while (!stopProcessing) {
		uint frameCount = 0;
		if (realtimeMode) {
			frameCount = uint((sampleRate * (MasterClock::getClockNanos() - firstSampleNanos)) / MasterClock::NANOS_PER_SECOND);
			if (frameCount < bufferSize) {
				usleep(ulong((MasterClock::MICROS_PER_SECOND * (bufferSize - frameCount)) / sampleRate));
				continue;
			} else {
				frameCount = bufferSize;
			}
		} else {
			while (midiEventIx < midiEvents.count()) {
				const QMidiEvent &e = midiEvents.at(midiEventIx);
				bool eventPushed = true;
				MasterClockNanos nextEventNanos = midiNanos + e.getTimestamp() * midiTick;
				quint32 nextEventFrames = quint32(((double)sampleRate * nextEventNanos) / MasterClock::NANOS_PER_SECOND);
				frameCount = uint((sampleRate * (nextEventNanos - firstSampleNanos)) / MasterClock::NANOS_PER_SECOND);
				if (bufferSize < frameCount) {
					frameCount = bufferSize;
					break;
				}
				switch (e.getType()) {
					case SHORT_MESSAGE:
						eventPushed = synth->playMIDIShortMessage(e.getShortMessage(), nextEventFrames);
						break;
					case SYSEX:
						eventPushed = synth->playMIDISysex(e.getSysexData(), e.getSysexLen(), nextEventFrames);
						break;
					case SET_TEMPO:
						midiTick = parsers[parserIx].getMidiTick(e.getShortMessage());
						break;
					default:
						break;
				}
				if (!eventPushed) {
					qDebug() << "AudioFileWriter: MIDI buffer overflow, midiNanos:" << midiNanos;
					break;
				}
				midiNanos = nextEventNanos;
				midiEventIx++;
				emit midiEventProcessed(midiEventIx, midiEvents.count());
			}
			if (midiEvents.count() <= midiEventIx) {
				if (parserIx < parsersCount - 1) {
					++parserIx;
					midiEventIx = 0;
					midiEvents = parsers[parserIx].getMIDIEvents();
					midiTick = parsers[parserIx].getMidiTick();
					continue;
				}
				if (!synth->isActive() && frameCount == 0) break;
				frameCount += bufferSize;
				qDebug() << "AudioFileWriter: Rendering after the end of MIDI file, time:" << (double)midiNanos / MasterClock::NANOS_PER_SECOND;
			}
		}
		while (frameCount > 0) {
			uint framesToRender = qMin(bufferSize, frameCount);
			synth->render(buffer, framesToRender);
			qint64 bytesToWrite = framesToRender * FRAME_SIZE;
			char *bufferPos = (char *)buffer;
			if (skipSilence) {
				qint32 *startPos = (qint32 *)buffer;
				qint32 *endPos = startPos + framesToRender;
				bytesToWrite = 0;
				for (qint32 *p = startPos; p < endPos; p++) {
					if (*p != 0) {
						skipSilence = false;
						bufferPos = (char *)p;
						bytesToWrite = (char *)endPos - bufferPos;
						break;
					}
				}
			}
			while (bytesToWrite > 0) {
				qint64 bytesWritten = file.write(bufferPos, bytesToWrite);
				if (bytesWritten == -1) {
					qDebug() << "AudioFileWriter: error writing into the audio file:" << file.errorString();
					file.close();
					return;
				}
				bytesToWrite -= bytesWritten;
				bufferPos += bytesWritten;
			}
			firstSampleNanos += MasterClock::NANOS_PER_SECOND * framesToRender / sampleRate;
			frameCount -= framesToRender;
			if (!realtimeMode) qDebug() << "AudioFileWriter: Rendering time:" << (double)firstSampleNanos / MasterClock::NANOS_PER_SECOND;
		}
	}
	qDebug() << "AudioFileWriter: Rendering finished";
	if (!realtimeMode) qDebug() << "AudioFileWriter: Elapsed seconds: " << 1e-9 * (MasterClock::getClockNanos() - startNanos);
	if (waveMode) {
		unsigned char *charBuffer = (unsigned char *)buffer;
		memcpy(charBuffer, WAVE_HEADER, 44);
		quint32 fileSize = (quint32)file.size();
		qToLittleEndian(fileSize - 8, charBuffer + 4);
		qToLittleEndian(fileSize - 44, charBuffer + 40);
		qToLittleEndian(sampleRate, charBuffer + 24);
		qToLittleEndian(sampleRate * FRAME_SIZE, charBuffer + 28);
		file.seek(0);
		file.write((char *)charBuffer, 44);
	}
	file.close();
	synth->close();
	if (!realtimeMode) {
		Master::getInstance()->setAudioFileWriterSynth(NULL);
	}
	if (!stopProcessing) emit conversionFinished();
}
