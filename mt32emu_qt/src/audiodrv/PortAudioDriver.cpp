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

#include "PortAudioDriver.h"

#include "../Master.h"
#include "../MasterClock.h"
#include "../QSynth.h"

using namespace MT32Emu;

static const int FRAME_SIZE = 4; // Stereo, 16-bit

static bool paInitialised = false;

static void dumpPortAudioDevices() {
	PaHostApiIndex hostApiCount = Pa_GetHostApiCount();
	if (hostApiCount < 0) {
		qDebug() << "Pa_GetHostApiCount() returned error" << hostApiCount;
		return;
	}
	PaDeviceIndex deviceCount = Pa_GetDeviceCount();
	if (deviceCount < 0) {
		qDebug() << "Pa_GetDeviceCount() returned error" << deviceCount;
		deviceCount = 0;
	}
	for(PaHostApiIndex hostApiIndex = 0; hostApiIndex < hostApiCount; hostApiIndex++) {
		const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(hostApiIndex);
		if (hostApiInfo == NULL) {
			qDebug() << "Pa_GetHostApiInfo() returned NULL for" << hostApiIndex;
			continue;
		}
		qDebug() << "HostAPI: " << hostApiInfo->name;
		qDebug() << " type =" << hostApiInfo->type;
		qDebug() << " deviceCount =" << hostApiInfo->deviceCount;
		qDebug() << " defaultInputDevice =" << hostApiInfo->defaultInputDevice;
		qDebug() << " defaultOutputDevice =" << hostApiInfo->defaultOutputDevice;
		for(PaDeviceIndex deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
			const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
			if (deviceInfo == NULL) {
				qDebug() << "Pa_GetDeviceInfo() returned NULL for" << deviceIndex;
				continue;
			}
			if (deviceInfo->hostApi != hostApiIndex)
				continue;
			qDebug() << " Device:" << deviceIndex << deviceInfo->name;
		}
	}
}

PortAudioDriver::PortAudioDriver(Master *useMaster) : AudioDriver("portaudio", "PortAudio"), master(useMaster) {
	start();
}

PortAudioDriver::~PortAudioDriver() {
	stop();
}

Master *PortAudioDriver::getMaster() {
	return master;
}

void PortAudioDriver::start() {
	processor = new PortAudioProcessor(this);
	processor->moveToThread(&processorThread);
	processorThread.moveToThread(&processorThread);
	// Start the processor once the thread has started
	processor->connect(&processorThread, SIGNAL(started()), SLOT(start()));
	// Stop the thread once the processor has finished
	processorThread.connect(processor, SIGNAL(finished()), SLOT(quit()));
	processorThread.start();
}

void PortAudioDriver::stop() {
	if (processor != NULL) {
		if (processorThread.isRunning()) {
			processor->stop();
			processorThread.wait();
		}
		delete processor;
		processor = NULL;
	}
}

PortAudioProcessor::PortAudioProcessor(PortAudioDriver *useDriver) : driver(useDriver), stopProcessing(false) {
}

void PortAudioProcessor::stop() {
	stopProcessing = true;
}

void PortAudioProcessor::scanAudioDevices() {
	QList<AudioDevice *> foundDevices;
	PaDeviceIndex deviceCount = Pa_GetDeviceCount();
	if (deviceCount < 0) {
		qDebug() << "Pa_GetDeviceCount() returned error" << deviceCount;
		deviceCount = 0;
	}
	for(int deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
		// FIXME: Keep track so that we can properly dispose of them later
		const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
		if (deviceInfo == NULL) {
			qDebug() << "Pa_GetDeviceInfo() returned NULL for" << deviceIndex;
			continue;
		}
		if (deviceInfo->maxOutputChannels == 0) {
			// Seems to be an input device
			continue;
		}
		QString deviceName = "(" + QString::number(deviceInfo->hostApi) + ") " + deviceInfo->name;
		PortAudioDevice *newAudioDevice = new PortAudioDevice(driver, deviceIndex, deviceName);
		foundDevices.append(newAudioDevice);
		AudioDevice *found = driver->getMaster()->findAudioDevice(newAudioDevice);
		if (found == NULL) {
			qDebug() << "PortAudio device added:" << deviceIndex << deviceName;
			driver->getMaster()->addAudioDevice(newAudioDevice);
			continue;
		}
		if (found->id != QString::number(deviceIndex)) {
			qDebug() << "Changed ID of PortAudio device:" << deviceName;
			qDebug() << "Reconnecting...";
			driver->getMaster()->removeAudioDevice(found);
			driver->getMaster()->addAudioDevice(newAudioDevice);
		}
	}
	driver->getMaster()->removeStaleAudioDevices(driver, foundDevices);
}

void PortAudioProcessor::start() {
	if (!paInitialised) {
		PaError err = Pa_Initialize();
		if (err != paNoError) {
			qDebug() << "Error initialising PortAudio";
			// FIXME: Do something drastic instead of continuing on happily
		}
		paInitialised = true;
	}
	dumpPortAudioDevices();

	while (stopProcessing == false) {
		scanAudioDevices();
		for (int i = 0; i < 10; i++) {
			if (stopProcessing) break;
			MasterClock::sleepForNanos(300000000);
		}
	}
	emit finished();
}

// FIXME: The device index isn't permanent enough for the intended purpose of ID.
// Instead a QString should be constructed that will let us find the same device later
// (e.g. a combination hostAPI/device name/index as last resort).
PortAudioDevice::PortAudioDevice(PortAudioDriver *driver, int useDeviceIndex, QString useDeviceName) : AudioDevice(driver, QString::number(useDeviceIndex), useDeviceName), deviceIndex(useDeviceIndex) {
}

PortAudioStream *PortAudioDevice::startAudioStream(QSynth *synth, unsigned int sampleRate) {
	PortAudioStream *stream = new PortAudioStream(this, synth, sampleRate);
	if (stream->start()) {
		return stream;
	}
	delete stream;
	return NULL;
}

PortAudioStream::PortAudioStream(PortAudioDevice *useDevice, QSynth *useSynth, unsigned int useSampleRate) :
	device(useDevice), synth(useSynth), sampleRate(useSampleRate), stream(NULL), sampleCount(0) {
}

PortAudioStream::~PortAudioStream() {
	close();
}

bool PortAudioStream::start() {
	if (stream != NULL) {
		return true;
	}
	PaDeviceIndex deviceIndex = device->deviceIndex;
	if (deviceIndex < 0) {
		deviceIndex = Pa_GetDefaultOutputDevice();
		if (deviceIndex == paNoDevice) {
			qDebug() << "No default output device found";
			return false;
		}
	}
	const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
	if (deviceInfo == NULL) {
		qDebug() << "Pa_GetDeviceInfo() returned NULL for" << deviceIndex;
		return false;
	}
	qDebug() << "Using PortAudio device:" << deviceIndex << deviceInfo->name;
	/*
	if (deviceInfo->maxOutputChannels < 2) {
		qDebug() << "Device does not support stereo; maxOutputChannels =" << deviceInfo->maxOutputChannels;
		return false;
	}
	*/

	// FIXME: Set audio latency from stored preferences
	audioLatency = 0;
	if (!audioLatency) {
		audioLatency = deviceInfo->defaultHighOutputLatency * MasterClock::NANOS_PER_SECOND;
	}
	PaStreamParameters outStreamParameters = {deviceIndex, 2, paInt16, (double)audioLatency / MasterClock::NANOS_PER_SECOND, NULL};
	PaError err =  Pa_OpenStream(&stream, NULL, &outStreamParameters, sampleRate, paFramesPerBufferUnspecified, paNoFlag, paCallback, this);
	if(err != paNoError) {
		qDebug() << "Pa_OpenStream() returned PaError" << err;
		return false;
	}
	sampleCount = 0;
	clockSync.reset();
	err = Pa_StartStream(stream);
	if(err != paNoError) {
		qDebug() << "Pa_StartStream() returned PaError" << err;
		Pa_CloseStream(stream);
		stream = NULL;
		return false;
	}
	const PaStreamInfo *streamInfo = Pa_GetStreamInfo(stream);
	qDebug() << "Device Output latency (s):" << streamInfo->outputLatency;
	latency = 2.2 * MasterClock::NANOS_PER_SECOND * streamInfo->outputLatency;
	qDebug() << "Using latency (ns):" << latency;
	return true;
}

int PortAudioStream::paCallback(const void *inputBuffer, void *outputBuffer, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
	Q_UNUSED(inputBuffer);
	Q_UNUSED(statusFlags);
	PortAudioStream *stream = (PortAudioStream *)userData;
#ifdef USE_PA_TIMING
	double realSampleRate = timeInfo->actualSampleRate;
	if (realSampleRate == 0.0) {
		// This means PortAudio doesn't provide us the actualSampleRate estimation
		realSampleRate = Pa_GetStreamInfo(stream->stream)->sampleRate;
	}
	qint64 currentlyPlayingAudioNanos = timeInfo->currentTime * MasterClock::NANOS_PER_SECOND;
	qint64 firstSampleAudioNanos = timeInfo->outputBufferDacTime * MasterClock::NANOS_PER_SECOND;
	qint64 renderOffset = firstSampleAudioNanos - currentlyPlayingAudioNanos;
	qint64 offset = stream->latency - renderOffset;
	MasterClockNanos firstSampleMasterClockNanos = stream->clockSync.sync(currentlyPlayingAudioNanos) - offset * stream->clockSync.getDrift();
#else
	Q_UNUSED(timeInfo);
	double realSampleRate = Pa_GetStreamInfo(stream->stream)->sampleRate / stream->clockSync.getDrift();
	MasterClockNanos realSampleTime = MasterClockNanos((stream->sampleCount / Pa_GetStreamInfo(stream->stream)->sampleRate) * MasterClock::NANOS_PER_SECOND);
	MasterClockNanos firstSampleMasterClockNanos = stream->clockSync.sync(realSampleTime) - stream->latency;
#endif
	unsigned int rendered = stream->synth->render((Bit16s *)outputBuffer, frameCount, firstSampleMasterClockNanos, realSampleRate);
	if (rendered < frameCount) {
		char *out = (char *)outputBuffer;
		// PortAudio requires that the buffer is filled no matter what
		memset(out + rendered * FRAME_SIZE, 0, (frameCount - rendered) * FRAME_SIZE);
	}
	stream->sampleCount += frameCount;
	return paContinue;
}

void PortAudioStream::close() {
	if(stream == NULL)
		return;
	Pa_StopStream(stream);
	Pa_CloseStream(stream);
	stream = NULL;
}


