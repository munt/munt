#ifndef QT_AUDIO_DRIVER_H
#define QT_AUDIO_DRIVER_H

#include <QtCore>
#include <mt32emu/mt32emu.h>

#include "AudioDriver.h"

class Master;
class WaveGenerator;
class QSynth;
class QAudioOutput;
class QtAudioDriver;

class QtAudioStream : public AudioStream {
	friend class WaveGenerator;
private:
	QAudioOutput *audioOutput;
	WaveGenerator *waveGenerator;

	unsigned int sampleRate;

	// The number of nanos by which to delay (MIDI) events to help ensure accurate relative timing.
	qint64 latency;

public:
	QtAudioStream(QSynth *useSynth, unsigned int useSampleRate);
	~QtAudioStream();
	bool start();
	void close();
};

class QtAudioDefaultDevice : public AudioDevice {
	friend class QtAudioDriver;
private:
	QtAudioDefaultDevice(QtAudioDriver *driver);
public:
	QtAudioStream *startAudioStream(QSynth *synth, unsigned int sampleRate);
};

class QtAudioDriver : public AudioDriver {
public:
	QtAudioDriver(Master *useMaster);
	~QtAudioDriver();
	QList<AudioDevice *> getDeviceList();
};

#endif
