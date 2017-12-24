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

#include <QMessageBox>

#include "AudioFileWriter.h"
#include "MasterClock.h"
#include "Master.h"
#include "MidiParser.h"
#include "QSynth.h"

static const unsigned int FRAME_SIZE = 4; // Stereo, 16-bit
static const unsigned char WAVE_HEADER[] = {
	0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
	0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x44, 0xAC, 0x00, 0x00, 0x10, 0xB1, 0x02, 0x00,
	0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00, 0x00
};
static const unsigned int RIFF_PAYLOAD_SIZE_OFFSET = 4;
static const unsigned int RIFF_HEADER_LENGTH = 8;
static const unsigned int WAVE_SAMPLE_RATE_OFFSET = 24;
static const unsigned int WAVE_BYTE_RATE_OFFSET = 28;
static const unsigned int WAVE_DATA_SIZE_OFFSET = 40;
static const unsigned int WAVE_HEADER_LENGTH = 44;

bool AudioFileWriter::convertSamplesFromNativeEndian(const qint16 *sourceBuffer, qint16 *targetBuffer, uint sampleCount, QSysInfo::Endian targetByteOrder) {
	if (QSysInfo::ByteOrder == targetByteOrder) return false;
	while ((sampleCount--) > 0) {
		*(targetBuffer++) = qbswap<>(*(sourceBuffer++));
	}
	return true;
}

AudioFileWriter::AudioFileWriter(uint sampleRate, const QString &fileName) :
	sampleRate(sampleRate), fileName(fileName), waveMode(fileName.endsWith(".wav")), file(fileName)
{}

AudioFileWriter::~AudioFileWriter() {
	if (file.isOpen()) close();
}

bool AudioFileWriter::open(bool skipInitialSilence) {
	if (!file.open(QIODevice::WriteOnly)) {
		qDebug() << "AudioFileWriter: Can't open file '" + fileName + "' for writing:" << file.errorString();
		return false;
	}
	if (waveMode) file.seek(WAVE_HEADER_LENGTH);
	skipSilence = skipInitialSilence;
	return true;
}

// Writes samples in native byte order
bool AudioFileWriter::write(const qint16 *buffer, uint totalFrames) {
	static const uint MAX_FRAMES_PER_RUN = 4096;

	if (!file.isOpen()) return false;

	const qint32 *startPos = (const qint32 *)buffer;
	if (skipSilence) {
		const qint32 *endPos = startPos + totalFrames;
		totalFrames = 0;
		for (const qint32 *p = startPos; p < endPos; p++) {
			if (*p != 0) {
				skipSilence = false;
				totalFrames = endPos - p;
				buffer = (const qint16 *)p;
				break;
			}
		}
	}

	qint16 cnvBuffer[MAX_FRAMES_PER_RUN << 1];
	while (totalFrames > 0) {
		uint framesToWrite = qMin(MAX_FRAMES_PER_RUN, totalFrames);
		bool converted = convertSamplesFromNativeEndian(buffer, cnvBuffer, framesToWrite << 1, waveMode ? QSysInfo::LittleEndian : QSysInfo::BigEndian);
		if (!converted) framesToWrite = totalFrames;

		const char *bufferPos = (const char *)(converted ? cnvBuffer : buffer);
		qint64 bytesToWrite = framesToWrite * FRAME_SIZE;
		while (bytesToWrite > 0) {
			qint64 bytesWritten = file.write(bufferPos, bytesToWrite);
			if (bytesWritten == -1) {
				qDebug() << "AudioFileWriter: error writing into the audio file:" << file.errorString();
				file.close();
				return false;
			}
			bytesToWrite -= bytesWritten;
			bufferPos += bytesWritten;
		}
		buffer += framesToWrite << 1;
		totalFrames -= framesToWrite;
	}
	return true;
}

void AudioFileWriter::close() {
	if (waveMode) {
		uchar headerBuffer[WAVE_HEADER_LENGTH];
		quint32 fileSize = (quint32)file.size();
		memcpy(headerBuffer, WAVE_HEADER, WAVE_HEADER_LENGTH);
		qToLittleEndian(fileSize - RIFF_HEADER_LENGTH, headerBuffer + RIFF_PAYLOAD_SIZE_OFFSET);
		qToLittleEndian(fileSize - WAVE_HEADER_LENGTH, headerBuffer + WAVE_DATA_SIZE_OFFSET);
		qToLittleEndian(sampleRate, headerBuffer + WAVE_SAMPLE_RATE_OFFSET);
		qToLittleEndian(sampleRate * FRAME_SIZE, headerBuffer + WAVE_BYTE_RATE_OFFSET);
		file.seek(0);
		file.write((char *)headerBuffer, WAVE_HEADER_LENGTH);
	}
	file.close();
}

AudioFileRenderer::AudioFileRenderer() : synth(NULL), buffer(NULL), parsers(NULL) {
	connect(this, SIGNAL(parsingFailed(const QString &, const QString &)), Master::getInstance(), SLOT(showBalloon(const QString &, const QString &)));
}

AudioFileRenderer::~AudioFileRenderer() {
	stop();
	if (!realtimeMode && synth != NULL) {
		synth->close();
		delete synth;
	}
	delete[] parsers;
	delete[] buffer;
}

bool AudioFileRenderer::convertMIDIFiles(QString useOutFileName, QStringList midiFileNameList, QString synthProfileName, unsigned int useBufferSize) {
	if (useOutFileName.isEmpty() || midiFileNameList.isEmpty()) return false;
	delete[] parsers;
	parsersCount = midiFileNameList.size();
	parsers = new MidiParser[parsersCount];
	for (uint i = 0; i < parsersCount; i++) {
		if (!parsers[i].parse(midiFileNameList.at(i))) {
			qDebug() << "AudioFileRenderer: Error parsing MIDI files";
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
	sampleRate = 0;
	if (!synth->open(sampleRate, MT32Emu::SamplerateConversionQuality_BEST, synthProfileName)) {
		synth->close();
		delete synth;
		synth = NULL;
		delete[] parsers;
		parsers = NULL;
		qDebug() << "AudioFileRenderer: Can't open synth";
		QMessageBox::critical(NULL, "Error", "Failed to open synth");
		return false;
	}
	Master::getInstance()->setAudioFileWriterSynth(synth);
	bufferSize = useBufferSize;
	outFileName = useOutFileName;
	realtimeMode = false;
	stopProcessing = false;
	delete[] buffer;
	buffer = new qint16[2 * bufferSize];
	QThread::start();
	return true;
}

void AudioFileRenderer::startRealtimeProcessing(QSynth *useSynth, unsigned int useSampleRate, QString useOutFileName, unsigned int useBufferSize) {
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

void AudioFileRenderer::stop() {
	stopProcessing = true;
	wait();
}

void AudioFileRenderer::run() {
	AudioFileWriter writer(sampleRate, outFileName);
	if (!writer.open(!realtimeMode)) {
		synth->close();
		if (!realtimeMode) Master::getInstance()->setAudioFileWriterSynth(NULL);
		emit conversionFinished();
		return;
	}
	MasterClockNanos startNanos = MasterClock::getClockNanos();
	MasterClockNanos firstSampleNanos = 0;
	MasterClockNanos midiTick = 0;
	MasterClockNanos midiNanos = 0;
	QMidiEventList midiEvents;
	int midiEventIx = 0;
	uint parserIx = 0;
	if (realtimeMode) {
		firstSampleNanos = startNanos;
	} else {
		midiEvents = parsers[parserIx].getMIDIEvents();
		midiTick = parsers[parserIx].getMidiTick();
	}
	qDebug() << "AudioFileRenderer: Rendering started";
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
					qDebug() << "AudioFileRenderer: MIDI buffer overflow, midiNanos:" << midiNanos;
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
				qDebug() << "AudioFileRenderer: Rendering after the end of MIDI file, time:" << (double)midiNanos / MasterClock::NANOS_PER_SECOND;
			}
		}
		while (frameCount > 0) {
			uint framesToRender = qMin(bufferSize, frameCount);
			synth->render(buffer, framesToRender);
			if (!writer.write(buffer, framesToRender)) {
				synth->close();
				if (!realtimeMode) Master::getInstance()->setAudioFileWriterSynth(NULL);
				emit conversionFinished();
				return;
			}
			firstSampleNanos += MasterClock::NANOS_PER_SECOND * framesToRender / sampleRate;
			frameCount -= framesToRender;
			if (!realtimeMode) qDebug() << "AudioFileWriter: Rendering time:" << (double)firstSampleNanos / MasterClock::NANOS_PER_SECOND;
		}
	}
	qDebug() << "AudioFileRenderer: Rendering finished";
	if (!realtimeMode) qDebug() << "AudioFileRenderer: Elapsed seconds: " << 1e-9 * (MasterClock::getClockNanos() - startNanos);
	writer.close();
	synth->close();
	if (!realtimeMode) Master::getInstance()->setAudioFileWriterSynth(NULL);
	if (!stopProcessing) emit conversionFinished();
}
