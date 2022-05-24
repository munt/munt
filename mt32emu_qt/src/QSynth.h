#ifndef QSYNTH_H
#define QSYNTH_H

#include <QtCore>
#include <mt32emu/mt32emu.h>

#if !MT32EMU_IS_COMPATIBLE(2, 7)
#error Incompatible mt32emu library version
#endif

class AudioFileWriter;
class RealtimeHelper;
class QSynth;

enum SynthState {
	SynthState_CLOSED,
	SynthState_OPEN,
	SynthState_CLOSING
};

enum ReverbCompatibilityMode {
	ReverbCompatibilityMode_DEFAULT,
	ReverbCompatibilityMode_MT32,
	ReverbCompatibilityMode_CM32L
};

enum DisplayCompatibilityMode {
	DisplayCompatibilityMode_DEFAULT,
	DisplayCompatibilityMode_OLD_MT32,
	DisplayCompatibilityMode_NEW_MT32
};

struct SynthProfile {
	QDir romDir;
	QString controlROMFileName;
	QString controlROMFileName2;
	QString pcmROMFileName;
	QString pcmROMFileName2;
	MT32Emu::DACInputMode emuDACInputMode;
	MT32Emu::MIDIDelayMode midiDelayMode;
	MT32Emu::AnalogOutputMode analogOutputMode;
	MT32Emu::RendererType rendererType;
	int partialCount;
	ReverbCompatibilityMode reverbCompatibilityMode;
	float outputGain;
	float reverbOutputGain;
	bool reverbEnabled;
	bool reverbOverridden;
	int reverbMode;
	int reverbTime;
	int reverbLevel;
	bool reversedStereoEnabled;
	bool engageChannel1OnOpen;
	bool niceAmpRamp;
	bool nicePanning;
	bool nicePartialMixing;
	DisplayCompatibilityMode displayCompatibilityMode;
};

struct SoundGroup {
	struct Item {
		uint timbreGroup : 2;
		uint timbreNumber : 6;
		QString timbreName;
	};

	QString name;
	QVector<Item> constituents;
};

Q_DECLARE_METATYPE(SoundGroup::Item)

class QReportHandler : public QObject, public MT32Emu::ReportHandler2 {
	Q_OBJECT

// For the sake of Qt4 compatibility.
friend class RealtimeHelper;

public:
	QReportHandler(QSynth *qsynth);
	void printDebug(const char *fmt, va_list list);
	void showLCDMessage(const char *message);
	void onErrorControlROM();
	void onErrorPCMROM();
	void onDeviceReconfig();
	void onDeviceReset();
	void onNewReverbMode(MT32Emu::Bit8u mode);
	void onNewReverbTime(MT32Emu::Bit8u time);
	void onNewReverbLevel(MT32Emu::Bit8u level);
	void onPolyStateChanged(MT32Emu::Bit8u partNum);
	void onProgramChanged(MT32Emu::Bit8u partNum, const char soundGroupName[], const char patchName[]);
	void onLCDStateUpdated();
	void onMidiMessageLEDStateUpdated(bool ledState);
	void doShowLCDMessage(const char *message);

private:
	QSynth *qSynth() { return (QSynth *)parent(); }

signals:
	void balloonMessageAppeared(const QString &title, const QString &text);
	void masterVolumeChanged(int);
	void reverbModeChanged(int);
	void reverbTimeChanged(int);
	void reverbLevelChanged(int);
	void polyStateChanged(int);
	void programChanged(int, QString, QString);
	void lcdStateChanged();
	void midiMessageLEDStateChanged(bool);
};

/**
 * This is a wrapper for MT32Emu::Synth that provides a binding between the MT32Emu and Qt APIs
 * as well as adds certain thread-safety measures, namely support for MIDI messages to be safely
 * pushed from any number of threads, rendering of output audio samples in a separate thread
 * with concurrent changes of synth parameters from the main (GUI) thread, etc.
 */
class QSynth : public QObject {
	Q_OBJECT

friend class QReportHandler;
friend class RealtimeHelper;

private:
	volatile SynthState state;

	QMutex * const midiMutex;
	QMutex * const synthMutex;

	QDir romDir;
	QString controlROMFileName;
	QString controlROMFileName2;
	QString pcmROMFileName;
	QString pcmROMFileName2;
	const MT32Emu::ROMImage *controlROMImage;
	const MT32Emu::ROMImage *pcmROMImage;
	int reverbMode;
	int reverbTime;
	int reverbLevel;
	int partialCount;
	MT32Emu::AnalogOutputMode analogOutputMode;
	ReverbCompatibilityMode reverbCompatibilityMode;
	bool engageChannel1OnOpen;
	DisplayCompatibilityMode displayCompatibilityMode;

	MT32Emu::Synth *synth;
	QReportHandler reportHandler;
	QString synthProfileName;

	MT32Emu::SampleRateConverter *sampleRateConverter;
	AudioFileWriter *audioRecorder;

	RealtimeHelper *realtimeHelper;

	void setState(SynthState newState);
	void freeROMImages();
	MT32Emu::Bit32u convertOutputToSynthTimestamp(quint64 timestamp) const;

public:
	explicit QSynth(QObject *parent = NULL);
	~QSynth();
	void createSynth();
	bool isOpen() const;
	bool open(uint &targetSampleRate, MT32Emu::SamplerateConversionQuality srcQuality = MT32Emu::SamplerateConversionQuality_GOOD, const QString useSynthProfileName = "");
	void close();
	void reset() const;
	bool isRealtime() const;
	void enableRealtime();

	void flushMIDIQueue() const;
	void playMIDIShortMessageNow(MT32Emu::Bit32u msg) const;
	void playMIDISysexNow(const MT32Emu::Bit8u *sysex, MT32Emu::Bit32u sysexLen) const;
	bool playMIDIShortMessage(MT32Emu::Bit32u msg, quint64 timestamp) const;
	bool playMIDISysex(const MT32Emu::Bit8u *sysex, MT32Emu::Bit32u sysexLen, quint64 timestamp) const;
	void render(MT32Emu::Bit16s *buffer, uint length);
	void render(float *buffer, uint length);

	const QReportHandler *getReportHandler() const;

	void getSynthProfile(SynthProfile &synthProfile) const;
	void setSynthProfile(const SynthProfile &synthProfile, QString useSynthProfileName);

	void getROMImages(const MT32Emu::ROMImage *&controlROMImage, const MT32Emu::ROMImage *&pcmROMImage) const;

	void setMasterVolume(int masterVolume);
	void setOutputGain(float outputGain);
	void setReverbOutputGain(float reverbOutputGain);
	void setReverbEnabled(bool reverbEnabled);
	void setReverbOverridden(bool reverbOverridden);
	void setReverbSettings(int reverbMode, int reverbTime, int reverbLevel);
	void setPartVolumeOverride(uint partNumber, uint volumeOverride);
	void setReversedStereoEnabled(bool enabled);
	void setNiceAmpRampEnabled(bool enabled);
	void setNicePanningEnabled(bool enabled);
	void setNicePartialMixingEnabled(bool enabled);
	void resetMIDIChannelsAssignment(bool engageChannel1);
	void setInitialMIDIChannelsAssignment(bool engageChannel1);
	void setReverbCompatibilityMode(ReverbCompatibilityMode reverbCompatibilityMode);
	void setMIDIDelayMode(MT32Emu::MIDIDelayMode midiDelayMode);
	void setDACInputMode(MT32Emu::DACInputMode emuDACInputMode);
	void setAnalogOutputMode(MT32Emu::AnalogOutputMode analogOutputMode);
	void setRendererType(MT32Emu::RendererType useRendererType);
	void setPartialCount(int partialCount);
	const QString getPatchName(int partNum) const;
	void setTimbreOnPart(uint partNumber, uint timbreGroup, uint timbreNumber);
	void getSoundGroups(QVector<SoundGroup> &) const;
	void getPartialStates(MT32Emu::PartialState *partialStates) const;
	uint getPlayingNotes(uint partNumber, MT32Emu::Bit8u *keys, MT32Emu::Bit8u *velocities) const;
	uint getPartialCount() const;
	uint getSynthSampleRate() const;
	bool isActive() const;
	bool getDisplayState(char *targetBuffer) const;
	void setMainDisplayMode();
	void setDisplayCompatibilityMode(DisplayCompatibilityMode displayCompatibilityMode);

	void startRecordingAudio(const QString &fileName);
	void stopRecordingAudio();
	bool isRecordingAudio() const;

signals:
	void stateChanged(SynthState);
	void audioBlockRendered();
};

#endif
