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

#include <QFileDialog>

#include "AudioFileWriterDriver.h"
#include "../QSynth.h"
#include "../MasterClock.h"

static const unsigned int DEFAULT_AUDIO_LATENCY = 150;
static const unsigned int DEFAULT_MIDI_LATENCY = 200;

AudioFileWriterStream::AudioFileWriterStream(const AudioDriverSettings &useSettings, QSynth &useSynth, const quint32 useSampleRate) :
	AudioStream(useSettings, useSynth, useSampleRate) {}

bool AudioFileWriterStream::start() {
	static QString currentDir = NULL;
	QString fileName = QFileDialog::getSaveFileName(NULL, NULL, currentDir, "*.wav *.raw;;*.wav;;*.raw;;*.*");
	if (fileName.isEmpty()) return false;
	currentDir = QDir(fileName).absolutePath();
	timeInfo[0].lastPlayedNanos = MasterClock::getClockNanos();
	writer.startRealtimeProcessing(&synth, sampleRate, fileName, audioLatencyFrames);
	return true;
}

void AudioFileWriterStream::close() {
	writer.stop();
}

quint64 AudioFileWriterStream::estimateMIDITimestamp(const MasterClockNanos refNanos) {
	MasterClockNanos midiNanos = (refNanos == 0) ? MasterClock::getClockNanos() : refNanos;
	return quint64(((midiNanos - timeInfo[0].lastPlayedNanos) * sampleRate) / MasterClock::NANOS_PER_SECOND) + midiLatencyFrames;
}

AudioFileWriterDevice::AudioFileWriterDevice(AudioFileWriterDriver &driver, QString useDeviceName) :
	AudioDevice(driver, useDeviceName) {}

AudioStream *AudioFileWriterDevice::startAudioStream(QSynth &synth, const uint sampleRate) const {
	AudioFileWriterStream *stream = new AudioFileWriterStream(driver.getAudioSettings(), synth, sampleRate);
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
