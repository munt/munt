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
#include <dlfcn.h>

#include <pulse/error.h>

#include "PulseAudioDriver.h"

#include "../Master.h"
#include "../MasterClock.h"
#include "../QSynth.h"

using namespace MT32Emu;

#define USE_PA_TIMING
#define USE_DYNAMIC_LOADING

static const int FRAME_SIZE = 4; // Stereo, 16-bit
static const unsigned int DEFAULT_CHUNK_MS = 10;
static const unsigned int DEFAULT_MIDI_LATENCY = 300;

static const char PA_LIB_NAME[] = "libpulse-simple.so"; // PulseAudio library filename

#ifdef USE_DYNAMIC_LOADING
// Pointers for PulseAudio simple functions
	static const __typeof__ (pa_simple_new) *_pa_simple_new = NULL;
	static const __typeof__ (pa_simple_free) *_pa_simple_free = NULL;
	static const __typeof__ (pa_simple_write) *_pa_simple_write = NULL;
	static const __typeof__ (pa_simple_drain) *_pa_simple_drain = NULL;
	static const __typeof__ (pa_simple_get_latency) *_pa_simple_get_latency = NULL;
	static const __typeof__ (pa_strerror) *_pa_strerror = NULL;
#else
	static const __typeof__ (pa_simple_new) *_pa_simple_new = pa_simple_new;
	static const __typeof__ (pa_simple_free) *_pa_simple_free = pa_simple_free;
	static const __typeof__ (pa_simple_write) *_pa_simple_write = pa_simple_write;
	static const __typeof__ (pa_simple_drain) *_pa_simple_drain = pa_simple_drain;
	static const __typeof__ (pa_simple_get_latency) *_pa_simple_get_latency = pa_simple_get_latency;
	static const __typeof__ (pa_strerror) *_pa_strerror = pa_strerror;
#endif

static bool loadLibrary(bool loadNeeded) {
#ifdef USE_DYNAMIC_LOADING
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
	dlHandle = dlopen(PA_LIB_NAME, RTLD_NOW);
	if (NULL == dlHandle) {
		qDebug() << "PulseAudio library failed to load:" << dlerror();
		return false;
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
#endif
	return true;
}

PulseAudioStream::PulseAudioStream(const AudioDevice *device, QSynth *useSynth,
	unsigned int useSampleRate) : synth(useSynth), sampleRate(useSampleRate), stream(NULL),
	sampleCount(0), pendingClose(false)
{
	unsigned int unused;
	device->driver->getAudioSettings(&bufferSize, &unused, &midiLatency);
	bufferSize *= sampleRate / 1000 /* ms per sec*/;
	buffer = new Bit16s[2 * bufferSize];
}

PulseAudioStream::~PulseAudioStream() {
	if (stream != NULL) {
		close();
	}
	delete[] buffer;
}

void* PulseAudioStream::processingThread(void *userData) {
	int error;
	PulseAudioStream *driver = (PulseAudioStream *)userData;
	qDebug() << "PulseAudio: Processing thread started";
	while (!driver->pendingClose) {
#ifdef USE_PA_TIMING
		double realSampleRate = driver->sampleRate;
		MasterClockNanos realSampleTime = MasterClock::getClockNanos() + _pa_simple_get_latency(driver->stream, &error) * MasterClock::NANOS_PER_MICROSECOND;
		MasterClockNanos firstSampleNanos = realSampleTime - driver->midiLatency * MasterClock::NANOS_PER_MILLISECOND; // MIDI latency + total stream latency
#else
		double realSampleRate = driver->sampleRate / driver->clockSync.getDrift();
		MasterClockNanos realSampleTime = MasterClockNanos(driver->sampleCount / (double)driver->sampleRate * MasterClock::NANOS_PER_SECOND);
		MasterClockNanos firstSampleNanos = driver->clockSync.sync(realSampleTime) - driver->midiLatency * MasterClock::NANOS_PER_MILLISECOND; // MIDI latency only
#endif
		driver->synth->render(driver->buffer, driver->bufferSize,
			firstSampleNanos, realSampleRate);
		if (_pa_simple_write(driver->stream, driver->buffer, driver->bufferSize * FRAME_SIZE, &error) < 0) {
			qDebug() << "pa_simple_write() failed:" << _pa_strerror(error);
			_pa_simple_free(driver->stream);
			driver->stream = NULL;
			qDebug() << "PulseAudio: Processing thread stopped";
			driver->synth->close();
			return NULL;
		}
		driver->sampleCount += driver->bufferSize;
	}
	driver->pendingClose = false;
	return NULL;
}

bool PulseAudioStream::start() {
	int error;
	if (buffer == NULL) {
		return false;
	}
	memset(buffer, 0, FRAME_SIZE * bufferSize);
	if (stream != NULL) {
		close();
	}

	qDebug() << "Using PulseAudio default device";

	// The Sample format to use
	static const pa_sample_spec ss = {
			PA_SAMPLE_S16LE, // format
			sampleRate,
			2 // channels
	};

	// Create a new playback stream
	if (!(stream = _pa_simple_new(NULL, "mt32emu-qt", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) {
		qDebug() << "pa_simple_new() failed:" << _pa_strerror(error);
		return false;
	}

	// Start playing 1 sec to fill PulseAudio buffers
	int initFrames = sampleRate;
	while (initFrames > 0) {
		if (_pa_simple_write(stream, buffer, FRAME_SIZE * bufferSize, &error) < 0) {
			qDebug() << "pa_simple_write() failed:" << _pa_strerror(error);
			_pa_simple_free(stream); 
			stream = NULL;
			return false;
		}
		initFrames -= bufferSize;
	}
	pthread_t threadID;
	if((error = pthread_create(&threadID, NULL, processingThread, this))) {
		qDebug() << "PulseAudio: Processing Thread creation failed:" << error;
	}
	return true;
}

void PulseAudioStream::close() {
	int error;
	if (stream != NULL) {
		pendingClose = true;
		if (_pa_simple_drain(stream, &error) < 0) {
			qDebug() << "pa_simple_drain() failed:" << _pa_strerror(error);
		}
		qDebug() << "PulseAudio: Stopping processing thread...";
		while (pendingClose) {
			sleep(1);
		}
		qDebug() << "PulseAudio: Processing thread stopped";
		_pa_simple_free(stream); 
		stream = NULL;
	}
	return;
}

PulseAudioDefaultDevice::PulseAudioDefaultDevice(PulseAudioDriver const * const driver) : AudioDevice(driver, "default", "Default") {
}

PulseAudioStream *PulseAudioDefaultDevice::startAudioStream(QSynth *synth, unsigned int sampleRate) const {
	PulseAudioStream *stream = new PulseAudioStream(this, synth, sampleRate);
	if (stream->start()) {
		return stream;
	}
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

QList<AudioDevice *> PulseAudioDriver::getDeviceList() const {
	QList<AudioDevice *> deviceList;
	if (isLibraryFound) {
		deviceList.append(new PulseAudioDefaultDevice(this));
	}
	return deviceList;
}

void PulseAudioDriver::validateAudioSettings() {
	if (midiLatency == 0) {
		midiLatency = DEFAULT_MIDI_LATENCY;
	}
	if (chunkLen == 0) {
		chunkLen = DEFAULT_CHUNK_MS;
	}
}
