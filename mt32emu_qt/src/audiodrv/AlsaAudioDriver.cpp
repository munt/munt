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

#include <pthread.h>

#include "AlsaAudioDriver.h"

#include "../MasterClock.h"
#include "../QSynth.h"

using namespace MT32Emu;

static const unsigned int FRAME_SIZE = 4; // Stereo, 16-bit
static const unsigned int DEFAULT_CHUNK_MS = 16;
static const unsigned int DEFAULT_AUDIO_LATENCY = 64;
static const unsigned int DEFAULT_MIDI_LATENCY = 32;

AlsaAudioStream::AlsaAudioStream(const AudioDevice *device, QSynth *useSynth,
		unsigned int useSampleRate) : synth(useSynth), sampleRate(useSampleRate),
		stream(NULL), sampleCount(0), pendingClose(false)
{
	const AudioDriverSettings &driverSettings = device->driver->getAudioSettings();
	bufferSize = driverSettings.chunkLen * sampleRate / 1000 /* ms per sec*/;
	audioLatency = driverSettings.audioLatency;
	midiLatency = driverSettings.midiLatency;
	useAdvancedTiming = driverSettings.advancedTiming;
	buffer = new Bit16s[2 * bufferSize];
}

AlsaAudioStream::~AlsaAudioStream() {
	if (stream != NULL) {
		close();
	}
	delete[] buffer;
}

void* AlsaAudioStream::processingThread(void *userData) {
	int error;
	bool isErrorOccured = false;
	AlsaAudioStream *driver = (AlsaAudioStream *)userData;
	MasterClockNanos lastSampleNanos = MasterClock::getClockNanos() - (driver->audioLatency + driver->midiLatency) * MasterClock::NANOS_PER_MILLISECOND;
	qDebug() << "ALSA audio: Processing thread started";
	while (!driver->pendingClose) {
		double realSampleRate;
		MasterClockNanos realSampleTime;
		MasterClockNanos firstSampleNanos;
		if (driver->useAdvancedTiming) {
			snd_pcm_sframes_t delayp;
			if ((error = snd_pcm_delay(driver->stream, &delayp)) < 0) {
				qDebug() << "snd_pcm_delay failed:" << snd_strerror(error);
				isErrorOccured = true;
				break;
			}
			realSampleTime = MasterClock::getClockNanos() + MasterClock::NANOS_PER_SECOND * delayp / driver->sampleRate;
			firstSampleNanos = realSampleTime - (driver->midiLatency + driver->audioLatency)
				* MasterClock::NANOS_PER_MILLISECOND; // MIDI latency + total stream audio latency
			realSampleRate = AudioStream::estimateActualSampleRate(driver->sampleRate, firstSampleNanos, lastSampleNanos,
				driver->audioLatency * MasterClock::NANOS_PER_MILLISECOND, driver->bufferSize);
		} else {
			realSampleTime = MasterClockNanos(driver->sampleCount /
				(double)driver->sampleRate * MasterClock::NANOS_PER_SECOND);
			firstSampleNanos = driver->clockSync.sync(realSampleTime) -
				driver->midiLatency * MasterClock::NANOS_PER_MILLISECOND; // MIDI latency only
			realSampleRate = driver->sampleRate / driver->clockSync.getDrift();
		}
		driver->synth->render(driver->buffer, driver->bufferSize, firstSampleNanos, realSampleRate);
		if ((error = snd_pcm_writei(driver->stream, driver->buffer, driver->bufferSize)) < 0) {
			qDebug() << "snd_pcm_writei failed:" << snd_strerror(error);
			isErrorOccured = true;
			break;
		}
		if (error != (int)driver->bufferSize) {
			qDebug() << "snd_pcm_writei failed. Written frames:" << error;
			isErrorOccured = true;
			break;
		}
		driver->sampleCount += driver->bufferSize;
	}
	qDebug() << "ALSA audio: Processing thread stopped";
	if (isErrorOccured) {
		snd_pcm_close(driver->stream);
		driver->stream = NULL;
		driver->synth->close();
	} else {
		driver->pendingClose = false;
	}
	return NULL;
}

bool AlsaAudioStream::start() {
	int error;
	if (buffer == NULL) {
		return false;
	}
	memset(buffer, 0, FRAME_SIZE * bufferSize);
	if (stream != NULL) {
		close();
	}
	qDebug() << "Using ALSA default audio device";

	// Create a new playback stream
	if ((error = snd_pcm_open(&stream, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		qDebug() << "snd_pcm_open failed:" << snd_strerror(error);
		return false;
	}

	// Set Sample format to use
	if ((error = snd_pcm_set_params(stream, SND_PCM_FORMAT_S16_LE,
		SND_PCM_ACCESS_RW_INTERLEAVED, /* channels */ 2, sampleRate,
		/* allow resampling */ 1, audioLatency * 1000 /* ms to us*/)) < 0)
	{
		qDebug() << "snd_pcm_set_params failed:" << snd_strerror(error);
		snd_pcm_close(stream);
		stream = NULL;
		return false;
	}

	qDebug() << "snd_pcm_avail:" << snd_pcm_avail(stream);

	// Start playing to fill audio buffers
//	int initFrames = audioLatency * 1e-3 * sampleRate;
	int initFrames = sampleRate; // 1 sec seems to be enough
	while (initFrames > 0) {
		if ((error = snd_pcm_writei(stream, buffer, bufferSize)) < 0) {
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
	pthread_t threadID;
	if((error = pthread_create(&threadID, NULL, processingThread, this))) {
		qDebug() << "ALSA audio: Processing Thread creation failed:" << error;
	}
	return true;
}

void AlsaAudioStream::close() {
	int error;
	if (stream != NULL) {
		pendingClose = true;
		qDebug() << "ALSA audio: Stopping processing thread...";
		while (pendingClose) {
			sleep(1);
		}
		error = snd_pcm_close(stream);
		stream = NULL;
		if (error != 0) {
			qDebug() << "snd_pcm_close failed:" << snd_strerror(error);
		}
	}
	return;
}

AlsaAudioDefaultDevice::AlsaAudioDefaultDevice(AlsaAudioDriver * const driver) :
	AudioDevice(driver, "default", "Default")
{
}

AlsaAudioStream *AlsaAudioDefaultDevice::startAudioStream(QSynth *synth,
	unsigned int sampleRate) const
{
	AlsaAudioStream *stream = new AlsaAudioStream(this, synth, sampleRate);
	if (stream->start()) {
		return stream;
	}
	delete stream;
	return NULL;
}

AlsaAudioDriver::AlsaAudioDriver(Master *master) : AudioDriver("alsa", "ALSA") {
	Q_UNUSED(master);

	loadAudioSettings();
}

AlsaAudioDriver::~AlsaAudioDriver() {
}

const QList<const AudioDevice *> AlsaAudioDriver::createDeviceList() {
	QList<const AudioDevice *> deviceList;
	deviceList.append(new AlsaAudioDefaultDevice(this));
	return deviceList;
}

void AlsaAudioDriver::validateAudioSettings(AudioDriverSettings &settings) const {
	if (settings.midiLatency == 0) {
		settings.midiLatency = DEFAULT_MIDI_LATENCY;
	}
	if (settings.audioLatency == 0) {
		settings.audioLatency = DEFAULT_AUDIO_LATENCY;
	}
	if (settings.chunkLen == 0) {
		settings.chunkLen = DEFAULT_CHUNK_MS;
	}
	if (settings.chunkLen > settings.audioLatency) {
		settings.chunkLen = settings.audioLatency;
	}
}
