#ifndef PORT_AUDIO_DRIVER_H
#define PORT_AUDIO_DRIVER_H

#include <QtCore>

#include <portaudio.h>

#include <mt32emu/mt32emu.h>

#include "AudioDriver.h"

class QSynth;

class PortAudioDriver : public AudioDriver {
private:
	QSynth *synth;
	unsigned int sampleRate;
	int currentDeviceIndex;
	PaStream *stream;
	// The number of nanos by which to delay (MIDI) events to help ensure accurate relative timing.
	qint64 latency;

	qint64 getPlayedAudioNanosPlusLatency();
	static int paCallback(const void *inputBuffer, void *outputBuffer, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

public:
	PortAudioDriver(QSynth *useSynth, unsigned int useSampleRate);
	~PortAudioDriver();
	bool start(int deviceIndex);
	void close();
	QList<QString> getDeviceNames();
};

#endif
