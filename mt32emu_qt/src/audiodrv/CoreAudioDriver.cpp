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

#include "CoreAudioDriver.h"

#include "../Master.h"
#include "../SynthRoute.h"

static const uint DEFAULT_CHUNK_MS = 20;
static const uint DEFAULT_AUDIO_LATENCY = 60;
static const uint DEFAULT_MIDI_LATENCY = 30;

static CFStringRef qStringToCFString(QString qString) {
#if QT_VERSION < QT_VERSION_CHECK(5, 2, 0)
	return CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar *>(qString.unicode()), qString.length());
#else
	return qString.toCFString();
#endif
}

static QString cfStringToQString(CFStringRef string) {
#if QT_VERSION < QT_VERSION_CHECK(5, 2, 0)
	// Taken from Qt 5.2 source code
	// File: qtbase/src/corelib/tools/qstring_mac.mm
	if (!string)
		return QString();
	CFIndex length = CFStringGetLength(string);

	// Fast path: CFStringGetCharactersPtr does not copy but may
	// return null for any and no reason.
	const UniChar *chars = CFStringGetCharactersPtr(string);
	if (chars)
		return QString(reinterpret_cast<const QChar *>(chars), length);

	QString ret(length, Qt::Uninitialized);
	CFStringGetCharacters(string, CFRangeMake(0, length), reinterpret_cast<UniChar *>(ret.data()));
	return ret;
#else
	return QString::fromCFString(string);
#endif
}

CoreAudioStream::CoreAudioStream(const AudioDriverSettings &useSettings, SynthRoute &useSynthRoute, quint32 useSampleRate) :
	AudioStream(useSettings, useSynthRoute, useSampleRate), audioQueue(NULL)
{
	const uint bufferSize = (settings.chunkLen * sampleRate) / MasterClock::MILLIS_PER_SECOND;
	bufferByteSize = bufferSize << 2;
	// Number of buffers should be ceil(audioLatencyFrames / bufferSize)
	numberOfBuffers = (audioLatencyFrames + bufferSize - 1) / bufferSize;
	buffers = new AudioQueueBufferRef[numberOfBuffers];
	// Refine audioLatencyFrames as bufferSize * number of buffers, no less then the specified value
	audioLatencyFrames = numberOfBuffers * bufferSize;
	qDebug() << "CoreAudioDriver: Using" << numberOfBuffers << "buffers, buffer size:" << bufferSize << "frames, audio latency:" << audioLatencyFrames << "frames.";

	// Setup initial MIDI latency
	if (isAutoLatencyMode()) midiLatencyFrames = audioLatencyFrames + ((DEFAULT_MIDI_LATENCY * sampleRate) / MasterClock::MILLIS_PER_SECOND);
	qDebug() << "CoreAudio: total MIDI latency:" << midiLatencyFrames << "frames";
}

CoreAudioStream::~CoreAudioStream() {
	close();
	delete[] buffers;
}

void CoreAudioStream::renderOutputBuffer(void *userData, AudioQueueRef queue, AudioQueueBufferRef buffer) {
	CoreAudioStream *stream = (CoreAudioStream *)userData;
	const MasterClockNanos nanosNow = MasterClock::getClockNanos();
	quint32 framesInAudioBuffer = 0;
	if (queue == NULL) {
		// Priming the buffers, skip timestamp handling
		queue = stream->audioQueue;
	} else if (stream->settings.advancedTiming) {
		AudioTimeStamp audioTimeStamp;
		OSStatus res = AudioQueueGetCurrentTime(queue, NULL, &audioTimeStamp, NULL);
		if (res) {
			qDebug() << "CoreAudio: AudioQueueGetCurrentTime() failed with error code:" << res;
		} else if (audioTimeStamp.mFlags & kAudioTimeStampSampleTimeValid) {
			framesInAudioBuffer = quint32(stream->getRenderedFramesCount() - audioTimeStamp.mSampleTime);
		} else {
			qDebug() << "CoreAudio: AudioQueueGetCurrentTime() returns invalid sample time";
		}
	}

	uint frameCount = buffer->mAudioDataByteSize >> 2;
	stream->renderAndUpdateState((MT32Emu::Bit16s *)buffer->mAudioData, frameCount, nanosNow, framesInAudioBuffer);

	OSStatus res = AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
	if (res) qDebug() << "CoreAudio: AudioQueueEnqueueBuffer() failed with error code:" << res;
}

bool CoreAudioStream::start(const QString deviceUid) {
	if (audioQueue != NULL) {
		return true;
	}

	AudioStreamBasicDescription dataFormat = {(Float64)sampleRate, kAudioFormatLinearPCM, kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsSignedInteger, 4, 1, 4, 2, 16, 0};
	OSStatus res = AudioQueueNewOutput(&dataFormat, renderOutputBuffer, this, NULL, NULL, 0, &audioQueue);
	if (res || audioQueue == NULL) {
		qDebug() << "CoreAudio: AudioQueueNewOutput() failed with error code:" << res;
		return false;
	}

	// Set audio output device
	if (deviceUid.isEmpty()) {
		qDebug() << "CoreAudio: Using default audio output device";
	} else {
		CFStringRef deviceUidRef = qStringToCFString(deviceUid);
		res = AudioQueueSetProperty(audioQueue, kAudioQueueProperty_CurrentDevice, &deviceUidRef, sizeof(CFStringRef));
		if (res) {
			qDebug() << "CoreAudio: AudioQueueSetProperty() failed with error code" << res;
			res = AudioQueueDispose(audioQueue, true);
			if (res) qDebug() << "CoreAudio: AudioQueueDispose() failed with error code" << res;
			audioQueue = NULL;
			return false;
		}
		qDebug() << "CoreAudio: Using audio output device:" << deviceUid;
		CFRelease(deviceUidRef);
	}

	for (uint i = 0; i < numberOfBuffers; i++) {
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
		renderOutputBuffer(this, NULL, buffers[i]);
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
	if (audioQueue == NULL) return;
	OSStatus res = AudioQueueDispose(audioQueue, true);
	if (res) qDebug() << "CoreAudio: AudioQueueDispose() failed with error code" << res;
	audioQueue = NULL;
}

CoreAudioDevice::CoreAudioDevice(CoreAudioDriver &driver, const QString uid, const QString name) :
	AudioDevice(driver, name), uid(uid) {}

AudioStream *CoreAudioDevice::startAudioStream(SynthRoute &synthRoute, const uint sampleRate) const {
	CoreAudioStream *stream = new CoreAudioStream(driver.getAudioSettings(), synthRoute, sampleRate);
	if (stream->start(uid)) {
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
	deviceList.append(new CoreAudioDevice(*this)); // default device

#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_VERSION_12_0
	AudioObjectPropertyElement addressElement = kAudioObjectPropertyElementMaster;
#else
	AudioObjectPropertyElement addressElement = kAudioObjectPropertyElementMain;
#endif

	// Get system output devices
	UInt32 propertySize = 0;
	int numDevices = 0;
	QVector<AudioDeviceID> deviceIDs;
	AudioObjectPropertyAddress propertyAddress;
	propertyAddress.mSelector = kAudioHardwarePropertyDevices;
	propertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
	propertyAddress.mElement = addressElement;

	if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &propertyAddress, 0, 0, &propertySize) == noErr) {
		numDevices = propertySize / sizeof(AudioDeviceID);
		deviceIDs.resize(numDevices);

		if (AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &propertySize, deviceIDs.data()) == noErr) {
			foreach (const AudioDeviceID &id, deviceIDs) {
				AudioObjectPropertyAddress deviceAddress;
				propertySize = 0;
				deviceAddress.mSelector = kAudioDevicePropertyStreams;
				deviceAddress.mScope = kAudioObjectPropertyScopeOutput;
				deviceAddress.mElement = addressElement;

				if (AudioObjectGetPropertyDataSize(id, &deviceAddress, 0, NULL, &propertySize) == noErr) {
					if (propertySize > 0) {
						CFStringRef devUidRef;
						propertySize = sizeof(CFStringRef);
						deviceAddress.mSelector = kAudioDevicePropertyDeviceUID;
						deviceAddress.mScope = kAudioObjectPropertyScopeGlobal;
						deviceAddress.mElement = addressElement;

						if (AudioObjectGetPropertyData(id, &deviceAddress, 0, NULL, &propertySize, &devUidRef) == noErr) {
							CFStringRef devNameRef;
							propertySize = sizeof(CFStringRef);
							deviceAddress.mSelector = kAudioDevicePropertyDeviceNameCFString;
							deviceAddress.mScope = kAudioObjectPropertyScopeGlobal;
							deviceAddress.mElement = addressElement;

							if (AudioObjectGetPropertyData(id, &deviceAddress, 0, NULL, &propertySize, &devNameRef) == noErr) {
								QString uid = cfStringToQString(devUidRef);
								QString name = cfStringToQString(devNameRef);
								deviceList.append(new CoreAudioDevice(*this, uid, name));
							}
							CFRelease(devNameRef);
						}
						CFRelease(devUidRef);
					}
				}
			}
		}
	}

	return deviceList;
}

void CoreAudioDriver::validateAudioSettings(AudioDriverSettings &useSettings) const {
	if (useSettings.audioLatency == 0) {
		useSettings.audioLatency = DEFAULT_AUDIO_LATENCY;
	}
	if (useSettings.chunkLen == 0) {
		useSettings.chunkLen = DEFAULT_CHUNK_MS;
	}
	if (useSettings.audioLatency < useSettings.chunkLen) {
		useSettings.chunkLen = useSettings.audioLatency;
	}
	if ((useSettings.midiLatency != 0) && (useSettings.midiLatency < useSettings.chunkLen)) {
		useSettings.midiLatency = useSettings.chunkLen;
	}
}
