#ifndef PORT_AUDIO_DRIVER_H
#define PORT_AUDIO_DRIVER_H

#include <QtCore>

#include <portaudio.h>

#include <mt32emu/mt32emu.h>

#include "AudioDriver.h"
#include "../ClockSync.h"

class QSynth;
class Master;
class PortAudioDriver;
class PortAudioDevice;

class PortAudioStream : public AudioStream {
private:
	QSynth *synth;
	unsigned int sampleRate;
	PaStream *stream;

	ClockSync clockSync;
	// The total latency of audio stream buffers
	// Special value of 0 indicates PortAudio to use its own recommended latency value
	qint64 audioLatency;
	// The number of nanos by which to delay MIDI events to help ensure accurate relative timing.
	qint64 midiLatency;
	qint64 sampleCount;
	quint32 prevFrameCount;
	MasterClockNanos prevFirstSampleMasterClockNanos;
	bool useAdvancedTiming;

	static int paCallback(const void *inputBuffer, void *outputBuffer, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

public:
	PortAudioStream(const PortAudioDevice *device, QSynth *useSynth, unsigned int useSampleRate);
	~PortAudioStream();
	bool start(PaDeviceIndex deviceIndex);
	void close();
};

class PortAudioDevice : public AudioDevice {
friend class PortAudioDriver;
private:
	PaDeviceIndex deviceIndex;
	PortAudioDevice(const PortAudioDriver * const driver, int useDeviceIndex, QString useDeviceName);

public:
	PortAudioStream *startAudioStream(QSynth *synth, unsigned int sampleRate) const;
};

class PortAudioDriver : public AudioDriver {
private:
	void validateAudioSettings() {}
public:
	PortAudioDriver(Master *useMaster);
	~PortAudioDriver();
	QList<AudioDevice *> getDeviceList() const;
};

#endif
