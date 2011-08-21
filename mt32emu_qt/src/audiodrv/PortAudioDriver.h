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
class PortAudioProcessor;
class PortAudioDevice;
class PortAudioStream;

class PortAudioDriver : public AudioDriver {
private:
	Master *master;
	PortAudioProcessor *processor;
	QThread processorThread;

	void start();
	void stop();

public:
	PortAudioDriver(Master *useMaster);
	~PortAudioDriver();
	Master *getMaster();
};

class PortAudioProcessor : public QObject {
	Q_OBJECT
public:
	PortAudioProcessor(PortAudioDriver *master);

	void stop();

public slots:
	void start();

private:
	PortAudioDriver *driver;
	volatile bool stopProcessing;
	QList<QString> getDeviceNames();

signals:
	void finished();
};


class PortAudioStream : public AudioStream {
friend class PortAudioDevice;
private:
	PortAudioDevice *device;
	QSynth *synth;
	unsigned int sampleRate;
	PaStream *stream;

	ClockSync clockSync;
	// The total latency of audio stream buffers
	// Special value of 0 indicates PortAudio to use its own recommended latency value
	qint64 audioLatency;
	// The number of nanos by which to delay (MIDI) events to help ensure accurate relative timing.
	qint64 latency;
	qint64 sampleCount;

	qint64 getPlayedAudioNanosPlusLatency();
	bool start();
	void close();

	static int paCallback(const void *inputBuffer, void *outputBuffer, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

public:
	PortAudioStream(PortAudioDevice *useDevice, QSynth *useSynth, unsigned int useSampleRate);
	~PortAudioStream();
};

class PortAudioDevice : public AudioDevice {
friend class PortAudioProcessor;
friend class PortAudioStream;
private:
	PaDeviceIndex deviceIndex;
	PortAudioDevice(PortAudioDriver *driver, int useDeviceIndex, QString useDeviceName);

public:
	PortAudioStream *startAudioStream(QSynth *synth, unsigned int sampleRate);
};

#endif
