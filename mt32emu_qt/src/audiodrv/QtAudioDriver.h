#ifndef QT_AUDIO_DRIVER_H
#define QT_AUDIO_DRIVER_H

#include <QtCore>
#include <mt32emu/mt32emu.h>

#include "AudioDriver.h"
#include "../MasterClock.h"

class Master;
class WaveGenerator;
class SynthRoute;
class QAudioOutput;
class QAudioSink;
class QtAudioDriver;

class QtAudioStream : public AudioStream {
	friend class WaveGenerator;
private:
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	QAudioOutput *audioOutput;
#else
	QAudioSink *audioOutput;
#endif
	QThread *processingThread;
	WaveGenerator *waveGenerator;

public:
	QtAudioStream(const AudioDriverSettings &useSettings, SynthRoute &useSynthRoute, const quint32 useSampleRate);
	~QtAudioStream();
	void start();
	void close();
};

class QtAudioDefaultDevice : public AudioDevice {
	friend class QtAudioDriver;
private:
	QtAudioDefaultDevice(QtAudioDriver &driver);
public:
	AudioStream *startAudioStream(SynthRoute &synthRoute, const uint sampleRate) const;
};

class QtAudioDriver : public AudioDriver {
private:
	void validateAudioSettings(AudioDriverSettings &settings) const;
public:
	QtAudioDriver(Master *useMaster);
	const QList<const AudioDevice *> createDeviceList();
};

#endif
