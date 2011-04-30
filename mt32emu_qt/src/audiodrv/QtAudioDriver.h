#ifndef QT_AUDIO_DRIVER_H
#define QT_AUDIO_DRIVER_H

#include <QtCore>
#include <QtMultimediaKit/QAudioOutput>
#include <mt32emu/mt32emu.h>

#include "AudioDriver.h"

class WaveGenerator;
class QSynth;

class QtAudioDriver : public AudioDriver {
	friend class WaveGenerator;
private:
	QAudioOutput *audioOutput;
	WaveGenerator *waveGenerator;

	unsigned int sampleRate;

	// The number of nanos by which to delay (MIDI) events to help ensure accurate relative timing.
	qint64 latency;

	qint64 getPlayedAudioNanosPlusLatency();

public:
	QtAudioDriver(QSynth *useSynth, unsigned int useSampleRate);
	~QtAudioDriver();
	bool start(int deviceIndex);
	void close();
};

#endif
