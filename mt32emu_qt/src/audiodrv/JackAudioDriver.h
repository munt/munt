#ifndef JACK_AUDIO_DRIVER_H
#define JACK_AUDIO_DRIVER_H

#include <jack/jack.h>

#include "AudioDriver.h"

#define NUM_OF_CHANNELS 2

class Master;
class QSynth;
class JackAudioDriver;

class JackAudioStream : public AudioStream {
private:
	MT32Emu::Bit16s *buffer;
	jack_client_t *client;
	jack_port_t *port[NUM_OF_CHANNELS];

	static int processingThread(jack_nframes_t nframes, void *userData);
	static int allocateBuffer(jack_nframes_t nframes, void *userData);
public:
	JackAudioStream(const AudioDriverSettings &settings, QSynth &synth, const quint32 sampleRate);
	~JackAudioStream();
	bool start();
	void close();
};

class JackAudioDevice : public AudioDevice {
friend class JackAudioDriver;
private:
	JackAudioDevice(JackAudioDriver &driver);
public:
	AudioStream *startAudioStream(QSynth &synth, const uint sampleRate) const;
};

class JackAudioDriver : public AudioDriver {
private:
	void validateAudioSettings(AudioDriverSettings &settings) const;

public:
	JackAudioDriver(Master *useMaster);
	~JackAudioDriver();
	const QList<const AudioDevice *> createDeviceList();
};

#endif
