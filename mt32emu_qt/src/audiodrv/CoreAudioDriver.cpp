/* Copyright (C) 2011, 2012, 2013, 2014 Jerome Fisher, Sergey V. Mikayev
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

#include "CoreAudioDriver.h"

#include "../Master.h"
#include "../QSynth.h"

CoreAudioStream::CoreAudioStream(const AudioDriverSettings &useSettings, QSynth &useSynth, quint32 useSampleRate) :
	AudioStream(useSettings, useSynth, useSampleRate), audioQueue(NULL), bufferByteSize(4096*4) {}

CoreAudioStream::~CoreAudioStream() {
	close();
}

void CoreAudioStream::renderOutputBuffer(void *userData, AudioQueueRef queue, AudioQueueBufferRef buffer) {
	CoreAudioStream *stream = (CoreAudioStream *)userData;
	const MasterClockNanos nanosNow = MasterClock::getClockNanos();
	quint32 framesInAudioBuffer = 0;
	if (stream->settings.advancedTiming) {
		//framesInAudioBuffer = quint32((timeInfo->outputBufferDacTime - Pa_GetStreamTime(stream->stream)) * Pa_GetStreamInfo(stream->stream)->sampleRate);
	}
	stream->updateTimeInfo(nanosNow, framesInAudioBuffer);

	uint frameCount = buffer->mAudioDataByteSize >> 2;
	stream->synth.render((MT32Emu::Bit16s *)buffer->mAudioData, frameCount);
	stream->renderedFramesCount += frameCount;

	OSStatus res = AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
	if (res) qDebug() << "CoreAudio: AudioQueueEnqueueBuffer() failed with error code:" << res;
}

bool CoreAudioStream::start() {
	if (audioQueue != NULL) {
		return true;
	}

	qDebug() << "CoreAudio: using default audio output device";

	AudioStreamBasicDescription dataFormat = {sampleRate, kAudioFormatLinearPCM, kAudioFormatFlagIsSignedInteger, 4, 1, 4, 2, 16, 0};
	OSStatus res = AudioQueueNewOutput(&dataFormat, renderOutputBuffer, this, CFRunLoopGetCurrent(), kCFRunLoopCommonModes, 0, &audioQueue);
	if (res || audioQueue == NULL) {
		qDebug() << "CoreAudio: AudioQueueNewOutput() failed with error code:" << res;
		return false;
	}

	for (int i = 0; i < 3; i++) {
		res = AudioQueueAllocateBuffer(audioQueue, bufferByteSize, buffers + i);
		if (res || buffers[i] == NULL) {
			qDebug() << "CoreAudio: AudioQueueAllocateBuffer() failed with error code" << res;
			res = AudioQueueDispose(audioQueue, true);
			if (res) qDebug() << "CoreAudio: AudioQueueDispose() failed with error code" << res;
			audioQueue = NULL;
			return false;
		}
		buffers[i]->mAudioDataByteSize = bufferByteSize;
		// Prime the buffer allocated
		renderOutputBuffer(this, audioQueue, buffers[i]);
	}

	res = AudioQueueStart(audioQueue, NULL);
	if (res) {
		qDebug() << "CoreAudio: AudioQueueStart() failed with error code" << res;
		res = AudioQueueDispose(audioQueue, true);
		if (res) qDebug() << "CoreAudio: AudioQueueDispose() failed with error code" << res;
		audioQueue = NULL;
		return false;
	}

	return true;
}

void CoreAudioStream::close() {
	OSStatus res = AudioQueueDispose(audioQueue, true);
	if (res) qDebug() << "CoreAudio: AudioQueueDispose() failed with error code" << res;
	audioQueue = NULL;
}

CoreAudioDevice::CoreAudioDevice(CoreAudioDriver &driver) :
	AudioDevice(driver, "Default output device") {}

AudioStream *CoreAudioDevice::startAudioStream(QSynth &synth, const uint sampleRate) const {
	CoreAudioStream *stream = new CoreAudioStream(driver.getAudioSettings(), synth, sampleRate);
	if (stream->start()) {
		return (AudioStream *)stream;
	}
	delete stream;
	return NULL;
}

CoreAudioDriver::CoreAudioDriver(Master *useMaster) : AudioDriver("CoreAudio", "CoreAudio") {
	Q_UNUSED(useMaster);

	loadAudioSettings();
}

CoreAudioDriver::~CoreAudioDriver() {
}

const QList<const AudioDevice *> CoreAudioDriver::createDeviceList() {
	QList<const AudioDevice *> deviceList;
	deviceList.append(new CoreAudioDevice(*this));
	return deviceList;
}
