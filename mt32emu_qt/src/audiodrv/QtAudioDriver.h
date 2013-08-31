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

public:
	QtAudioStream(const AudioDriverSettings &useSettings, QSynth &useSynth, const quint32 useSampleRate);
	~QtAudioStream();
	bool start();
	void close();
};

class QtAudioDefaultDevice : public AudioDevice {
	friend class QtAudioDriver;
private:
	QtAudioDefaultDevice(QtAudioDriver &driver);
public:
	QtAudioStream *startAudioStream(QSynth &synth, const uint sampleRate) const;
};

class QtAudioDriver : public AudioDriver {
private:
	void validateAudioSettings(AudioDriverSettings &settings) const;
public:
	QtAudioDriver(Master *useMaster);
	const QList<const AudioDevice *> createDeviceList();
};

#endif
