#ifndef CORE_AUDIO_DRIVER_H
#define CORE_AUDIO_DRIVER_H

#include <AudioToolbox/AudioQueue.h>
#include <CoreAudio/AudioHardware.h>

#include "AudioDriver.h"

class SynthRoute;
class Master;
class CoreAudioDriver;

class CoreAudioStream : public AudioStream {
private:
	AudioQueueRef audioQueue;
	AudioQueueBufferRef *buffers;
	uint numberOfBuffers;
	uint bufferByteSize;

	static void renderOutputBuffer(void *userData, AudioQueueRef queue, AudioQueueBufferRef buffer);

public:
	CoreAudioStream(const AudioDriverSettings &settings, SynthRoute &synthRoute, const quint32 sampleRate);
	~CoreAudioStream();
	bool start(const QString deviceUid);
	void close();
};

class CoreAudioDevice : public AudioDevice {
friend class CoreAudioDriver;
private:
	const QString uid;

	CoreAudioDevice(CoreAudioDriver &driver, const QString uid = NULL, const QString name = "Default output device");

public:
	AudioStream *startAudioStream(SynthRoute &synthRoute, const uint sampleRate) const;
};

class CoreAudioDriver : public AudioDriver {
private:
	void validateAudioSettings(AudioDriverSettings &) const;

public:
	CoreAudioDriver(Master *useMaster);
	~CoreAudioDriver();
	const QList<const AudioDevice *> createDeviceList();
};

#endif
