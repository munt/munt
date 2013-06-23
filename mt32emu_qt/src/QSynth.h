#ifndef QSYNTH_H
#define QSYNTH_H

#include <ctime>
#include <QtCore>
#include <mt32emu/mt32emu.h>

#include "MidiEventQueue.h"

enum SynthState {
	SynthState_CLOSED,
	SynthState_OPEN,
	SynthState_CLOSING
};

enum PartialState {
	PartialState_DEAD,
	PartialState_ATTACK,
	PartialState_SUSTAINED,
	PartialState_RELEASED
};

struct SynthProfile {
	QDir romDir;
	QString controlROMFileName;
	QString pcmROMFileName;
	const MT32Emu::ROMImage *controlROMImage;
	const MT32Emu::ROMImage *pcmROMImage;
	MT32Emu::DACInputMode emuDACInputMode;
	float outputGain;
	float reverbOutputGain;
	bool reverbEnabled;
	bool reverbOverridden;
	int reverbMode;
	int reverbTime;
	int reverbLevel;
};

class QReportHandler : public QObject, public MT32Emu::ReportHandler {
	Q_OBJECT

public:
	QReportHandler(QObject *parent = NULL);
	void showLCDMessage(const char *message);
	void onErrorControlROM();
	void onErrorPCMROM();
	void onDeviceReconfig();
	void onDeviceReset();
	void onNewReverbMode(MT32Emu::Bit8u mode);
	void onNewReverbTime(MT32Emu::Bit8u time);
	void onNewReverbLevel(MT32Emu::Bit8u level);
	void onPartStateChanged(int partNum, bool isActive);
	void onPolyStateChanged(int partNum);
	void onPartialStateChanged(int partialNum, int oldPartialPhase, int newPartialPhase);
	void onProgramChanged(int partNum, const char patchName[]);

signals:
	void balloonMessageAppeared(const QString &title, const QString &text);
	void lcdMessageDisplayed(const QString);
	void masterVolumeChanged(int);
	void reverbModeChanged(int);
	void reverbTimeChanged(int);
	void reverbLevelChanged(int);
	void partStateChanged(int, bool);
	void polyStateChanged(int);
	void partialStateChanged(int, int);
	void programChanged(int, QString);
};

class QSynth : public QObject {
	Q_OBJECT

friend class QReportHandler;

private:
	SynthState state;

	QMutex *synthMutex;
	MidiEventQueue *midiEventQueue;

	volatile bool isOpen;
	QDir romDir;
	QString controlROMFileName;
	QString pcmROMFileName;
	const MT32Emu::ROMImage *controlROMImage;
	const MT32Emu::ROMImage *pcmROMImage;
	MT32Emu::DACInputMode emuDACInputMode;
	float outputGain;
	float reverbOutputGain;
	int reverbMode;
	int reverbTime;
	int reverbLevel;

	// For debugging
	quint64 debugSampleIx;
	quint64 debugLastEventSampleIx;

	MT32Emu::Synth *synth;
	QReportHandler *reportHandler;
	QString synthProfileName;

	void setState(SynthState newState);
	void freeROMImages();

public:
	QSynth(QObject *parent = NULL);
	~QSynth();
	bool open(const QString useSynthProfileName = "");
	void close();
	bool reset();
	bool pushMIDIShortMessage(MT32Emu::Bit32u msg, SynthTimestamp timestamp);
	bool pushMIDISysex(MT32Emu::Bit8u *sysex, unsigned int sysexLen, SynthTimestamp timestamp);
	unsigned int render(MT32Emu::Bit16s *buf, unsigned int len, SynthTimestamp firstSampleTimestamp, double actualSampleRate);

	void getSynthProfile(SynthProfile &synthProfile) const;
	void setSynthProfile(const SynthProfile &synthProfile, QString useSynthProfileName);

	void setMasterVolume(int masterVolume);
	void setOutputGain(float outputGain);
	void setReverbOutputGain(float reverbOutputGain);
	void setReverbEnabled(bool reverbEnabled);
	void setReverbOverridden(bool reverbOverridden);
	void setReverbSettings(int reverbMode, int reverbTime, int reverbLevel);
	void setDACInputMode(MT32Emu::DACInputMode emuDACInputMode);
	QString getPatchName(int partNum);
	const MT32Emu::Partial *getPartial(int partialNum) const;
	bool isActive() const;

signals:
	void stateChanged(SynthState state);
	void partStateReset();
};

#endif
