#ifndef ALSA_AUDIO_DRIVER_H
#define ALSA_AUDIO_DRIVER_H

#include <QtCore>

#include <alsa/asoundlib.h>

#include <mt32emu/mt32emu.h>

#include "AudioDriver.h"

class Master;
class SynthRoute;
class AlsaAudioDriver;

class AlsaAudioStream : public AudioStream {
private:
	MT32Emu::Bit16s *buffer;
	snd_pcm_t *stream;
	uint bufferSize;
	pthread_t processingThreadID;
	volatile bool stopProcessing;

	static void *processingThread(void *);

public:
	AlsaAudioStream(const AudioDriverSettings &settings, SynthRoute &synthRoute, const quint32 sampleRate);
	~AlsaAudioStream();
	bool start(const char *deviceID);
	void close();
};

class AlsaAudioDevice : public AudioDevice {
friend class AlsaAudioDriver;
private:
	const char *deviceID;

	AlsaAudioDevice(AlsaAudioDriver &driver, const char *useDeviceID, const QString name);

public:
	AudioStream *startAudioStream(SynthRoute &synthRoute, const uint sampleRate) const;
};

class AlsaAudioDriver : public AudioDriver {
private:
	void validateAudioSettings(AudioDriverSettings &settings) const;

public:
	AlsaAudioDriver(Master *useMaster);
	const QList<const AudioDevice *> createDeviceList();
};

#endif
