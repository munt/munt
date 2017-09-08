#ifndef CORE_AUDIO_DRIVER_H
#define CORE_AUDIO_DRIVER_H

#include <AudioToolbox/AudioQueue.h>

#include "AudioDriver.h"

class QSynth;
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
	CoreAudioStream(const AudioDriverSettings &settings, QSynth &synth, const quint32 sampleRate);
	~CoreAudioStream();
	bool start();
	void close();
};

class CoreAudioDevice : public AudioDevice {
friend class CoreAudioDriver;
private:
	CoreAudioDevice(CoreAudioDriver &driver);

public:
	AudioStream *startAudioStream(QSynth &synth, const uint sampleRate) const;
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
