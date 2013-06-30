#ifndef AUDIO_FILE_WRITER_DRIVER_H
#define AUDIO_FILE_WRITER_DRIVER_H

#include <QtCore>
#include <mt32emu/mt32emu.h>
#include "AudioDriver.h"
#include "../AudioFileWriter.h"

class Master;
class QSynth;
class AudioFileWriterDriver;
class AudioFileWriterDevice;

class AudioFileWriterStream : public AudioStream {
private:
	AudioFileWriter writer;
	QSynth *synth;
	unsigned int sampleRate;
	unsigned int bufferSize;
	MasterClockNanos latency;

public:
	AudioFileWriterStream(const AudioFileWriterDevice *device, QSynth *useSynth, unsigned int useSampleRate);
	bool start();
	void close();
};

class AudioFileWriterDevice : public AudioDevice {
friend class AudioFileWriterDriver;
private:
	AudioFileWriterDevice(AudioFileWriterDriver * const driver, QString useDeviceIndex, QString useDeviceName);
public:
	AudioFileWriterStream *startAudioStream(QSynth *synth, unsigned int sampleRate) const;
};

class AudioFileWriterDriver : public AudioDriver {
private:
	void validateAudioSettings(AudioDriverSettings &settings) const;
public:
	AudioFileWriterDriver(Master *useMaster);
	~AudioFileWriterDriver();
	const QList<const AudioDevice *> createDeviceList();
};

#endif
