#ifndef JACK_AUDIO_DRIVER_H
#define JACK_AUDIO_DRIVER_H

#include <QtCore>

#include "AudioDriver.h"

typedef float JACKAudioSample;

class Master;
class QSynth;
class JACKClient;
class JACKAudioDriver;

class JACKAudioStream : public AudioStream {
public:
	JACKAudioStream(const AudioDriverSettings &useSettings, QSynth &useSynth, const quint32 useSampleRate, QSharedPointer<JACKClient> &jackClient);
	~JACKAudioStream();
	bool start();
	void stop();
	void onJACKShutdown();
	void renderStreams(const quint32 frameCount, JACKAudioSample *leftOutBuffer, JACKAudioSample *rightOutBuffer);

private:
	QSharedPointer<JACKClient> jackClient;
	float *buffer;
};

class JACKAudioDefaultDevice : public AudioDevice {
	friend class JACKAudioDriver;
public:
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
