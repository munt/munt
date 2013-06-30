#ifndef OSS_AUDIO_DRIVER_H
#define OSS_AUDIO_DRIVER_H

#include <QtCore>

#include <mt32emu/mt32emu.h>

#include "AudioDriver.h"
#include "../ClockSync.h"

class Master;
class QSynth;
class OSSAudioDriver;

class OSSAudioStream : public AudioStream {
private:
	unsigned int bufferSize;
	unsigned int audioLatency;
	unsigned int midiLatency;
	ClockSync clockSync;
	QSynth *synth;
	unsigned int sampleRate;
	MT32Emu::Bit16s *buffer;
	int stream;
	qint64 sampleCount;
	bool pendingClose;
	bool useAdvancedTiming;

	static void* processingThread(void *);

public:
	OSSAudioStream(const AudioDevice *device, QSynth *useSynth, unsigned int useSampleRate);
	~OSSAudioStream();
	bool start();
	void stop();
};

class OSSAudioDefaultDevice : public AudioDevice {
friend class OSSAudioDriver;
	OSSAudioDefaultDevice(OSSAudioDriver * const driver);
public:
	OSSAudioStream *startAudioStream(QSynth *synth, unsigned int sampleRate) const;
};

class OSSAudioDriver : public AudioDriver {
private:
	void validateAudioSettings(AudioDriverSettings &settings) const;
public:
	OSSAudioDriver(Master *useMaster);
	~OSSAudioDriver();
	const QList<const AudioDevice *> createDeviceList();
};

#endif
