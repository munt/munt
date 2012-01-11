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

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/soundcard.h>

#include "OSSAudioDriver.h"

#include "../QSynth.h"

using namespace MT32Emu;

static const unsigned int NUM_OF_FRAGMENTS = 2;
static const unsigned int FRAME_SIZE = 4; // Stereo, 16-bit
static const unsigned int DEFAULT_CHUNK_MS = 16;
static const unsigned int DEFAULT_AUDIO_LATENCY = 32;
static const unsigned int DEFAULT_MIDI_LATENCY = 16;
static const char deviceName[] = "/dev/dsp";

OSSAudioStream::OSSAudioStream(const AudioDevice *device, QSynth *useSynth,
		unsigned int useSampleRate) : synth(useSynth), sampleRate(useSampleRate),
		buffer(NULL), stream(0), sampleCount(0), pendingClose(false)
{
	device->driver->getAudioSettings(&bufferSize, &audioLatency, &midiLatency);
	bufferSize *= sampleRate / 1000 /* ms per sec*/;
}

OSSAudioStream::~OSSAudioStream() {
	if (stream != 0) {
		stop();
	}
	delete[] buffer;
}

void* OSSAudioStream::processingThread(void *userData) {
	int error;
	bool isErrorOccured = false;
	OSSAudioStream *driver = (OSSAudioStream *)userData;
	qDebug() << "OSS audio: Processing thread started";
	while (!driver->pendingClose) {
#ifdef USE_OSS_TIMING
		int delay = 0;
		if (ioctl (driver->stream, SNDCTL_DSP_GETODELAY, &delay) == -1) {
			qDebug() << "SNDCTL_DSP_GETODELAY failed:" << errno;
			isErrorOccured = true;
			break;
		}
		// qDebug() << "Delay:" << delay;
		MasterClockNanos realSampleTime = MasterClock::getClockNanos()
		 + (double)delay / (FRAME_SIZE * driver->sampleRate) * MasterClock::NANOS_PER_SECOND;
		MasterClockNanos firstSampleNanos = realSampleTime - (driver->midiLatency + driver->audioLatency)
			* MasterClock::NANOS_PER_MILLISECOND; // MIDI latency + total stream audio latency
		double realSampleRate = driver->sampleRate;
#else
		double realSampleRate = driver->sampleRate / driver->clockSync.getDrift();
		MasterClockNanos realSampleTime = MasterClockNanos(driver->sampleCount /
			(double)driver->sampleRate * MasterClock::NANOS_PER_SECOND);
		MasterClockNanos firstSampleNanos = driver->clockSync.sync(realSampleTime) -
			driver->midiLatency * MasterClock::NANOS_PER_MILLISECOND; // MIDI latency only
#endif
		driver->synth->render(driver->buffer, driver->bufferSize, firstSampleNanos, realSampleRate);
		if ((error = write(driver->stream, driver->buffer, FRAME_SIZE * driver->bufferSize)) != (int)(FRAME_SIZE * driver->bufferSize)) {
			if (error == -1) {
				qDebug() << "OSS audio: write failed:" << errno;
			} else {
				qDebug() << "OSS audio: write failed. Written bytes:" << error;
			}
			isErrorOccured = true;
			break;
		}
		driver->sampleCount += driver->bufferSize;
	}
	qDebug() << "OSS audio: Processing thread stopped";
	if (isErrorOccured) {
		close(driver->stream);
		driver->stream = 0;
		driver->synth->close();
	} else {
		driver->pendingClose = false;
	}
	return NULL;
}

bool OSSAudioStream::start() {
	if (stream != 0) {
		stop();
	}
	qDebug() << "Using OSS default audio device";

	// Open audio device
	if ((stream = open(deviceName, O_WRONLY, 0)) == -1) {
		qDebug() << "OSS audio: open failed:" << errno;
		stream = 0;
		return false;
	}

	int tmp = 0;
	unsigned int fragSize = FRAME_SIZE * sampleRate / 1000 * audioLatency / NUM_OF_FRAGMENTS;
	while (fragSize > 1) {
		tmp++;
		fragSize >>= 1;
	}
	tmp |= (NUM_OF_FRAGMENTS << 16);
	if (ioctl (stream, SNDCTL_DSP_SETFRAGMENT, &tmp) == -1) {
		qDebug() << "OSS audio: SNDCTL_DSP_SETFRAGMENT failed:" << errno;
		return false;
	}
	qDebug() << "OSS audio: Set number of fragments:"  << (tmp >> 16) << "Fragment size:" << (tmp & 0xFFFF);

	// Set the sample format
	tmp = AFMT_S16_NE;/* Native 16 bits */
	if (ioctl (stream, SNDCTL_DSP_SETFMT, &tmp) == -1) {
		qDebug() << "OSS audio: SNDCTL_DSP_SETFMT failed:" << errno;
		return false;
	}

	if (tmp != AFMT_S16_NE) {
		qDebug() << "OSS audio: The device doesn't support the 16 bit sample format";
		return false;
	}

	// Set the number of channels
	tmp = 2; // Stereo
	if (ioctl (stream, SNDCTL_DSP_CHANNELS, &tmp) == -1) {
		qDebug() << "OSS audio: SNDCTL_DSP_CHANNELS failed:" << errno;
		return false;
	}

	if (tmp != 2) {
		qDebug() << "OSS audio: The device doesn't support stereo mode";
		return false;
	}

	// Set the sample rate
	tmp = sampleRate;
	if (ioctl (stream, SNDCTL_DSP_SPEED, &tmp) == -1) {
		qDebug() << "OSS audio: SNDCTL_DSP_SPEED failed:" << errno;
		return false;
	}

	if (tmp != (int)sampleRate) {
		qDebug() << "OSS audio: Can't set sample rate" << sampleRate << ". Proposed value:" << tmp;
		return false;
	}

	audio_buf_info bi;
	if (ioctl (stream, SNDCTL_DSP_GETOSPACE, &bi) == -1)
	{
		qDebug() << "OSS audio: SNDCTL_DSP_GETOSPACE failed:" << errno;
		return false;
	}
	audioLatency = 1000 * bi.bytes / (FRAME_SIZE * sampleRate);
	bufferSize = bi.fragsize / FRAME_SIZE;
	buffer = new Bit16s[bufferSize * /* number of channels */ 2];
	if (buffer == NULL) {
		qDebug() << "OSS audio setup: Memory allocation error, driver died";
		return false;
	}
	memset(buffer, 0, FRAME_SIZE * bufferSize);
	qDebug() << "OSS audio setup: Number of fragments:" << bi.fragments << "Audio buffer size:" << bi.bytes << "bytes ("<< audioLatency  << ") ms";
	qDebug() << "fragsize:" << bi.fragsize << "bufferSize:" << bufferSize;

	// Start playing to fill audio buffers
	int initFrames = audioLatency * 1e-3 * sampleRate;
	while (initFrames > 0) {
		if ((tmp = write(stream, buffer, FRAME_SIZE * bufferSize)) != (int)(FRAME_SIZE * bufferSize)) {
			if (tmp == -1) {
				qDebug() << "OSS audio: write failed:" << errno;
			} else {
				qDebug() << "OSS audio: write failed. Written bytes:" << tmp;
			}
			return false;
		}
		initFrames -= bufferSize;
	}
	pthread_t threadID;
	if((tmp = pthread_create(&threadID, NULL, processingThread, this))) {
		qDebug() << "OSS audio: Processing Thread creation failed:" << tmp;
	}
	return true;
}

void OSSAudioStream::stop() {
	if (stream != 0) {
		pendingClose = true;
		qDebug() << "OSS audio: Stopping processing thread...";
		while (pendingClose) {
			sleep(1);
		}
		close(stream);
		stream = 0;
	}
	return;
}

OSSAudioDefaultDevice::OSSAudioDefaultDevice(OSSAudioDriver const * const driver) :
	AudioDevice(driver, "default", "Default")
{
}

OSSAudioStream *OSSAudioDefaultDevice::startAudioStream(QSynth *synth,
	unsigned int sampleRate) const
{
	OSSAudioStream *stream = new OSSAudioStream(this, synth, sampleRate);
	if (stream->start()) {
		return stream;
	}
	delete stream;
	return NULL;
}

OSSAudioDriver::OSSAudioDriver(Master *master) : AudioDriver("OSS", "OSS") {
	Q_UNUSED(master);

	loadAudioSettings();
}

OSSAudioDriver::~OSSAudioDriver() {
}

QList<AudioDevice *> OSSAudioDriver::getDeviceList() const {
	QList<AudioDevice *> deviceList;
	deviceList.append(new OSSAudioDefaultDevice(this));
	return deviceList;
}

void OSSAudioDriver::validateAudioSettings() {
	if (midiLatency == 0) {
		midiLatency = DEFAULT_MIDI_LATENCY;
	}
	if (audioLatency == 0) {
		audioLatency = DEFAULT_AUDIO_LATENCY;
	}
	if (chunkLen == 0) {
		chunkLen = DEFAULT_CHUNK_MS;
	}
	if (chunkLen > audioLatency) {
		chunkLen = audioLatency;
	}
}
