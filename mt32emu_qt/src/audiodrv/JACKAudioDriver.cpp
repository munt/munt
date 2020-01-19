/* Copyright (C) 2011-2019 Jerome Fisher, Sergey V. Mikayev
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

#include "JACKAudioDriver.h"

#include "../Master.h"
#include "../QSynth.h"
#include "../JACKClient.h"

static const uint CHANNEL_COUNT = 2;
static const uint MINIMUM_JACK_BUFFER_COUNT = 2;

JACKAudioStream::JACKAudioStream(const AudioDriverSettings &useSettings, QSynth &useSynth, const quint32 useSampleRate, JACKClient *useJACKClient) :
	AudioStream(useSettings, useSynth, useSampleRate),
	jackClient(useJACKClient),
	buffer(new float[CHANNEL_COUNT * MT32Emu::MAX_SAMPLES_PER_RUN])
{}

JACKAudioStream::~JACKAudioStream() {
	stop();
	delete[] buffer;
}

bool JACKAudioStream::start(MidiSession *midiSession) {
	JACKClientState state = jackClient->open(midiSession, this);
	if (JACKClientState_OPEN != state) {
		qDebug() << "JACKAudioDriver: Failed to open JACK client connection";
		return false;
	}
	jackClient->connectToPhysicalPorts();

	// Rendering is synchronous, zero additional latency introduced
	audioLatencyFrames = 0;
	quint32 jackBufferSize = jackClient->getBufferSize();
	qDebug() << "JACKAudioDriver: JACK reported initial audio buffer size (s):" << double(jackBufferSize) / sampleRate;

	if (midiSession == NULL) {
		// Setup initial MIDI latency
		if (isAutoLatencyMode()) midiLatencyFrames = MINIMUM_JACK_BUFFER_COUNT * jackBufferSize;
		qDebug() << "JACKAudioDriver: Configured MIDI latency (s):" << double(midiLatencyFrames) / sampleRate;
	} else {
		// MIDI processing is synchronous, zero latency introduced
		midiLatencyFrames = 0;
		qDebug() << "JACKAudioDriver: Configured synchronous MIDI processing";
	}

	return true;
}

void JACKAudioStream::stop() {
	qDebug() << "JACKAudioDriver: Stopping JACK client";
	jackClient->close();
	qDebug() << "JACKAudioDriver: JACK client stopped";
}

void JACKAudioStream::onJACKShutdown() {
	qDebug() << "JACKAudioDriver: JACK server is shutting down, closing synth";
	synth.close();
}

void JACKAudioStream::renderStreams(const quint32 totalFrameCount, JACKAudioSample *leftOutBuffer, JACKAudioSample *rightOutBuffer) {
	// Only bother with updating TimeInfo when MIDI processing is asynchronous
	if (midiLatencyFrames != 0) {
		quint32 framesInAudioBuffer;
		if (settings.advancedTiming) {
			framesInAudioBuffer = qMin(0U, jackClient->getBufferSize() - jackClient->getFramesSinceCycleStart());
		} else {
			framesInAudioBuffer = 0U;
		}
		updateTimeInfo(MasterClock::getClockNanos(), framesInAudioBuffer);
	}
	for (quint32 frameCount = totalFrameCount; frameCount > 0;) {
		const uint framesToRender = qMin(frameCount, MT32Emu::MAX_SAMPLES_PER_RUN);
		synth.render(buffer, framesToRender);
		float *bufferPtr = buffer;
		float *bufferEnd = bufferPtr + CHANNEL_COUNT * framesToRender;
		while (bufferPtr < bufferEnd) {
			*(leftOutBuffer++) = JACKAudioSample(*(bufferPtr++));
			*(rightOutBuffer++) = JACKAudioSample(*(bufferPtr++));
		}
		frameCount -= framesToRender;
	}
	renderedFramesCount += totalFrameCount;
}

quint64 JACKAudioStream::computeMIDITimestamp(const quint32 jackBufferFrameTime) const {
	return renderedFramesCount + jackBufferFrameTime;
}

JACKAudioDefaultDevice::JACKAudioDefaultDevice(JACKAudioDriver &useDriver) :
	AudioDevice(useDriver, "Default")
{}

AudioStream *JACKAudioDefaultDevice::startAudioStream(QSynth &synth, const uint sampleRate) const {
	return startAudioStream(this, synth, sampleRate, NULL);
}

AudioStream *JACKAudioDefaultDevice::startAudioStream(const AudioDevice *audioDevice, QSynth &synth, const uint sampleRate, MidiSession *midiSession) {
	JACKClient *jackClient = new JACKClient;
	JACKAudioStream *stream = new JACKAudioStream(audioDevice->driver.getAudioSettings(), synth, sampleRate, jackClient);
	if (stream->start(midiSession)) return stream;
	delete stream;
	return NULL;
}

JACKAudioDriver::JACKAudioDriver(Master *useMaster) : AudioDriver("jackaudio", "JACKAudio") {
	Q_UNUSED(useMaster)

	loadAudioSettings();
}

const QList<const AudioDevice *> JACKAudioDriver::createDeviceList() {
	QList<const AudioDevice *> deviceList;
	deviceList.append(new JACKAudioDefaultDevice(*this));
	return deviceList;
}

void JACKAudioDriver::validateAudioSettings(AudioDriverSettings &newSettings) const {
	newSettings.chunkLen = 0;
	newSettings.audioLatency = 0;
}
