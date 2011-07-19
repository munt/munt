/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pthread.h>
#include <pulse/error.h>

#include "PulseAudioDriver.h"

#include "../MasterClock.h"
#include "../QSynth.h"

using namespace MT32Emu;

static const int FRAME_SIZE = 4; // Stereo, 16-bit
static const int FRAMES_IN_BUFFER = 1024;
// Latency for MIDI processing
static const MasterClockNanos latency = 100 * MasterClock::NANOS_PER_MILLISECOND;

PulseAudioDriver::PulseAudioDriver(QSynth *useSynth, unsigned int useSampleRate) : synth(useSynth), sampleRate(useSampleRate), stream(NULL), sampleCount(0), pendingClose(false) {
	buffer = new Bit16s[2 * FRAMES_IN_BUFFER];
}

PulseAudioDriver::~PulseAudioDriver() {
	if (stream != NULL) {
		close();
	}
	delete[] buffer;
}

SynthTimestamp PulseAudioDriver::getPlayedAudioNanosPlusLatency() {
	if (stream == NULL) {
		qDebug() << "Stream NULL at getPlayedAudioNanosPlusLatency()";
		return 0;
	}
	int error;
	return MasterClockNanos(sampleCount / sampleRate * MasterClock::NANOS_PER_SECOND) - pa_simple_get_latency(stream, &error) * MasterClock::NANOS_PER_MICROSECOND + latency;
}

void* PulseAudioDriver::processingThread(void *userData) {
	int error;
	PulseAudioDriver *driver = (PulseAudioDriver *)userData;
	qDebug() << "Processing thread started";
	while (!driver->pendingClose) {
#ifdef USE_PA_TIMING
		double realSampleRate = driver->sampleRate / driver->clockSync.getDrift();
		MasterClockNanos realSampleTime = MasterClock::getClockNanos() + pa_simple_get_latency(driver->stream, &error) * MasterClock::NANOS_PER_MICROSECOND;
		MasterClockNanos firstSampleNanos = driver->clockSync.sync(realSampleTime) - latency /* MIDI latency + total stream latency*/;
#else
		double realSampleRate = driver->sampleRate / driver->clockSync.getDrift();
		MasterClockNanos realSampleTime = MasterClockNanos(driver->sampleCount / (double)driver->sampleRate * MasterClock::NANOS_PER_SECOND);
		MasterClockNanos firstSampleNanos = driver->clockSync.sync(realSampleTime) - latency /* MIDI latency only */;
#endif
		driver->synth->render(driver->buffer, FRAMES_IN_BUFFER,
			firstSampleNanos, realSampleRate);
		if (pa_simple_write(driver->stream, driver->buffer, FRAMES_IN_BUFFER * FRAME_SIZE, &error) < 0) {
			qDebug() << "pa_simple_write() failed:" << pa_strerror(error);
			return NULL;
		}
		driver->sampleCount += FRAMES_IN_BUFFER;
	}
	driver->pendingClose = false;
	return NULL;
}

QList<QString> PulseAudioDriver::getDeviceNames() {
	QList<QString> deviceNames;
	deviceNames << "PulseAudio default device";
	return deviceNames;
}

bool PulseAudioDriver::start(int /* deviceIndex */) {
	int error;
	if (buffer == NULL) {
		return false;
	}
	memset(buffer, 0, FRAME_SIZE * FRAMES_IN_BUFFER);
	if (stream != NULL) {
		close();
	}
	qDebug() << "Using PulseAudio default device";

	// The Sample format to use
	static const pa_sample_spec ss = {
			format : PA_SAMPLE_S16LE,
			rate : sampleRate,
			channels : 2
	};

	// Create a new playback stream
	if (!(stream = pa_simple_new(NULL, "mt32emu-qt", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) {
		qDebug() << "pa_simple_new() failed:" << pa_strerror(error);
		return false;
	}

	// Start playing 1 sec to fill PulseAudio buffers
	int initFrames = sampleRate;
	while (initFrames > 0) {
		if (pa_simple_write(stream, buffer, FRAME_SIZE * FRAMES_IN_BUFFER, &error) < 0) {
			qDebug() << "pa_simple_write() failed:" << pa_strerror(error);
			pa_simple_free(stream); 
			stream = NULL;
			return false;
		}
		initFrames -= FRAMES_IN_BUFFER;
	}
	pthread_t threadID;
	if((error = pthread_create(&threadID, NULL, processingThread, this))) {
		qDebug() << "Processing Thread creation failed:" << error;
	}
	return true;
}

void PulseAudioDriver::close() {
	int error;
	if (stream != NULL) {
		pendingClose = true;
		if (pa_simple_drain(stream, &error) < 0) {
			qDebug() << "pa_simple_drain() failed:" << pa_strerror(error);
		}
		qDebug() << "Stopping processing thread...";
		while (pendingClose) {
			sleep(1);
		}
		qDebug() << "Processing thread stopped";
		pa_simple_free(stream); 
 		stream = NULL;
	}
	return;
}
