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

OSSAudioStream::OSSAudioStream(const AudioDriverSettings &useSettings, QSynth &useSynth, const quint32 useSampleRate) :
	AudioStream(useSettings, useSynth, useSampleRate), buffer(NULL), stream(0), processingThreadID(0), stopProcessing(false)
{
	bufferSize = settings.chunkLen * sampleRate / MasterClock::MILLIS_PER_SECOND;
}

OSSAudioStream::~OSSAudioStream() {
	if (stream != 0) stop();
	delete[] buffer;
}

void *OSSAudioStream::processingThread(void *userData) {
	int error;
	bool isErrorOccured = false;
	OSSAudioStream &audioStream = *(OSSAudioStream *)userData;
	qDebug() << "OSS audio: Processing thread started";
	while (!audioStream.stopProcessing) {
		MasterClockNanos nanosNow = MasterClock::getClockNanos();
		quint32 framesInAudioBuffer;
		if (audioStream.settings.advancedTiming) {
			int delay = 0;
			if (ioctl(audioStream.stream, SNDCTL_DSP_GETODELAY, &delay) == -1) {
				qDebug() << "SNDCTL_DSP_GETODELAY failed:" << errno;
				isErrorOccured = true;
				break;
			}
			framesInAudioBuffer = quint32(delay / FRAME_SIZE);
		} else {
			framesInAudioBuffer = 0;
		}
		audioStream.updateTimeInfo(nanosNow, framesInAudioBuffer);
		audioStream.synth.render(audioStream.buffer, audioStream.bufferSize);
		error = write(audioStream.stream, audioStream.buffer, FRAME_SIZE * audioStream.bufferSize);
		if (error != int(FRAME_SIZE * audioStream.bufferSize)) {
			if (error == -1) {
				qDebug() << "OSS audio: write failed:" << errno;
			} else {
				qDebug() << "OSS audio: write failed. Written bytes:" << error;
			}
			isErrorOccured = true;
			break;
		}
		audioStream.renderedFramesCount += audioStream.bufferSize;
	}
	qDebug() << "OSS audio: Processing thread stopped";
	if (isErrorOccured) {
		close(audioStream.stream);
		audioStream.stream = 0;
		audioStream.synth.close();
	} else {
		audioStream.stopProcessing = false;
	}
	audioStream.processingThreadID = 0;
	return NULL;
}

bool OSSAudioStream::start() {
	if (stream != 0) stop();

	qDebug() << "Using OSS default audio device";

	// Open audio device
	stream = open(deviceName, O_WRONLY, 0);
	if (stream == -1) {
		qDebug() << "OSS audio: open failed:" << errno;
		stream = 0;
		return false;
	}

	int tmp = 0;
	uint fragSize = (FRAME_SIZE * audioLatencyFrames) / NUM_OF_FRAGMENTS;
	while (fragSize > 1) {
		tmp++;
		fragSize >>= 1;
	}
	tmp |= (NUM_OF_FRAGMENTS << 16);
	if (ioctl(stream, SNDCTL_DSP_SETFRAGMENT, &tmp) == -1) {
		qDebug() << "OSS audio: SNDCTL_DSP_SETFRAGMENT failed:" << errno;
		close(stream);
		stream = 0;
		return false;
	}
	qDebug() << "OSS audio: Set number of fragments:"  << (tmp >> 16) << "Fragment size:" << (tmp & 0xFFFF);

	// Set the sample format
	tmp = AFMT_S16_NE;/* Native 16 bits */
	if (ioctl(stream, SNDCTL_DSP_SETFMT, &tmp) == -1) {
		qDebug() << "OSS audio: SNDCTL_DSP_SETFMT failed:" << errno;
		close(stream);
		stream = 0;
		return false;
	}

	if (tmp != AFMT_S16_NE) {
		qDebug() << "OSS audio: The device doesn't support the 16 bit sample format";
		close(stream);
		stream = 0;
		return false;
	}

	// Set the number of channels
	tmp = 2; // Stereo
	if (ioctl(stream, SNDCTL_DSP_CHANNELS, &tmp) == -1) {
		qDebug() << "OSS audio: SNDCTL_DSP_CHANNELS failed:" << errno;
		close(stream);
		stream = 0;
		return false;
	}

	if (tmp != 2) {
		qDebug() << "OSS audio: The device doesn't support stereo mode";
		close(stream);
		stream = 0;
		return false;
	}

	// Set the sample rate
	tmp = sampleRate;
	if (ioctl(stream, SNDCTL_DSP_SPEED, &tmp) == -1) {
		qDebug() << "OSS audio: SNDCTL_DSP_SPEED failed:" << errno;
		close(stream);
		stream = 0;
		return false;
	}

	if (tmp != (int)sampleRate) {
		qDebug() << "OSS audio: Can't set sample rate" << sampleRate << ". Proposed value:" << tmp;
		close(stream);
		stream = 0;
		return false;
	}

	audio_buf_info bi;
	if (ioctl(stream, SNDCTL_DSP_GETOSPACE, &bi) == -1)
	{
		qDebug() << "OSS audio: SNDCTL_DSP_GETOSPACE failed:" << errno;
		close(stream);
		stream = 0;
		return false;
	}
	audioLatencyFrames = bi.bytes / FRAME_SIZE;
	bufferSize = bi.fragsize / FRAME_SIZE;
	buffer = new Bit16s[/* number of channels */ 2 * bufferSize];
	if (buffer == NULL) {
		qDebug() << "OSS audio setup: Memory allocation error, driver died";
		close(stream);
		stream = 0;
		return false;
	}
	memset(buffer, 0, FRAME_SIZE * bufferSize);
	qDebug() << "OSS audio setup: Number of fragments:" << bi.fragments << "Audio buffer size:" << bi.bytes << "bytes ("<< audioLatencyFrames << ") frames";
	qDebug() << "fragsize:" << bi.fragsize << "bufferSize:" << bufferSize;

	// Setup initial MIDI latency
	if (isAutoLatencyMode()) midiLatencyFrames = audioLatencyFrames + ((DEFAULT_MIDI_LATENCY * sampleRate) / MasterClock::MILLIS_PER_SECOND);

	// Start playing to fill audio buffers
	int initFrames = audioLatencyFrames;
	while (initFrames > 0) {
		tmp = write(stream, buffer, FRAME_SIZE * bufferSize);
		if (tmp != int(FRAME_SIZE * bufferSize)) {
			if (tmp == -1) {
				qDebug() << "OSS audio: write failed:" << errno;
			} else {
				qDebug() << "OSS audio: write failed. Written bytes:" << tmp;
			}
			close(stream);
			stream = 0;
			return false;
		}
		initFrames -= bufferSize;
	}
	tmp = pthread_create(&processingThreadID, NULL, processingThread, this);
	if (tmp != 0) {
		processingThreadID = 0;
		qDebug() << "OSS audio: Processing Thread creation failed:" << tmp;
		close(stream);
		stream = 0;
		return false;
	}
	return true;
}

void OSSAudioStream::stop() {
	if (stream != 0) {
		qDebug() << "OSS audio: Stopping processing thread...";
		stopProcessing = true;
		pthread_join(processingThreadID, NULL);
		processingThreadID = 0;
		stopProcessing = false;
		close(stream);
		stream = 0;
	}
	return;
}

OSSAudioDefaultDevice::OSSAudioDefaultDevice(OSSAudioDriver &driver) : AudioDevice(driver, "Default") {}

AudioStream *OSSAudioDefaultDevice::startAudioStream(QSynth &synth, const uint sampleRate) const {
	OSSAudioStream *stream = new OSSAudioStream(driver.getAudioSettings(), synth, sampleRate);
	if (stream->start()) return stream;
	delete stream;
	return NULL;
}

OSSAudioDriver::OSSAudioDriver(Master *master) : AudioDriver("OSS", "OSS") {
	Q_UNUSED(master);

	loadAudioSettings();
}

const QList<const AudioDevice *> OSSAudioDriver::createDeviceList() {
	QList<const AudioDevice *> deviceList;
	deviceList.append(new OSSAudioDefaultDevice(*this));
	return deviceList;
}

void OSSAudioDriver::validateAudioSettings(AudioDriverSettings &settings) const {
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
