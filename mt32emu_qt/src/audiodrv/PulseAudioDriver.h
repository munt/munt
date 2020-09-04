#ifndef PULSE_AUDIO_DRIVER_H
#define PULSE_AUDIO_DRIVER_H

#include <QtCore>

#include <pulse/simple.h>

#include <mt32emu/mt32emu.h>

#include "AudioDriver.h"

class Master;
class SynthRoute;
class PulseAudioDriver;

class PulseAudioStream : public AudioStream {
private:
	MT32Emu::Bit16s *buffer;
	pa_simple *stream;
	uint bufferSize;
	pthread_t processingThreadID;
	volatile bool stopProcessing;

	static void *processingThread(void *);

public:
	PulseAudioStream(const AudioDriverSettings &settings, SynthRoute &synthRoute, const quint32 sampleRate);
	~PulseAudioStream();
	bool start();
	void close();
};

class PulseAudioDefaultDevice : public AudioDevice {
friend class PulseAudioDriver;
	PulseAudioDefaultDevice(PulseAudioDriver &driver);
public:
	AudioStream *startAudioStream(SynthRoute &synthRoute, const uint sampleRate) const;
};

class PulseAudioDriver : public AudioDriver {
private:
	bool isLibraryFound;
	void validateAudioSettings(AudioDriverSettings &settings) const;
public:
	PulseAudioDriver(Master *useMaster);
	~PulseAudioDriver();
	const QList<const AudioDevice *> createDeviceList();
};

#endif
