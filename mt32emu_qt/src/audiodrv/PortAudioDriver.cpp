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

#include "PortAudioDriver.h"

#include "../Master.h"
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
			qDebug() << " Device:" << deviceIndex << QString().fromLocal8Bit(deviceInfo->name);
		}
	}
}

PortAudioStream::PortAudioStream(const PortAudioDevice *device, QSynth *useSynth, unsigned int useSampleRate) :
	synth(useSynth), sampleRate(useSampleRate), stream(NULL), sampleCount(0)
{
	const AudioDriverSettings &driverSettings = device->driver->getAudioSettings();
	audioLatency = driverSettings.audioLatency * MasterClock::NANOS_PER_MILLISECOND;
	midiLatency = driverSettings.midiLatency * MasterClock::NANOS_PER_MILLISECOND;
	useAdvancedTiming = driverSettings.advancedTiming;
}

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
	qDebug() << "PortAudio: using audio device:" << deviceIndex << QString().fromLocal8Bit(deviceInfo->name);
	/*
	if (deviceInfo->maxOutputChannels < 2) {
		qDebug() << "Device does not support stereo; maxOutputChannels =" << deviceInfo->maxOutputChannels;
		return false;
	}
	*/

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
	clockSync.scheduleReset();
	const PaStreamInfo *streamInfo = Pa_GetStreamInfo(stream);
	if (streamInfo->outputLatency != 0) { // Quick fix for Mac
		audioLatency = streamInfo->outputLatency * MasterClock::NANOS_PER_SECOND;
	}
	qDebug() << "PortAudio: audio latency (s):" << (double)audioLatency / MasterClock::NANOS_PER_SECOND;
	if (!midiLatency) {
		midiLatency = audioLatency / 2;
	}
	qDebug() << "PortAudio: MIDI latency (s):" << (double)midiLatency / MasterClock::NANOS_PER_SECOND;
	clockSync.setParams(audioLatency, 10 * audioLatency);
	lastSampleMasterClockNanos = MasterClock::getClockNanos() - audioLatency - midiLatency;
	audioBufferSize = quint32((audioLatency * sampleRate) / MasterClock::NANOS_PER_SECOND);
	midiLatencyFrames = quint32((midiLatency * sampleRate) / MasterClock::NANOS_PER_SECOND);

	timeInfoIx = 0;
	timeInfo[0].lastPlayedNanos = MasterClock::getClockNanos();
	timeInfo[0].lastPlayedFramesCount = 0;
	timeInfo[0].actualSampleRate = Pa_GetStreamInfo(stream)->sampleRate;

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
	double realSampleRate;
	MasterClockNanos firstSampleMasterClockNanos;
	// Set this variable to false if PortAudio doesn't provide correct timing information.
	// As for V19, this should be used for OSS (still not implemented, though OSS allows _really_ low latencies) and for PulseAudio + ALSA setup.
	MasterClockNanos nanosNow = MasterClock::getClockNanos();
	if (stream->useAdvancedTiming) {
		MasterClockNanos nanosInAudioBuffer = MasterClockNanos(MasterClock::NANOS_PER_SECOND * (timeInfo->outputBufferDacTime - Pa_GetStreamTime(stream->stream)));
		firstSampleMasterClockNanos = nanosNow + nanosInAudioBuffer - stream->audioLatency - stream->midiLatency;
		realSampleRate = AudioStream::estimateActualSampleRate(Pa_GetStreamInfo(stream->stream)->sampleRate, firstSampleMasterClockNanos, stream->lastSampleMasterClockNanos, stream->audioLatency, (quint32)frameCount);

		quint32 framesInAudioBuffer = quint32((timeInfo->outputBufferDacTime - Pa_GetStreamTime(stream->stream)) * Pa_GetStreamInfo(stream->stream)->sampleRate);
		stream->updateTimeInfo(framesInAudioBuffer, nanosNow);
	} else {
		MasterClockNanos realSampleTime = MasterClockNanos((stream->sampleCount / Pa_GetStreamInfo(stream->stream)->sampleRate) * MasterClock::NANOS_PER_SECOND);
		stream->lastRenderedNanos = stream->clockSync.sync(nanosNow, realSampleTime);
		stream->lastRenderedFramesCount = stream->sampleCount;
		firstSampleMasterClockNanos = stream->lastRenderedNanos - stream->midiLatency;
		realSampleRate = Pa_GetStreamInfo(stream->stream)->sampleRate * stream->clockSync.getDrift();
	}
	unsigned int rendered = stream->synth->render((Bit16s *)outputBuffer, frameCount, firstSampleMasterClockNanos, realSampleRate);
	if (rendered < frameCount) {
		char *out = (char *)outputBuffer;
		// PortAudio requires that the buffer is filled no matter what
		memset(out + rendered * FRAME_SIZE, 0, (frameCount - rendered) * FRAME_SIZE);
	}
	stream->sampleCount += frameCount;
	return paContinue;
}

void PortAudioStream::updateTimeInfo(const quint32 framesInAudioBuffer, const MasterClockNanos measuredNanos) {
	// Count of played frames (assuming no x-runs happend)
	quint32 estimatedNewPlayedFramesCount = quint32(sampleCount - framesInAudioBuffer);
	double secondsElapsed = double(measuredNanos - timeInfo[timeInfoIx].lastPlayedNanos) / MasterClock::NANOS_PER_SECOND;

	// Ensure lastPlayedFramesCount is monotonically increasing and has no jumps
	quint32 newPlayedFramesCount = timeInfo[timeInfoIx].lastPlayedFramesCount + quint32(timeInfo[timeInfoIx].actualSampleRate * secondsElapsed);

	// If the estimation goes too far - do reset
	if (qAbs(qint32(estimatedNewPlayedFramesCount - newPlayedFramesCount)) > (qint32)audioBufferSize) {
		qDebug() << "PortAudio: Estimated play position is way off:" << qint32(estimatedNewPlayedFramesCount - newPlayedFramesCount) << "-> resetting...";
		uint i = 1 - timeInfoIx;
		timeInfo[i].lastPlayedNanos = measuredNanos;
		timeInfo[i].lastPlayedFramesCount = estimatedNewPlayedFramesCount;
		timeInfo[i].actualSampleRate = Pa_GetStreamInfo(stream)->sampleRate;
		timeInfoIx = i;
		return;
	}

	double estimatedNewActualSampleRate = ((double)estimatedNewPlayedFramesCount - timeInfo[timeInfoIx].lastPlayedFramesCount) / secondsElapsed;

	// Now fixup sample rate estimation
	double nominalSampleRate = Pa_GetStreamInfo(stream)->sampleRate;
	double relativeError = estimatedNewActualSampleRate / nominalSampleRate;
	if (relativeError < 0.99) {
		estimatedNewActualSampleRate = 0.99 * nominalSampleRate;
	}
	if (relativeError > 1.01) {
		estimatedNewActualSampleRate = 1.01 * nominalSampleRate;
	}

	uint i = 1 - timeInfoIx;
	timeInfo[i].lastPlayedNanos = measuredNanos;
	timeInfo[i].lastPlayedFramesCount = newPlayedFramesCount;
	timeInfo[i].actualSampleRate = estimatedNewActualSampleRate;
	timeInfoIx = i;
}

// Intended to be called from MIDI receiving thread
bool PortAudioStream::estimateMIDITimestamp(quint32 &timestamp, const MasterClockNanos refNanos) {
	if (useAdvancedTiming) {
		uint i = timeInfoIx;
		quint32 refFrameOffset = quint32(((refNanos - timeInfo[i].lastPlayedNanos) * timeInfo[i].actualSampleRate) / MasterClock::NANOS_PER_SECOND);
		timestamp = timeInfo[i].lastPlayedFramesCount + refFrameOffset + audioBufferSize + midiLatencyFrames;
		return true;
	}

	// Taking snapshots in reverse order to avoid interference with the rendering thread
	quint32 lastRenderedFramesCountSnapshot = (quint32)lastRenderedFramesCount;
	MasterClockNanos lastRenderedNanosSnapshot = lastRenderedNanos;

	MasterClockNanos refNanoOffset = refNanos - lastRenderedNanosSnapshot;
	quint32 refFrameOffset = quint32((refNanoOffset * sampleRate) * clockSync.getDrift() / MasterClock::NANOS_PER_SECOND);
	timestamp = lastRenderedFramesCountSnapshot + refFrameOffset + midiLatencyFrames;
	return true;
}

void PortAudioStream::close() {
	if(stream == NULL)
		return;
	Pa_StopStream(stream);
	Pa_CloseStream(stream);
	stream = NULL;
}

// FIXME: The device index isn't permanent enough for the intended purpose of ID.
// Instead a QString should be constructed that will let us find the same device later
// (e.g. a combination hostAPI/device name/index as last resort).
PortAudioDevice::PortAudioDevice(PortAudioDriver * const driver, int useDeviceIndex, QString useDeviceName) : AudioDevice(driver, QString::number(useDeviceIndex), useDeviceName), deviceIndex(useDeviceIndex) {
}

PortAudioStream *PortAudioDevice::startAudioStream(QSynth *synth, unsigned int sampleRate) const {
	PortAudioStream *stream = new PortAudioStream(this, synth, sampleRate);
	if (stream->start(deviceIndex)) {
		return stream;
	}
	delete stream;
	return NULL;
}

PortAudioDriver::PortAudioDriver(Master *useMaster) :
	AudioDriver("portaudio", "PortAudio")
{
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
	PaDeviceIndex deviceCount = Pa_GetDeviceCount();
	if (deviceCount < 0) {
		qDebug() << "Pa_GetDeviceCount() returned error" << deviceCount;
		deviceCount = 0;
	}
	for(int deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
		const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(deviceIndex);
		if (deviceInfo == NULL) {
			qDebug() << "Pa_GetDeviceInfo() returned NULL for" << deviceIndex;
			continue;
		}
		if (deviceInfo->maxOutputChannels == 0) {
			// Seems to be an input device
			continue;
		}
		const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);
		if (hostApiInfo == NULL) {
			qDebug() << "Pa_GetHostApiInfo() returned NULL for" << deviceInfo->hostApi;
			QString().setNum(deviceInfo->hostApi);
		}
		QString deviceName = "(" + QString(hostApiInfo->name) + ") " + QString().fromLocal8Bit(deviceInfo->name);
		deviceList.append(new PortAudioDevice(this, deviceIndex, deviceName));
	}
	return deviceList;
}
