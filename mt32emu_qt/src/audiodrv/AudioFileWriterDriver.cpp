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

#include <QFileDialog>

#include "AudioFileWriterDriver.h"

#include "../MasterClock.h"
#include "../Master.h"
#include "../SynthRoute.h"

static const unsigned int DEFAULT_AUDIO_LATENCY = 150;
static const unsigned int DEFAULT_MIDI_LATENCY = 200;

AudioFileWriterStream::AudioFileWriterStream(const AudioDriverSettings &useSettings, SynthRoute &useSynthRoute, const quint32 useSampleRate) :
	AudioStream(useSettings, useSynthRoute, useSampleRate) {}

bool AudioFileWriterStream::start() {
	static QString currentDir = NULL;
	QFileDialog::Options qFileDialogOptions = QFileDialog::Options(Master::getInstance()->getSettings()->value("Master/qFileDialogOptions", 0).toInt());
	QString fileName = QFileDialog::getSaveFileName(NULL, NULL, currentDir, "*.wav *.raw;;*.wav;;*.raw;;*.*",
		NULL, qFileDialogOptions);
	if (fileName.isEmpty()) return false;
	currentDir = QDir(fileName).absolutePath();
	timeInfos[0].lastPlayedNanos = MasterClock::getClockNanos();
	writer.startRealtimeProcessing(this, sampleRate, fileName, audioLatencyFrames);
	return true;
}

void AudioFileWriterStream::audioStreamFailed() {
	synthRoute.audioStreamFailed();
}

void AudioFileWriterStream::render(qint16 *buffer, uint frameCount) {
	// No need to update time info, we assume perfect timing.
	synthRoute.render(buffer, frameCount);
	framesRendered(frameCount);
}

quint64 AudioFileWriterStream::estimateMIDITimestamp(const MasterClockNanos midiNanos) {
	// We assume perfect timing with constant sample rate.
	return quint64(((midiNanos - timeInfos[0].lastPlayedNanos) * sampleRate) / MasterClock::NANOS_PER_SECOND) + midiLatencyFrames;
}

AudioFileWriterDevice::AudioFileWriterDevice(AudioFileWriterDriver &driver, QString useDeviceName) :
	AudioDevice(driver, useDeviceName) {}

AudioStream *AudioFileWriterDevice::startAudioStream(SynthRoute &synthRoute, const uint sampleRate) const {
	AudioFileWriterStream *stream = new AudioFileWriterStream(driver.getAudioSettings(), synthRoute, sampleRate);
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

const QList<const AudioDevice *> AudioFileWriterDriver::createDeviceList() {
	QList<const AudioDevice *> deviceList;
	deviceList.append(new AudioFileWriterDevice(*this, "Audio file writer"));
	return deviceList;
}

void AudioFileWriterDriver::validateAudioSettings(AudioDriverSettings &settings) const {
	if (settings.midiLatency == 0) {
		settings.midiLatency = DEFAULT_MIDI_LATENCY;
	}
	if (settings.audioLatency == 0) {
		settings.audioLatency = DEFAULT_AUDIO_LATENCY;
	}
	settings.chunkLen = 0;
	settings.advancedTiming = true;
}
