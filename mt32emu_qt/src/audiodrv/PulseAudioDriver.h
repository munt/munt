#ifndef PULSE_AUDIO_DRIVER_H
#define PULSE_AUDIO_DRIVER_H

#include <QtCore>

#include <pulse/simple.h>

#include <mt32emu/mt32emu.h>

#include "AudioDriver.h"
#include "../ClockSync.h"

class Master;
class QSynth;
class PulseAudioDriver;

class PulseAudioStream : public AudioStream {
private:
	ClockSync clockSync;
	QSynth *synth;
	unsigned int sampleRate;
	int currentDeviceIndex;
	MT32Emu::Bit16s *buffer;
	pa_simple *stream;
	// The number of nanos by which to delay (MIDI) events to help ensure accurate relative timing.
	qint64 sampleCount;
	bool pendingClose;

	qint64 getPlayedAudioNanosPlusLatency();
	static void* processingThread(void *);

public:
	PulseAudioStream(QSynth *useSynth, unsigned int useSampleRate);
	~PulseAudioStream();
	bool start();
	void close();
};

class PulseAudioDefaultDevice : public AudioDevice {
friend class PulseAudioDriver;
	PulseAudioDefaultDevice(PulseAudioDriver *driver);
public:
	PulseAudioStream *startAudioStream(QSynth *synth, unsigned int sampleRate);
};

class PulseAudioDriver : public AudioDriver {
public:
	PulseAudioDriver(Master *useMaster);
	~PulseAudioDriver();
};

#endif
