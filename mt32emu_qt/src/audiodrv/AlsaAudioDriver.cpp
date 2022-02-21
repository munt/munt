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

#include <pthread.h>

#include "AlsaAudioDriver.h"

#include "../MasterClock.h"
#include "../SynthRoute.h"

using namespace MT32Emu;

static const unsigned int FRAME_SIZE = 4; // Stereo, 16-bit
static const unsigned int DEFAULT_CHUNK_MS = 16;
static const unsigned int DEFAULT_AUDIO_LATENCY = 64;
static const unsigned int DEFAULT_MIDI_LATENCY = 32;

AlsaAudioStream::AlsaAudioStream(const AudioDriverSettings &useSettings, SynthRoute &useSynthRoute, const quint32 useSampleRate) :
  AudioStream(useSettings, useSynthRoute, useSampleRate), stream(NULL), processingThreadID(0), stopProcessing(false)
{
	bufferSize = settings.chunkLen * sampleRate / MasterClock::MILLIS_PER_SECOND;
	buffer = new Bit16s[/* channels */ 2 * bufferSize];
}

AlsaAudioStream::~AlsaAudioStream() {
	if (stream != NULL) close();
	delete[] buffer;
}

void *AlsaAudioStream::processingThread(void *userData) {
	int error;
	bool isErrorOccurred = false;
	AlsaAudioStream &audioStream = *(AlsaAudioStream *)userData;
	qDebug() << "ALSA audio: Processing thread started";
	while (!audioStream.stopProcessing) {
		MasterClockNanos nanosNow = MasterClock::getClockNanos();
		quint32 framesInAudioBuffer = 0;
		if (audioStream.settings.advancedTiming) {
			snd_pcm_sframes_t delayp;
			error = snd_pcm_delay(audioStream.stream, &delayp);
			if (error < 0) {
				qDebug() << "snd_pcm_delay failed:" << snd_strerror(error);
//				isErrorOccurred = true;
//				break;
			} else {
				framesInAudioBuffer = (quint32)delayp;
			}
		}
		audioStream.renderAndUpdateState(audioStream.buffer, audioStream.bufferSize, nanosNow, framesInAudioBuffer);
		error = snd_pcm_writei(audioStream.stream, audioStream.buffer, audioStream.bufferSize);
		if (error < 0) {
			qDebug() << "snd_pcm_writei failed:" << snd_strerror(error) << "-> recovering...";
			error = snd_pcm_recover(audioStream.stream, error, 0);
			if (error != 0) {
				qDebug() << "snd_pcm_recover failed:" << snd_strerror(error) << "-> closing...";
				isErrorOccurred = true;
				break;
			}
		} else if (error != (int)audioStream.bufferSize) {
			qDebug() << "snd_pcm_writei failed. Written frames:" << error;
//			isErrorOccurred = true;
//			break;
		}
	}
	if (isErrorOccurred) {
		snd_pcm_close(audioStream.stream);
		audioStream.stream = NULL;
		audioStream.synthRoute.audioStreamFailed();
	} else {
		audioStream.stopProcessing = false;
	}
	audioStream.processingThreadID = 0;
	return NULL;
}

bool AlsaAudioStream::start(const char *deviceID) {
	int error;
	if (buffer == NULL) return false;
	memset(buffer, 0, FRAME_SIZE * bufferSize);
	if (stream != NULL) close();

	qDebug() << "Using ALSA audio device:" << deviceID;

	// Create a new playback stream
	error = snd_pcm_open(&stream, deviceID, SND_PCM_STREAM_PLAYBACK, 0);
	if (error < 0) {
		qDebug() << "snd_pcm_open failed:" << snd_strerror(error);
		stream = NULL;
		return false;
	}

	// Set Sample format to use
	error = snd_pcm_set_params(stream, SND_PCM_FORMAT_S16, SND_PCM_ACCESS_RW_INTERLEAVED, /* channels */ 2,
	  sampleRate, /* allow resampling */ 1, settings.audioLatency * MasterClock::MICROS_PER_MILLISECOND);
	if (error < 0) {
		qDebug() << "snd_pcm_set_params failed:" << snd_strerror(error);
		snd_pcm_close(stream);
		stream = NULL;
		return false;
	}

	audioLatencyFrames = snd_pcm_avail(stream);

	if (audioLatencyFrames <= bufferSize) {
		bufferSize = audioLatencyFrames / 2;
	}

	qDebug() << "Using audio latency:" << audioLatencyFrames << "frames, chunk size:" << bufferSize << "frames";

	// Setup initial MIDI latency
	if (isAutoLatencyMode()) midiLatencyFrames = audioLatencyFrames + ((DEFAULT_MIDI_LATENCY * sampleRate) / MasterClock::MILLIS_PER_SECOND);

	// Start playing to fill audio buffers
	int initFrames = audioLatencyFrames;
	while (initFrames > 0) {
		error = snd_pcm_writei(stream, buffer, bufferSize);
		if (error < 0) {
			qDebug() << "snd_pcm_writei failed:" << snd_strerror(error);
			snd_pcm_close(stream);
			stream = NULL;
			return false;
		}
		if (error != (int)bufferSize) {
			qDebug() << "snd_pcm_writei failed. Written frames:" << error;
			snd_pcm_close(stream);
			stream = NULL;
			return false;
		}
		initFrames -= bufferSize;
	}
	error = pthread_create(&processingThreadID, NULL, processingThread, this);
	if (error != 0) {
		processingThreadID = 0;
		qDebug() << "ALSA audio: Processing Thread creation failed:" << error;
		snd_pcm_close(stream);
		stream = NULL;
		return false;
	}
	return true;
}

void AlsaAudioStream::close() {
	int error;
	if (stream != NULL) {
		if (processingThreadID != 0) {
			qDebug() << "ALSA audio: Stopping processing thread...";
			stopProcessing = true;
			pthread_join(processingThreadID, NULL);
			stopProcessing = false;
			processingThreadID = 0;
			qDebug() << "ALSA audio: Processing thread stopped";
		}
		error = snd_pcm_close(stream);
		stream = NULL;
		if (error != 0) {
			qDebug() << "snd_pcm_close failed:" << snd_strerror(error);
		}
	}
	return;
}

AlsaAudioDevice::AlsaAudioDevice(AlsaAudioDriver &driver, const char *useDeviceID, const QString name) : AudioDevice(driver, name), deviceID(useDeviceID) {}

AudioStream *AlsaAudioDevice::startAudioStream(SynthRoute &synthRoute, const uint sampleRate) const {
	AlsaAudioStream *stream = new AlsaAudioStream(driver.getAudioSettings(), synthRoute, sampleRate);
	if (stream->start(deviceID)) return stream;
	delete stream;
	return NULL;
}

AlsaAudioDriver::AlsaAudioDriver(Master *master) : AudioDriver("alsa", "ALSA") {
	Q_UNUSED(master);

	loadAudioSettings();
}

const QList<const AudioDevice *> AlsaAudioDriver::createDeviceList() {
	QList<const AudioDevice *> deviceList;
	deviceList.append(new AlsaAudioDevice(*this, "default", "Default"));
	deviceList.append(new AlsaAudioDevice(*this, "sysdefault", "System default"));
	deviceList.append(new AlsaAudioDevice(*this, "plug:hw", "Exclusive mode"));
	return deviceList;
}

void AlsaAudioDriver::validateAudioSettings(AudioDriverSettings &settings) const {
	if (settings.audioLatency == 0) {
		settings.audioLatency = DEFAULT_AUDIO_LATENCY;
	}
	if (settings.chunkLen == 0) {
		settings.chunkLen = DEFAULT_CHUNK_MS;
	}
	if (settings.chunkLen > settings.audioLatency) {
		settings.chunkLen = settings.audioLatency;
	}
	if ((settings.midiLatency != 0) && (settings.midiLatency < settings.chunkLen)) {
		settings.midiLatency = settings.chunkLen;
	}
}
