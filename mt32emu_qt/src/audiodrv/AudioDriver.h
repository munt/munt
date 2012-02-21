#ifndef AUDIO_DRIVER_H
#define AUDIO_DRIVER_H

#include <QObject>
#include <QtGlobal>
#include <QList>
#include <QString>
#include <QMetaType>

class AudioDriver;
class QSynth;

class AudioStream {
public:
	//virtual void suspend() = 0;
	//virtual void unsuspend() = 0;
	virtual ~AudioStream() {};
};

class AudioDevice {
public:
	const AudioDriver *driver;
	// id must be unique within the driver and as permanent as possible -
	// it will be stored and retrieved from settings.
	const QString id;
	const QString name;
	AudioDevice(const AudioDriver * const driver, QString id, QString name);
	virtual ~AudioDevice() {};
	virtual AudioStream *startAudioStream(QSynth *synth, unsigned int sampleRate) const = 0;
};

Q_DECLARE_METATYPE(AudioDevice*);

class AudioDriver {
protected:
	// The maximum number of milliseconds to render at once
	unsigned int chunkLen;
	// The total latency of audio stream buffers in milliseconds
	unsigned int audioLatency;
	// The number of milliseconds by which to delay MIDI events to ensure accurate relative timing
	unsigned int midiLatency;
	// true - use advanced timing functions provided by audio API
	// false - instead, compute average actual sample rate using clockSync
	bool advancedTiming;

	virtual void loadAudioSettings();
	virtual void validateAudioSettings() = 0;

public:
	// id must be unique within the application and permanent -
	// it will be stored and retrieved from settings. Preferably lowercase.
	const QString id;
	// name is the English human-readable name to be used in the GUI.
	const QString name;
	AudioDriver(QString useID, QString useName);
	virtual ~AudioDriver() {};
	virtual QList<AudioDevice *> getDeviceList() const = 0;
	virtual void getAudioSettings(unsigned int *chunkLen, unsigned int *audioLatency, unsigned int *midiLatency, bool *advancedTiming) const;
	virtual void setAudioSettings(unsigned int *chunkLen, unsigned int *audioLatency, unsigned int *midiLatency, bool *advancedTiming);
};

#endif
