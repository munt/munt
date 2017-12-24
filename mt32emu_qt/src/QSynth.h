#ifndef QSYNTH_H
#define QSYNTH_H

#include <QtCore>
#include <mt32emu/mt32emu.h>

class AudioFileWriter;

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

struct SynthProfile {
	QDir romDir;
	QString controlROMFileName;
	QString pcmROMFileName;
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
};

class QReportHandler : public QObject, public MT32Emu::ReportHandler {
	Q_OBJECT

public:
	QReportHandler(QObject *parent = NULL);
	void printDebug(const char *fmt, va_list list);
	void showLCDMessage(const char *message);
	void onErrorControlROM();
	void onErrorPCMROM();
	void onMIDIMessagePlayed();
	void onDeviceReconfig();
	void onDeviceReset();
	void onNewReverbMode(MT32Emu::Bit8u mode);
	void onNewReverbTime(MT32Emu::Bit8u time);
	void onNewReverbLevel(MT32Emu::Bit8u level);
	void onPolyStateChanged(MT32Emu::Bit8u partNum);
	void onProgramChanged(MT32Emu::Bit8u partNum, const char soundGroupName[], const char patchName[]);

signals:
	void balloonMessageAppeared(const QString &title, const QString &text);
	void lcdMessageDisplayed(const QString);
	void midiMessagePlayed();
	void masterVolumeChanged(int);
	void reverbModeChanged(int);
	void reverbTimeChanged(int);
	void reverbLevelChanged(int);
	void polyStateChanged(int);
	void programChanged(int, QString, QString);
};

class QSynth : public QObject {
	Q_OBJECT

friend class QReportHandler;

private:
	volatile SynthState state;

	QMutex midiMutex;
	QMutex *synthMutex;

	QDir romDir;
	QString controlROMFileName;
	QString pcmROMFileName;
	const MT32Emu::ROMImage *controlROMImage;
	const MT32Emu::ROMImage *pcmROMImage;
	int reverbMode;
	int reverbTime;
	int reverbLevel;
	int partialCount;
	MT32Emu::AnalogOutputMode analogOutputMode;
	ReverbCompatibilityMode reverbCompatibilityMode;
	bool engageChannel1OnOpen;

	MT32Emu::Synth *synth;
	QReportHandler reportHandler;
	QString synthProfileName;

	MT32Emu::SampleRateConverter *sampleRateConverter;
	AudioFileWriter *audioRecorder;

	void setState(SynthState newState);
	void freeROMImages();
	MT32Emu::Bit32u convertOutputToSynthTimestamp(quint64 timestamp);

public:
	QSynth(QObject *parent = NULL);
	~QSynth();
	bool isOpen() const;
	bool open(uint &targetSampleRate, MT32Emu::SamplerateConversionQuality srcQuality = MT32Emu::SamplerateConversionQuality_GOOD, const QString useSynthProfileName = "");
	void close();
	bool reset();

	void flushMIDIQueue();
	void playMIDIShortMessageNow(MT32Emu::Bit32u msg);
	void playMIDISysexNow(const MT32Emu::Bit8u *sysex, MT32Emu::Bit32u sysexLen);
	bool playMIDIShortMessage(MT32Emu::Bit32u msg, quint64 timestamp);
	bool playMIDISysex(const MT32Emu::Bit8u *sysex, MT32Emu::Bit32u sysexLen, quint64 timestamp);
	void render(MT32Emu::Bit16s *buffer, uint length);

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
	void setReversedStereoEnabled(bool enabled);
	void setNiceAmpRampEnabled(bool enabled);
	void resetMIDIChannelsAssignment(bool engageChannel1);
	void setInitialMIDIChannelsAssignment(bool engageChannel1);
	void setReverbCompatibilityMode(ReverbCompatibilityMode reverbCompatibilityMode);
	void setMIDIDelayMode(MT32Emu::MIDIDelayMode midiDelayMode);
	void setDACInputMode(MT32Emu::DACInputMode emuDACInputMode);
	void setAnalogOutputMode(MT32Emu::AnalogOutputMode analogOutputMode);
	void setRendererType(MT32Emu::RendererType useRendererType);
	void setPartialCount(int partialCount);
	const QString getPatchName(int partNum) const;
	void getPartStates(bool *partStates) const;
	void getPartialStates(MT32Emu::PartialState *partialStates) const;
	unsigned int getPlayingNotes(unsigned int partNumber, MT32Emu::Bit8u *keys, MT32Emu::Bit8u *velocities) const;
	unsigned int getPartialCount() const;
	unsigned int getSynthSampleRate() const;
	bool isActive() const;

	void startRecordingAudio(const QString &fileName);
	void stopRecordingAudio();
	bool isRecordingAudio() const;

signals:
	void stateChanged(SynthState state);
	void audioBlockRendered();
};

#endif
