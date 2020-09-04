#ifndef OSS_AUDIO_DRIVER_H
#define OSS_AUDIO_DRIVER_H

#include <QtCore>

#include <mt32emu/mt32emu.h>

#include "AudioDriver.h"

class Master;
class SynthRoute;
class OSSAudioDriver;

class OSSAudioStream : public AudioStream {
private:
	MT32Emu::Bit16s *buffer;
	int stream;
	uint bufferSize;
	pthread_t processingThreadID;
	volatile bool stopProcessing;

	static void *processingThread(void *);

public:
	OSSAudioStream(const AudioDriverSettings &settings, SynthRoute &synthRoute, const quint32 sampleRate);
	~OSSAudioStream();
	bool start();
	void stop();
};

class OSSAudioDefaultDevice : public AudioDevice {
friend class OSSAudioDriver;
	OSSAudioDefaultDevice(OSSAudioDriver &driver);
public:
	AudioStream *startAudioStream(SynthRoute &synthRoute, const uint sampleRate) const;
};

class OSSAudioDriver : public AudioDriver {
private:
	void validateAudioSettings(AudioDriverSettings &settings) const;
public:
	OSSAudioDriver(Master *useMaster);
	const QList<const AudioDevice *> createDeviceList();
};

#endif
