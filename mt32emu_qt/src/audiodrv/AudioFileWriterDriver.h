#ifndef AUDIO_FILE_WRITER_DRIVER_H
#define AUDIO_FILE_WRITER_DRIVER_H

#include <QtCore>
#include <mt32emu/mt32emu.h>
#include "AudioDriver.h"
#include "../AudioFileWriter.h"

class Master;
class SynthRoute;
class AudioFileWriterDriver;
class AudioFileWriterDevice;

class AudioFileWriterStream : public AudioStream {
private:
	AudioFileRenderer writer;

public:
	AudioFileWriterStream(const AudioDriverSettings &settings, SynthRoute &useSynthRoute, const quint32 useSampleRate);
	quint64 estimateMIDITimestamp(const MasterClockNanos refNanos);
	bool start();
	void audioStreamFailed();
	void render(qint16 *buffer, uint frameCount);
};

class AudioFileWriterDevice : public AudioDevice {
friend class AudioFileWriterDriver;
private:
	AudioFileWriterDevice(AudioFileWriterDriver &driver, QString useDeviceName);
public:
	AudioStream *startAudioStream(SynthRoute &synthRoute, const uint sampleRate) const;
};

class AudioFileWriterDriver : public AudioDriver {
private:
	void validateAudioSettings(AudioDriverSettings &settings) const;
public:
	AudioFileWriterDriver(Master *useMaster);
	const QList<const AudioDevice *> createDeviceList();
};

#endif
