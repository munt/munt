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

#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>

#include <pulse/error.h>

#include "PulseAudioDriver.h"

#include "../Master.h"
#include "../MasterClock.h"
#include "../SynthRoute.h"

using namespace MT32Emu;

static const int FRAME_SIZE = 4; // Stereo, 16-bit
static const unsigned int DEFAULT_CHUNK_MS = 10;
static const unsigned int DEFAULT_AUDIO_LATENCY = 40;
static const unsigned int DEFAULT_MIDI_LATENCY = 20;

#ifdef USE_PULSEAUDIO_DYNAMIC_LOADING

static const char PA_LIB_NAME[] = "libpulse-simple.so"; // PulseAudio library filename
static const char PA_LIB_NAME_MAJOR_VERSION[] = "libpulse-simple.so.0"; // PulseAudio library filename with major version appended

// Pointers for PulseAudio simple functions
	static __typeof__ (pa_simple_new) *_pa_simple_new = NULL;
	static __typeof__ (pa_simple_free) *_pa_simple_free = NULL;
	static __typeof__ (pa_simple_write) *_pa_simple_write = NULL;
	static __typeof__ (pa_simple_drain) *_pa_simple_drain = NULL;
	static __typeof__ (pa_simple_get_latency) *_pa_simple_get_latency = NULL;
	static __typeof__ (pa_strerror) *_pa_strerror = NULL;
#else
	static __typeof__ (pa_simple_new) *_pa_simple_new = pa_simple_new;
	static __typeof__ (pa_simple_free) *_pa_simple_free = pa_simple_free;
	static __typeof__ (pa_simple_write) *_pa_simple_write = pa_simple_write;
	static __typeof__ (pa_simple_drain) *_pa_simple_drain = pa_simple_drain;
	static __typeof__ (pa_simple_get_latency) *_pa_simple_get_latency = pa_simple_get_latency;
	static __typeof__ (pa_strerror) *_pa_strerror = pa_strerror;
#endif

static bool loadLibrary(bool loadNeeded) {
#ifdef USE_PULSEAUDIO_DYNAMIC_LOADING
	static void *dlHandle = NULL;
	char *error;

	if (!loadNeeded) {
		if (dlHandle) {
			if (dlclose(dlHandle) == 0) {
				dlHandle = NULL;
				_pa_simple_new = NULL;
				_pa_simple_free = NULL;
				_pa_simple_write = NULL;
				_pa_simple_drain = NULL;
				_pa_simple_get_latency = NULL;
				_pa_strerror = NULL;
			} else {
				qDebug() << "PulseAudio library unload failed:" << dlerror();
			}
		}
		return true;
	}

	// Loading the PulseAudio library dynamically
	if (dlHandle) {
		// Already loaded
		return true;
	}
	qDebug() << "Loading PulseAudio library";
	dlHandle = dlopen(PA_LIB_NAME_MAJOR_VERSION, RTLD_NOW);
	if (NULL == dlHandle) {
		qDebug() << "PulseAudio library failed to load:" << dlerror();
		qDebug() << "Trying versionless PulseAudio library name";
		dlHandle = dlopen(PA_LIB_NAME, RTLD_NOW);
		if (NULL == dlHandle) {
			qDebug() << "PulseAudio library failed to load:" << dlerror();
			return false;
		}
	}

	// Getting function pointers
	_pa_simple_new = (__typeof__(_pa_simple_new)) dlsym(dlHandle, "pa_simple_new");
	error = dlerror();
	if (!error) {
		_pa_simple_free = (__typeof__(_pa_simple_free)) dlsym(dlHandle, "pa_simple_free");
		error = dlerror();
	}
	if (!error) {
		_pa_simple_write = (__typeof__(_pa_simple_write)) dlsym(dlHandle, "pa_simple_write");
		error = dlerror();
	}
	if (!error) {
		_pa_simple_drain = (__typeof__(_pa_simple_drain)) dlsym(dlHandle, "pa_simple_drain");
		error = dlerror();
	}
	if (!error) {
		_pa_simple_get_latency = (__typeof__(_pa_simple_get_latency)) dlsym(dlHandle, "pa_simple_get_latency");
		error = dlerror();
	}
	if (!error) {
		_pa_strerror = (__typeof__(_pa_strerror)) dlsym(dlHandle, "pa_strerror");
		error = dlerror();
	}
	if (error) {
		qDebug() << "Failed to get addresses of PulseAudio library functions, dlsym() returned:" << error;
		loadLibrary(false);
		return false;
	}
#else
	(void)loadNeeded;
#endif
	return true;
}

PulseAudioStream::PulseAudioStream(const AudioDriverSettings &useSettings, SynthRoute &useSynthRoute, const quint32 useSampleRate) :
	AudioStream(useSettings, useSynthRoute, useSampleRate), stream(NULL), processingThreadID(0), stopProcessing(false)
{
	bufferSize = settings.chunkLen * sampleRate / MasterClock::MILLIS_PER_SECOND;
	buffer = new Bit16s[/* channels */ 2 * bufferSize];
}

PulseAudioStream::~PulseAudioStream() {
	if (stream != NULL) close();
	delete[] buffer;
}

void *PulseAudioStream::processingThread(void *userData) {
	int error;
	PulseAudioStream &audioStream = *(PulseAudioStream *)userData;
	qDebug() << "PulseAudio: Processing thread started";
	while (!audioStream.stopProcessing) {
		MasterClockNanos nanosNow = MasterClock::getClockNanos();
		quint32 framesInAudioBuffer;
		if (audioStream.settings.advancedTiming) {
			framesInAudioBuffer = quint32(audioStream.sampleRate * ((double)_pa_simple_get_latency(audioStream.stream, &error) / MasterClock::MICROS_PER_SECOND));
		} else {
			framesInAudioBuffer = 0;
		}
		audioStream.renderAndUpdateState(audioStream.buffer, audioStream.bufferSize, nanosNow, framesInAudioBuffer);
		if (_pa_simple_write(audioStream.stream, audioStream.buffer, audioStream.bufferSize * FRAME_SIZE, &error) < 0) {
			qDebug() << "pa_simple_write() failed:" << _pa_strerror(error);
			_pa_simple_free(audioStream.stream);
			audioStream.stream = NULL;
			qDebug() << "PulseAudio: Processing thread stopped";
			audioStream.synthRoute.audioStreamFailed();
			audioStream.processingThreadID = 0;
			return NULL;
		}
	}
	audioStream.stopProcessing = false;
	audioStream.processingThreadID = 0;
	return NULL;
}

bool PulseAudioStream::start() {
	int error;
	if (buffer == NULL) return false;
	memset(buffer, 0, FRAME_SIZE * bufferSize);
	if (stream != NULL) close();

	qDebug() << "Using PulseAudio default device";

	// The Sample format to use
	static const pa_sample_spec ss = {
		PA_SAMPLE_S16NE, // format
		sampleRate,
		2 // channels
	};

	// Configuring desired audio latency
	qDebug() << "Using audio latency:" << settings.audioLatency;
	static const pa_buffer_attr ba = {
		audioLatencyFrames * FRAME_SIZE, // uint32_t maxlength;
		audioLatencyFrames * FRAME_SIZE, // uint32_t tlength;
		(uint32_t)-1, // uint32_t prebuf;
		(uint32_t)-1, // uint32_t minreq;
		(uint32_t)-1 // uint32_t fragsize;
	};

	// Create a new playback stream
	stream = _pa_simple_new(NULL, "mt32emu-qt", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, &ba, &error);
	if (stream == NULL) {
		qDebug() << "pa_simple_new() failed:" << _pa_strerror(error);
		return false;
	}

	// Setup initial MIDI latency
	if (isAutoLatencyMode()) midiLatencyFrames = audioLatencyFrames + ((DEFAULT_MIDI_LATENCY * sampleRate) / MasterClock::MILLIS_PER_SECOND);

	// Start playing to fill audio buffers
	int initFrames = audioLatencyFrames;
	while (initFrames > 0) {
		if (_pa_simple_write(stream, buffer, FRAME_SIZE * bufferSize, &error) < 0) {
			qDebug() << "pa_simple_write() failed:" << _pa_strerror(error);
			_pa_simple_free(stream);
			stream = NULL;
			return false;
		}
		initFrames -= bufferSize;
	}
	error = pthread_create(&processingThreadID, NULL, processingThread, this);
	if (error != 0) {
		processingThreadID = 0;
		qDebug() << "PulseAudio: Processing Thread creation failed:" << error;
		_pa_simple_free(stream);
		stream = NULL;
		return false;
	}
	return true;
}

void PulseAudioStream::close() {
	int error;
	if (stream != NULL) {
		if (processingThreadID != 0) {
			qDebug() << "PulseAudio: Stopping processing thread...";
			stopProcessing = true;
			pthread_join(processingThreadID, NULL);
			stopProcessing = false;
			processingThreadID = 0;
			qDebug() << "PulseAudio: Processing thread stopped";
		}
		if (_pa_simple_drain(stream, &error) < 0) {
			qDebug() << "pa_simple_drain() failed:" << _pa_strerror(error);
		}
		_pa_simple_free(stream);
		stream = NULL;
	}
	return;
}

PulseAudioDefaultDevice::PulseAudioDefaultDevice(PulseAudioDriver &driver) : AudioDevice(driver, "Default") {}

AudioStream *PulseAudioDefaultDevice::startAudioStream(SynthRoute &synthRoute, const uint sampleRate) const {
	PulseAudioStream *stream = new PulseAudioStream(driver.getAudioSettings(), synthRoute, sampleRate);
	if (stream->start()) return stream;
	delete stream;
	return NULL;
}

PulseAudioDriver::PulseAudioDriver(Master *master) : AudioDriver("pulseaudio", "PulseAudio") {
	Q_UNUSED(master);

	isLibraryFound = loadLibrary(true);
	loadAudioSettings();
}

PulseAudioDriver::~PulseAudioDriver() {
	if (isLibraryFound) {
		loadLibrary(false);
	}
}

const QList<const AudioDevice *> PulseAudioDriver::createDeviceList() {
	QList<const AudioDevice *> deviceList;
	if (isLibraryFound) {
		deviceList.append(new PulseAudioDefaultDevice(*this));
	}
	return deviceList;
}

void PulseAudioDriver::validateAudioSettings(AudioDriverSettings &settings) const {
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
