#ifndef ALSA_AUDIO_DRIVER_H
#define ALSA_AUDIO_DRIVER_H

#include <QtCore>

#include <asoundlib.h>

#include <mt32emu/mt32emu.h>

#include "AudioDriver.h"
#include "../ClockSync.h"

class Master;
class QSynth;
class AlsaAudioDriver;

class AlsaAudioStream : public AudioStream {
private:
	unsigned int bufferSize;
	unsigned int audioLatency;
	unsigned int midiLatency;
	ClockSync clockSync;
	QSynth *synth;
	unsigned int sampleRate;
	MT32Emu::Bit16s *buffer;
	snd_pcm_t *stream;
	qint64 sampleCount;
	bool pendingClose;

	static void* processingThread(void *);

public:
	AlsaAudioStream(const AudioDevice *device, QSynth *useSynth, unsigned int useSampleRate);
	~AlsaAudioStream();
	bool start();
	void close();
};

class AlsaAudioDefaultDevice : public AudioDevice {
friend class AlsaAudioDriver;
	AlsaAudioDefaultDevice(AlsaAudioDriver const * const driver);
public:
	AlsaAudioStream *startAudioStream(QSynth *synth, unsigned int sampleRate) const;
};

class AlsaAudioDriver : public AudioDriver {
private:
	void validateAudioSettings();
public:
	AlsaAudioDriver(Master *useMaster);
	~AlsaAudioDriver();
	QList<AudioDevice *> getDeviceList() const;
};

#endif
