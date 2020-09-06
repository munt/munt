#ifndef JACK_AUDIO_DRIVER_H
#define JACK_AUDIO_DRIVER_H

#include <QtCore>

#include "AudioDriver.h"

typedef float JACKAudioSample;

class Master;
class SynthRoute;
class JACKClient;
class JACKAudioDriver;
class JACKAudioProcessor;
class MidiSession;

class JACKAudioStream : public AudioStream {
public:
	JACKAudioStream(const AudioDriverSettings &useSettings, SynthRoute &synthRoute, const quint32 useSampleRate);
	~JACKAudioStream();
	bool start(MidiSession *midiSession);
	void stop();
	bool checkSampleRate(quint32 sampleRate) const;
	void onJACKBufferSizeChange(const quint32 bufferSize);
	void onJACKShutdown();
	void renderStreams(const quint32 frameCount, JACKAudioSample *leftOutBuffer, JACKAudioSample *rightOutBuffer);

private:
	JACKClient * const jackClient;
	float *buffer;
	JACKAudioProcessor *processor;
	const quint32 configuredAudioLatencyFrames;
};

class JACKAudioDefaultDevice : public AudioDevice {
	friend class JACKAudioDriver;
public:
	static AudioStream *startAudioStream(const AudioDevice *audioDevice, SynthRoute &synthRoute, const uint sampleRate, MidiSession *midiSession);

	AudioStream *startAudioStream(SynthRoute &synthRoute, const uint sampleRate) const;

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
