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

#include <pthread.h>

#include "AlsaAudioDriver.h"

#include "../MasterClock.h"
#include "../QSynth.h"

using namespace MT32Emu;

static const unsigned int FRAME_SIZE = 4; // Stereo, 16-bit
static const unsigned int FRAMES_IN_BUFFER = 320;
static const unsigned int audioLatency = 256000; // usec
// Latency for MIDI processing
static const MasterClockNanos latency = 256 * MasterClock::NANOS_PER_MILLISECOND;

#define USE_ALSA_TIMING

AlsaAudioStream::AlsaAudioStream(QSynth *useSynth, unsigned int useSampleRate) : synth(useSynth), sampleRate(useSampleRate), stream(NULL), sampleCount(0), pendingClose(false) {
	buffer = new Bit16s[2 * FRAMES_IN_BUFFER];
}

AlsaAudioStream::~AlsaAudioStream() {
	if (stream != NULL) {
		close();
	}
	delete[] buffer;
}

void* AlsaAudioStream::processingThread(void *userData) {
	int error;
	AlsaAudioStream *driver = (AlsaAudioStream *)userData;
	qDebug() << "Processing thread started";
	while (!driver->pendingClose) {
#ifdef USE_ALSA_TIMING
		snd_pcm_sframes_t delayp;
		if ((error = snd_pcm_delay(driver->stream, &delayp)) < 0) {
			qDebug() << "snd_pcm_delay failed:" << snd_strerror(error);
			break;
		}
		double realSampleRate = driver->sampleRate;
		MasterClockNanos realSampleTime = MasterClock::getClockNanos() + delayp / realSampleRate * MasterClock::NANOS_PER_SECOND;
		MasterClockNanos firstSampleNanos = realSampleTime - latency - audioLatency * MasterClock::NANOS_PER_MICROSECOND; // MIDI latency + total stream latency
#else
		double realSampleRate = driver->sampleRate / driver->clockSync.getDrift();
		MasterClockNanos realSampleTime = MasterClockNanos(driver->sampleCount / (double)driver->sampleRate * MasterClock::NANOS_PER_SECOND);
		MasterClockNanos firstSampleNanos = driver->clockSync.sync(realSampleTime) - latency; // MIDI latency only
#endif
		driver->synth->render(driver->buffer, FRAMES_IN_BUFFER,
			firstSampleNanos, realSampleRate);
		if ((error = snd_pcm_writei(driver->stream, driver->buffer, FRAMES_IN_BUFFER)) < 0) {
			qDebug() << "snd_pcm_writei failed:" << snd_strerror(error);
			break;
		}
		if (error != (int)FRAMES_IN_BUFFER) {
			qDebug() << "snd_pcm_writei failed. Written frames:" << error;
			break;
		}
		driver->sampleCount += FRAMES_IN_BUFFER;
	}
	driver->pendingClose = false;
	return NULL;
}

QList<QString> AlsaAudioStream::getDeviceNames() {
	QList<QString> deviceNames;
	deviceNames << "ALSA default audio device";
	return deviceNames;
}

bool AlsaAudioStream::start() {
	int error;
	if (buffer == NULL) {
		return false;
	}
	memset(buffer, 0, FRAME_SIZE * FRAMES_IN_BUFFER);
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
	if ((error = snd_pcm_set_params(stream, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
			/* channels */ 2, sampleRate, /* allow resampling */ 1, audioLatency)) < 0) {
		qDebug() << "snd_pcm_set_params failed:" << snd_strerror(error);
		snd_pcm_close(stream);
		stream = NULL;
		return false;
	}

	qDebug() << "snd_pcm_avail:" << snd_pcm_avail(stream);

	// Start playing to fill audio buffers
//	int initFrames = audioLatency * 1e-6 * sampleRate;
	int initFrames = sampleRate; // 1 sec seems to be enough
	while (initFrames > 0) {
		if ((error = snd_pcm_writei(stream, buffer, FRAMES_IN_BUFFER)) < 0) {
			qDebug() << "snd_pcm_writei failed:" << snd_strerror(error);
			snd_pcm_close(stream);
			stream = NULL;
			return false;
		}
		if (error != (int)FRAMES_IN_BUFFER) {
			qDebug() << "snd_pcm_writei failed. Written frames:" << error;
			snd_pcm_close(stream);
			stream = NULL;
			return NULL;
		}
		initFrames -= FRAMES_IN_BUFFER;
	}
	pthread_t threadID;
	if((error = pthread_create(&threadID, NULL, processingThread, this))) {
		qDebug() << "Processing Thread creation failed:" << error;
	}
	return true;
}

void AlsaAudioStream::close() {
	int error;
	if (stream != NULL) {
		pendingClose = true;
		qDebug() << "Stopping processing thread...";
		while (pendingClose) {
			sleep(1);
		}
		qDebug() << "Processing thread stopped";
		error = snd_pcm_close(stream);
		stream = NULL;
		if (error != 0) {
			qDebug() << "snd_pcm_close failed:" << snd_strerror(error);
		}
	}
	return;
}

AlsaAudioDefaultDevice::AlsaAudioDefaultDevice(AlsaAudioDriver *driver) : AudioDevice(driver, "default", "Default") {
}

AlsaAudioStream *AlsaAudioDefaultDevice::startAudioStream(QSynth *synth, unsigned int sampleRate) {
	AlsaAudioStream *stream = new AlsaAudioStream(synth, sampleRate);
	if (stream->start()) {
		return stream;
	}
	delete stream;
	return NULL;
}

AlsaAudioDriver::AlsaAudioDriver(Master *master) : AudioDriver("alsa", "ALSA") {
	Q_UNUSED(master);
}

AlsaAudioDriver::~AlsaAudioDriver() {
}

QList<AudioDevice *> AlsaAudioDriver::getDeviceList() {
	QList<AudioDevice *> deviceList;
	deviceList.append(new AlsaAudioDefaultDevice(this));
	return deviceList;
}
