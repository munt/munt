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

#include "PortAudioDriver.h"

#include "../Master.h"
#include "../QSynth.h"

using namespace MT32Emu;

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
	for (PaHostApiIndex hostApiIndex = 0; hostApiIndex < hostApiCount; hostApiIndex++) {
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
		for (PaDeviceIndex deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
			const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
			if (deviceInfo == NULL) {
				qDebug() << "Pa_GetDeviceInfo() returned NULL for" << deviceIndex;
				continue;
			}
			if (deviceInfo->hostApi != hostApiIndex)
				continue;
			qDebug() << " Device:" << deviceIndex << QString().fromLocal8Bit(deviceInfo->name);
		}
	}
}

PortAudioStream::PortAudioStream(const AudioDriverSettings &useSettings, QSynth &useSynth, quint32 useSampleRate) :
	AudioStream(useSettings, useSynth, useSampleRate), stream(NULL) {}

PortAudioStream::~PortAudioStream() {
	close();
}

bool PortAudioStream::start(PaDeviceIndex deviceIndex) {
	if (stream != NULL) {
		return true;
	}
	if (deviceIndex < 0) {
		deviceIndex = Pa_GetDefaultOutputDevice();
		if (deviceIndex == paNoDevice) {
			qDebug() << "PortAudio: no default output device found";
			return false;
		}
	}
	const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
	if (deviceInfo == NULL) {
		qDebug() << "Pa_GetDeviceInfo() returned NULL for" << deviceIndex;
		return false;
	}
	const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
	qDebug() << "PortAudio: using audio device:" << deviceIndex << "API: " + QString(hostApiInfo->name) << QString().fromLocal8Bit(deviceInfo->name);
	/*
	if (deviceInfo->maxOutputChannels < 2) {
		qDebug() << "Device does not support stereo; maxOutputChannels =" << deviceInfo->maxOutputChannels;
		return false;
	}
	*/

	MasterClockNanos audioLatency = settings.audioLatency * MasterClock::NANOS_PER_MILLISECOND;
	if (audioLatency == 0) {
		audioLatency = deviceInfo->defaultHighOutputLatency * MasterClock::NANOS_PER_SECOND;
	}
	PaStreamParameters outStreamParameters = {deviceIndex, 2, paInt16, (double)audioLatency / MasterClock::NANOS_PER_SECOND, NULL};
	PaError err =  Pa_OpenStream(&stream, NULL, &outStreamParameters, sampleRate, paFramesPerBufferUnspecified, paNoFlag, paCallback, this);
	if(err != paNoError) {
		qDebug() << "Pa_OpenStream() returned PaError" << err << "-" << Pa_GetErrorText(err);
		return false;
	}
	const PaStreamInfo *streamInfo = Pa_GetStreamInfo(stream);
	if (streamInfo->outputLatency != 0) { // Quick fix for Mac
		audioLatency = streamInfo->outputLatency * MasterClock::NANOS_PER_SECOND;
	}
	audioLatencyFrames = quint32((audioLatency * sampleRate) / MasterClock::NANOS_PER_SECOND);
	qDebug() << "PortAudio: audio latency (s):" << (double)audioLatency / MasterClock::NANOS_PER_SECOND;

	// Setup initial MIDI latency
	if (isAutoLatencyMode()) midiLatencyFrames = audioLatencyFrames;
	qDebug() << "PortAudio: MIDI latency (s):" << (double)midiLatencyFrames / sampleRate;

	err = Pa_StartStream(stream);
	if(err != paNoError) {
		qDebug() << "Pa_StartStream() returned PaError" << err;
		Pa_CloseStream(stream);
		stream = NULL;
		return false;
	}
	return true;
}

int PortAudioStream::paCallback(const void *inputBuffer, void *outputBuffer, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
	Q_UNUSED(inputBuffer);
	Q_UNUSED(statusFlags);

	PortAudioStream *stream = (PortAudioStream *)userData;
	MasterClockNanos nanosNow = MasterClock::getClockNanos();
	quint32 framesInAudioBuffer;
	if (stream->settings.advancedTiming) {
		double currentTime = Pa_GetStreamTime(stream->stream);
		double bufferedTime = timeInfo->outputBufferDacTime - currentTime;
		framesInAudioBuffer = bufferedTime <= 0.0 ? 0 : quint32(bufferedTime * Pa_GetStreamInfo(stream->stream)->sampleRate);
	} else {
		framesInAudioBuffer = 0;
	}
	stream->updateTimeInfo(nanosNow, framesInAudioBuffer);

	stream->synth.render((Bit16s *)outputBuffer, frameCount);
	stream->renderedFramesCount += frameCount;
	return paContinue;
}

void PortAudioStream::close() {
	if (stream == NULL) return;
	Pa_StopStream(stream);
	Pa_CloseStream(stream);
	stream = NULL;
}

PortAudioDevice::PortAudioDevice(PortAudioDriver &driver, int useDeviceIndex, QString useDeviceName) :
	AudioDevice(driver, useDeviceName), deviceIndex(useDeviceIndex) {}

AudioStream *PortAudioDevice::startAudioStream(QSynth &synth, const uint sampleRate) const {
	PortAudioStream *stream = new PortAudioStream(driver.getAudioSettings(), synth, sampleRate);
	if (stream->start(deviceIndex)) {
		return stream;
	}
	delete stream;
	return NULL;
}

PortAudioDriver::PortAudioDriver(Master *useMaster) : AudioDriver("portaudio", "PortAudio") {
	Q_UNUSED(useMaster);

	if (!paInitialised) {
		PaError err = Pa_Initialize();
		if (err != paNoError) {
			qDebug() << "Error initializing PortAudio";
			// FIXME: Do something drastic instead of continuing on happily
		} else {
			paInitialised = true;
		}
	}
	dumpPortAudioDevices();
	loadAudioSettings();
}

PortAudioDriver::~PortAudioDriver() {
	if (paInitialised) {
		if (Pa_Terminate() != paNoError) {
			qDebug() << "Error terminating PortAudio";
			// FIXME: Do something drastic instead of continuing on happily
		} else {
			paInitialised = false;
		}
	}
}

const QList<const AudioDevice *> PortAudioDriver::createDeviceList() {
	QList<const AudioDevice *> deviceList;
	PaDeviceIndex deviceCount = paInitialised ? Pa_GetDeviceCount() : 0;
	if (deviceCount < 0) {
		qDebug() << "Pa_GetDeviceCount() returned error" << deviceCount;
		deviceCount = 0;
	}
	for (int deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
		const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
		if (deviceInfo == NULL) {
			qDebug() << "Pa_GetDeviceInfo() returned NULL for" << deviceIndex;
			continue;
		}
		if (deviceInfo->maxOutputChannels == 0) {
			// Seems to be an input device
			continue;
		}
		QString hostApiName;
		const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
		if (hostApiInfo == NULL) {
			qDebug() << "Pa_GetHostApiInfo() returned NULL for" << deviceInfo->hostApi;
			hostApiName = "#" + QString().setNum(deviceInfo->hostApi);
		} else {
			hostApiName = QString(hostApiInfo->name);
		}
		QString deviceName = "(" + hostApiName + ") " + QString().fromLocal8Bit(deviceInfo->name);
		deviceList.append(new PortAudioDevice(*this, deviceIndex, deviceName));
	}
	return deviceList;
}
