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

#include <QFileDialog>

#include "AudioFileWriter.h"
#include "../QSynth.h"
#include "../MasterClock.h"

using namespace MT32Emu;

static const unsigned int DEFAULT_CHUNK_MS = 30;
static const unsigned int DEFAULT_AUDIO_LATENCY = 150;
static const unsigned int DEFAULT_MIDI_LATENCY = 50;
static const unsigned int FRAME_SIZE = 4;              // Stereo, 16-bit
static const unsigned char WAVE_HEADER[] = {
	0x52, 0x49, 0x46, 0x46, 0x00, 0x00, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74, 0x20,
	0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x44, 0xAC, 0x00, 0x00, 0x10, 0xB1, 0x02, 0x00,
	0x04, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x00, 0x00, 0x00, 0x00
};

class AudioFileWriter : public QThread {
private:
	unsigned int chunkSize;
	unsigned int bufferSize;
	MasterClockNanos latency;
	QSynth *synth;
	const unsigned int sampleRate;
	Bit16s *buffer;
	QString fileName;
	bool volatile pendingClose;

public:
	AudioFileWriter(const AudioFileWriterDevice *device, QSynth *useSynth, unsigned int useSampleRate) :
		synth(useSynth), sampleRate(useSampleRate), pendingClose(false)
	{
		unsigned int midiLatency;
		bool unused;

		device->driver->getAudioSettings(&chunkSize, &bufferSize, &midiLatency, &unused);
		chunkSize *= sampleRate / 1000.0 /* ms per sec */;
		bufferSize *= sampleRate / 1000.0 /* ms per sec */;
		latency = midiLatency * MasterClock::NANOS_PER_MILLISECOND;
		buffer = new Bit16s[2 * bufferSize];
	}

	~AudioFileWriter() {
		stop();
		delete[] buffer;
	}

	void setFileName(QString useFileName) {
		fileName = useFileName;
	}

	void run() {
		if (fileName == NULL || fileName.isEmpty()) return;
		QFile file(fileName);
		bool waveMode = false;
		if (fileName.endsWith(".wav")) waveMode = true;
		file.open(QIODevice::WriteOnly);
		if (waveMode) file.seek(44);
		MasterClockNanos firstSampleNanos = MasterClock::getClockNanos();
		while (!pendingClose) {
			MasterClockNanos nanosNow = MasterClock::getClockNanos();
			unsigned int frameCount = sampleRate * (nanosNow - firstSampleNanos) / MasterClock::NANOS_PER_SECOND;
			if (frameCount < chunkSize) {
				MasterClock::sleepForNanos(MasterClock::NANOS_PER_SECOND * (chunkSize - frameCount) / sampleRate);
				continue;
			}
			while(frameCount > 0)
			{
				unsigned int framesToRender = bufferSize < frameCount ? bufferSize : frameCount;
				unsigned int framesRendered = synth->render(buffer, framesToRender, firstSampleNanos - latency, sampleRate);
				for (qint64 framesToWrite = framesRendered; framesToWrite > 0;) {
					qint64 framesWritten = file.write((char *)buffer, framesRendered * FRAME_SIZE);
					if (framesWritten == -1) {
						qDebug() << "AudioFileWriter: error writing into the audio file:" << file.errorString();
						file.close();
						return;
					}
					framesToWrite -= framesWritten;
				}
				firstSampleNanos += MasterClock::NANOS_PER_SECOND * framesRendered / sampleRate;
				frameCount -= framesRendered;
			}
		}
		if (waveMode) {
			unsigned char *charBuffer = (unsigned char *)buffer;
			memcpy(charBuffer, WAVE_HEADER, 44);
			unsigned long fileSize = (unsigned long)file.size();
			qToLittleEndian(fileSize - 8, charBuffer + 4);
			qToLittleEndian(fileSize - 44, charBuffer + 40);
			qToLittleEndian(sampleRate, charBuffer + 24);
			qToLittleEndian(sampleRate * FRAME_SIZE, charBuffer + 28);
			file.seek(0);
			file.write((char *)charBuffer, 44);
		}
		file.close();
	}

	void stop() {
		pendingClose = true;
		wait();
	}
};

AudioFileWriterStream::AudioFileWriterStream(const AudioFileWriterDevice *device, QSynth *useSynth, unsigned int useSampleRate)	{
	writer = new AudioFileWriter(device, useSynth, useSampleRate);
}

AudioFileWriterStream::~AudioFileWriterStream() {
	delete writer;
}

bool AudioFileWriterStream::start() {
	QString fileName = QFileDialog::getSaveFileName(NULL, NULL, NULL, "*.wav *.raw;;*.wav;;*.raw;;*.*");
	if (fileName.isEmpty()) return false;
	writer->setFileName(fileName);
	writer->start();
	return true;
}

void AudioFileWriterStream::close() {
	writer->stop();
}

AudioFileWriterDevice::AudioFileWriterDevice(const AudioFileWriterDriver * const driver, QString useDeviceIndex, QString useDeviceName) :
	AudioDevice(driver, useDeviceIndex, useDeviceName) {
}

AudioFileWriterStream *AudioFileWriterDevice::startAudioStream(QSynth *synth, unsigned int sampleRate) const {
	AudioFileWriterStream *stream = new AudioFileWriterStream(this, synth, sampleRate);
	if (stream->start()) {
		return stream;
	}
	delete stream;
	return NULL;
}

AudioFileWriterDriver::AudioFileWriterDriver(Master *master) : AudioDriver("fileWriter", "AudioFileWriter") {
	Q_UNUSED(master);

	loadAudioSettings();
}

AudioFileWriterDriver::~AudioFileWriterDriver() {
}

QList<AudioDevice *> AudioFileWriterDriver::getDeviceList() const {
	QList<AudioDevice *> deviceList;
	deviceList.append(new AudioFileWriterDevice(this, "fileWriter", "Audio file writer"));
	return deviceList;
}

void AudioFileWriterDriver::validateAudioSettings() {
	if (midiLatency == 0) {
		midiLatency = DEFAULT_MIDI_LATENCY;
	}
	if (audioLatency == 0) {
		audioLatency = DEFAULT_AUDIO_LATENCY;
	}
	if (chunkLen == 0) {
		chunkLen = DEFAULT_CHUNK_MS;
	}
	if (chunkLen > audioLatency) {
		chunkLen = audioLatency;
	}
	advancedTiming = true;
}
