#ifndef QT_AUDIO_DRIVER_H
#define QT_AUDIO_DRIVER_H

#include <QtCore>
#include <mt32emu/mt32emu.h>

#include "AudioDriver.h"
#include "../MasterClock.h"

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
	unsigned int midiLatency;
	unsigned int audioLatency;
	bool advancedTiming;

public:
	QtAudioStream(const AudioDevice *device, QSynth *useSynth, unsigned int useSampleRate);
	~QtAudioStream();
	bool start();
	void close();
};

class QtAudioDefaultDevice : public AudioDevice {
	friend class QtAudioDriver;
private:
	QtAudioDefaultDevice(QtAudioDriver * const driver);
public:
	QtAudioStream *startAudioStream(QSynth *synth, unsigned int sampleRate) const;
};

class QtAudioDriver : public AudioDriver {
private:
	void validateAudioSettings(AudioDriverSettings &settings) const;
public:
	QtAudioDriver(Master *useMaster);
	~QtAudioDriver();
	const QList<const AudioDevice *> createDeviceList();
};

#endif
