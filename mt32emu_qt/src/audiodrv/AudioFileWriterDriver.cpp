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

#include <QFileDialog>

#include "AudioFileWriterDriver.h"
#include "../QSynth.h"
#include "../MasterClock.h"

static const unsigned int DEFAULT_AUDIO_LATENCY = 150;
static const unsigned int DEFAULT_MIDI_LATENCY = 50;

AudioFileWriterStream::AudioFileWriterStream(const AudioFileWriterDevice *device, QSynth *useSynth, unsigned int useSampleRate)	{
	synth = useSynth;
	sampleRate = useSampleRate;
	const AudioDriverSettings driveSettings = device->driver->getAudioSettings();
	bufferSize = sampleRate * driveSettings.audioLatency / 1000;
	latency = MasterClock::NANOS_PER_MILLISECOND * driveSettings.midiLatency;
}

bool AudioFileWriterStream::start() {
	static QString currentDir = NULL;
	QString fileName = QFileDialog::getSaveFileName(NULL, NULL, currentDir, "*.wav *.raw;;*.wav;;*.raw;;*.*");
	if (fileName.isEmpty()) return false;
	currentDir = QDir(fileName).absolutePath();
	writer.startRealtimeProcessing(synth, sampleRate, fileName, bufferSize, latency);
	return true;
}

void AudioFileWriterStream::close() {
	writer.stop();
}

AudioFileWriterDevice::AudioFileWriterDevice(AudioFileWriterDriver * const driver, QString useDeviceIndex, QString useDeviceName) :
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

const QList<const AudioDevice *> AudioFileWriterDriver::createDeviceList() {
	QList<const AudioDevice *> deviceList;
	deviceList.append(new AudioFileWriterDevice(this, "fileWriter", "Audio file writer"));
	return deviceList;
}

void AudioFileWriterDriver::validateAudioSettings(AudioDriverSettings &settings) const {
	if (settings.midiLatency == 0) {
		settings.midiLatency = DEFAULT_MIDI_LATENCY;
	}
	if (settings.audioLatency == 0) {
		settings.audioLatency = DEFAULT_AUDIO_LATENCY;
	}
	if (settings.chunkLen > settings.audioLatency) {
		settings.chunkLen = settings.audioLatency;
	}
	settings.chunkLen = 0;
	settings.advancedTiming = true;
}
