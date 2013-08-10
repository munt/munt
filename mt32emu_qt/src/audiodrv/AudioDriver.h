#ifndef AUDIO_DRIVER_H
#define AUDIO_DRIVER_H

#include <QObject>
#include <QtGlobal>
#include <QList>
#include <QString>
#include <QMetaType>

#include "../MasterClock.h"

class AudioDriver;
class QSynth;

class AudioStream {
protected:
	static double estimateActualSampleRate(const double sampleRate, MasterClockNanos &firstSampleMasterClockNanos, MasterClockNanos &lastSampleMasterClockNanos, const MasterClockNanos audioLatency, const quint32 frameCount);

public:
	//virtual void suspend() = 0;
	//virtual void unsuspend() = 0;
	virtual ~AudioStream() {};
	virtual bool estimateMIDITimestamp(quint32 &timestamp, const MasterClockNanos refNanos = 0) {return false;}
};

class AudioDevice {
public:
	AudioDriver * const driver;
	// id must be unique within the driver and as permanent as possible -
	// it will be stored and retrieved from settings.
	const QString id;
	const QString name;
	AudioDevice(AudioDriver *driver, QString id, QString name);
	virtual ~AudioDevice() {};
	virtual AudioStream *startAudioStream(QSynth *synth, unsigned int sampleRate) const = 0;
};

Q_DECLARE_METATYPE(const AudioDevice *);

struct AudioDriverSettings {
	// The maximum number of milliseconds to render at once
	unsigned int chunkLen;
	// The total latency of audio stream buffers in milliseconds
	unsigned int audioLatency;
	// The number of milliseconds by which to delay MIDI events to ensure accurate relative timing
	unsigned int midiLatency;
	// true - use advanced timing functions provided by audio API
	// false - instead, compute average actual sample rate using clockSync
	bool advancedTiming;
};

class AudioDriver {
protected:
	// settings holds current driver settings in effect
	AudioDriverSettings settings;

	virtual void loadAudioSettings();
	virtual void validateAudioSettings(AudioDriverSettings &settings) const = 0;

public:
	// id must be unique within the application and permanent -
	// it will be stored and retrieved from settings. Preferably lowercase.
	const QString id;
	// name is the English human-readable name to be used in the GUI.
	const QString name;

	AudioDriver(QString useID, QString useName);
	virtual ~AudioDriver() {};
	virtual const QList<const AudioDevice *> createDeviceList() = 0;
	virtual const AudioDriverSettings &getAudioSettings() const;
	virtual void setAudioSettings(AudioDriverSettings &useSettings);
};

#endif
