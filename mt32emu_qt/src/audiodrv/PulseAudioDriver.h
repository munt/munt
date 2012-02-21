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
	unsigned int bufferSize;
	unsigned int audioLatency;
	unsigned int midiLatency;
	ClockSync clockSync;
	QSynth *synth;
	unsigned int sampleRate;
	MT32Emu::Bit16s *buffer;
	pa_simple *stream;
	qint64 sampleCount;
	bool pendingClose;
	bool useAdvancedTiming;

	static void* processingThread(void *);

public:
	PulseAudioStream(const AudioDevice *device, QSynth *useSynth, unsigned int useSampleRate);
	~PulseAudioStream();
	bool start();
	void close();
};

class PulseAudioDefaultDevice : public AudioDevice {
friend class PulseAudioDriver;
	PulseAudioDefaultDevice(PulseAudioDriver const * const driver);
public:
	PulseAudioStream *startAudioStream(QSynth *synth, unsigned int sampleRate) const;
};

class PulseAudioDriver : public AudioDriver {
private:
	bool isLibraryFound;
	void validateAudioSettings();
public:
	PulseAudioDriver(Master *useMaster);
	~PulseAudioDriver();
	QList<AudioDevice *> getDeviceList() const;
};

#endif
