#ifndef AUDIO_DRIVER_H
#define AUDIO_DRIVER_H

#include <QObject>
#include <QtGlobal>
#include <QList>
#include <QString>
#include <QMetaType>

#include <mt32emu/mt32emu.h>

#include "../MasterClock.h"

class AudioDriver;
class QSynth;
struct AudioDriverSettings;

class AudioStream {
protected:
	QSynth &synth;
	const quint32 sampleRate;
	const AudioDriverSettings &settings;
	quint32 audioLatencyFrames;
	quint32 midiLatencyFrames;

	quint64 renderedFramesCount;
	quint64 lastEstimatedPlayedFramesCount;
	bool resetScheduled;

	struct {
		MasterClockNanos lastPlayedNanos;
		quint64 lastPlayedFramesCount;
		double actualSampleRate;
	} timeInfo[2];
	volatile uint timeInfoIx;

	void updateTimeInfo(const MasterClockNanos measuredNanos, const quint32 framesInAudioBuffer);
	bool isAutoLatencyMode() const;

public:
	AudioStream(const AudioDriverSettings &settings, QSynth &synth, const quint32 sampleRate);
	virtual ~AudioStream() {}
	virtual quint64 estimateMIDITimestamp(const MasterClockNanos refNanos = 0);
};

class AudioDevice {
public:
	AudioDriver &driver;
	const QString name;

	AudioDevice(AudioDriver &driver, const QString name);
	virtual ~AudioDevice() {}
	virtual AudioStream *startAudioStream(QSynth &synth, const uint sampleRate) const = 0;
};

Q_DECLARE_METATYPE(const AudioDevice *)

struct AudioDriverSettings {
	// The sample rate to use for instances of AudioStream being created
	unsigned int sampleRate;
	// The quality of sample rate conversion if applicable
	MT32Emu::SamplerateConversionQuality srcQuality;
	// The maximum number of milliseconds to render at once
	unsigned int chunkLen;
	// The total latency of audio stream buffers in milliseconds
	unsigned int audioLatency;
	// The number of milliseconds by which to delay MIDI events to ensure accurate relative timing
	unsigned int midiLatency;
	// true - use advanced timing functions provided by audio API
	// false - instead, rely on count of rendered samples to compute average actual sample rate
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

	static void migrateAudioSettingsFromVersion1();

	AudioDriver(QString useID, QString useName);
	virtual ~AudioDriver() {}
	virtual const QList<const AudioDevice *> createDeviceList() = 0;
	virtual const AudioDriverSettings &getAudioSettings() const;
	virtual void setAudioSettings(AudioDriverSettings &useSettings);
};

#endif
