#ifndef AUDIO_DRIVER_H
#define AUDIO_DRIVER_H

#include <QtGlobal>
#include <QList>
#include <QString>
#include <QMetaType>

#include <mt32emu/mt32emu.h>

#include "../MasterClock.h"

class AudioDriver;
class SynthRoute;
struct AudioDriverSettings;

class AudioStream {
protected:
	SynthRoute &synthRoute;
	const quint32 sampleRate;
	const AudioDriverSettings &settings;
	quint32 audioLatencyFrames;
	quint32 midiLatencyFrames;

	quint64 lastEstimatedPlayedFramesCount;
	bool resetScheduled;

	// Note, renderedFramesCount and timeInfo are read from several MIDI receiving threads,
	// so they need a thread-safe publication tech. The two snapshots are written in turn,
	// so that while the change count stays intact, it is safe to read the snapshot
	// with the index equal to the last bit of the change count.

	quint64 renderedFramesCounts[2];
	QAtomicInt renderedFramesChangeCount;

	struct TimeInfo {
		MasterClockNanos lastPlayedNanos;
		quint64 lastPlayedFramesCount;
		double actualSampleRate;
	} timeInfos[2];
	QAtomicInt timeInfoChangeCount;

	void renderAndUpdateState(MT32Emu::Bit16s *buffer, const quint32 frameCount, const MasterClockNanos measuredNanos, const quint32 framesInAudioBuffer);
	void updateTimeInfo(const MasterClockNanos measuredNanos, const quint32 framesInAudioBuffer);
	bool isAutoLatencyMode() const;
	void framesRendered(quint32 frameCount);
	quint64 getRenderedFramesCount() const;

public:
	AudioStream(const AudioDriverSettings &settings, SynthRoute &synthRoute, const quint32 sampleRate);
	virtual ~AudioStream() {}
	virtual quint64 estimateMIDITimestamp(const MasterClockNanos refNanos);
	quint64 computeMIDITimestamp(uint relativeFrameTime) const;
};

class AudioDevice {
public:
	AudioDriver &driver;
	const QString name;

	AudioDevice(AudioDriver &driver, const QString name);
	virtual ~AudioDevice() {}
	virtual AudioStream *startAudioStream(SynthRoute &synthRoute, const uint sampleRate) const = 0;
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
