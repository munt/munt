#ifndef AUDIO_FILE_WRITER_H
#define AUDIO_FILE_WRITER_H

#include <QtCore>
#include <mt32emu/mt32emu.h>
#include "AudioDriver.h"

class Master;
class QSynth;
class AudioFileWriter;
class AudioFileWriterDriver;
class AudioFileWriterDevice;

class AudioFileWriterStream : public AudioStream {
private:
	AudioFileWriter *writer;

public:
	AudioFileWriterStream(const AudioFileWriterDevice *device, QSynth *useSynth, unsigned int useSampleRate);
	~AudioFileWriterStream();
	bool start();
	void close();
};

class AudioFileWriterDevice : public AudioDevice {
friend class AudioFileWriterDriver;
private:
	AudioFileWriterDevice(const AudioFileWriterDriver * const driver, QString useDeviceIndex, QString useDeviceName);
public:
	AudioFileWriterStream *startAudioStream(QSynth *synth, unsigned int sampleRate) const;
};

class AudioFileWriterDriver : public AudioDriver {
private:
	void validateAudioSettings();
public:
	AudioFileWriterDriver(Master *useMaster);
	~AudioFileWriterDriver();
	QList<AudioDevice *> getDeviceList() const;
};

#endif
