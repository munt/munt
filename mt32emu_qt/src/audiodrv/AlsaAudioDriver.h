#ifndef ALSA_AUDIO_DRIVER_H
#define ALSA_AUDIO_DRIVER_H

#include <QtCore>

#include <alsa/asoundlib.h>

#include <mt32emu/mt32emu.h>

#include "AudioDriver.h"
#include "../ClockSync.h"

class Master;
class QSynth;
class AlsaAudioDriver;

class AlsaAudioStream : public AudioStream {
private:
	MT32Emu::Bit16s *buffer;
	snd_pcm_t *stream;
	uint bufferSize;
	bool pendingClose;

	static void *processingThread(void *);

public:
	AlsaAudioStream(const AudioDriverSettings &settings, QSynth &synth, const quint32 sampleRate);
	~AlsaAudioStream();
	bool start();
	void close();
};

class AlsaAudioDefaultDevice : public AudioDevice {
friend class AlsaAudioDriver;
	AlsaAudioDefaultDevice(AlsaAudioDriver &driver);
public:
	AudioStream *startAudioStream(QSynth &synth, const uint sampleRate) const;
};

class AlsaAudioDriver : public AudioDriver {
private:
	void validateAudioSettings(AudioDriverSettings &settings) const;
public:
	AlsaAudioDriver(Master *useMaster);
	const QList<const AudioDevice *> createDeviceList();
};

#endif
