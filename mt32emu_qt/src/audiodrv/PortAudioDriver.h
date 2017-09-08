#ifndef PORT_AUDIO_DRIVER_H
#define PORT_AUDIO_DRIVER_H

#include <QtCore>

#include <portaudio.h>

#include <mt32emu/mt32emu.h>

#include "AudioDriver.h"

class QSynth;
class Master;
class PortAudioDriver;
class PortAudioDevice;

class PortAudioStream : public AudioStream {
private:
	PaStream *stream;

	static int paCallback(const void *inputBuffer, void *outputBuffer, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

public:
	PortAudioStream(const AudioDriverSettings &settings, QSynth &synth, const quint32 sampleRate);
	~PortAudioStream();
	bool start(PaDeviceIndex deviceIndex);
	void close();
};

class PortAudioDevice : public AudioDevice {
friend class PortAudioDriver;
private:
	PaDeviceIndex deviceIndex;
	PortAudioDevice(PortAudioDriver &driver, int useDeviceIndex, QString useDeviceName);

public:
	AudioStream *startAudioStream(QSynth &synth, const uint sampleRate) const;
};

class PortAudioDriver : public AudioDriver {
private:
	void validateAudioSettings(AudioDriverSettings &) const {}
public:
	PortAudioDriver(Master *useMaster);
	~PortAudioDriver();
	const QList<const AudioDevice *> createDeviceList();
};

#endif
