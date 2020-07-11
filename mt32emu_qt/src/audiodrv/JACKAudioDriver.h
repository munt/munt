#ifndef JACK_AUDIO_DRIVER_H
#define JACK_AUDIO_DRIVER_H

#include <QtCore>

#include "AudioDriver.h"

typedef float JACKAudioSample;

class Master;
class QSynth;
class JACKClient;
class JACKAudioDriver;
class JACKAudioProcessor;
class MidiSession;

class JACKAudioStream : public AudioStream {
public:
	JACKAudioStream(const AudioDriverSettings &useSettings, QSynth &useSynth, const quint32 useSampleRate);
	~JACKAudioStream();
	bool start(MidiSession *midiSession);
	void stop();
	void onJACKShutdown();
	void renderStreams(const quint32 frameCount, JACKAudioSample *leftOutBuffer, JACKAudioSample *rightOutBuffer);
	quint64 computeMIDITimestamp(const quint32 jackBufferFrameTime) const;

private:
	JACKClient * const jackClient;
	float *buffer;
	JACKAudioProcessor *processor;
};

class JACKAudioDefaultDevice : public AudioDevice {
	friend class JACKAudioDriver;
public:
	static AudioStream *startAudioStream(const AudioDevice *audioDevice, QSynth &synth, const uint sampleRate, MidiSession *midiSession);

	AudioStream *startAudioStream(QSynth &synth, const uint sampleRate) const;

private:
	JACKAudioDefaultDevice(JACKAudioDriver &driver);
};

class JACKAudioDriver : public AudioDriver {
public:
	JACKAudioDriver(Master *useMaster);
	const QList<const AudioDevice *> createDeviceList();

private:
	void validateAudioSettings(AudioDriverSettings &settings) const;
};

#endif
