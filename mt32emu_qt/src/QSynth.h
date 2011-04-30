#ifndef QSYNTH_H
#define QSYNTH_H

#include <ctime>
#include <QtCore>
#include <QtMultimediaKit/QAudioOutput>
#include <mt32emu/mt32emu.h>

#include "MidiEventQueue.h"

class WaveGenerator;

class QSynth {
friend class WaveGenerator;
private:
	QAudioOutput *audioOutput;
	WaveGenerator *waveGenerator;

	unsigned int sampleRate;
	bool reverbEnabled;
	MT32Emu::DACInputMode emuDACInputMode;
	QDir romDir;

	QMutex *synthMutex;
	MidiEventQueue *midiEventQueue;

	volatile unsigned int latency;

	volatile bool isOpen;
	volatile clock_t bufferStartClock;
	volatile quint64 bufferStartSampleIx;
	volatile quint64 sampleIx;

	MT32Emu::Synth *synth;

	bool openSynth();
	SynthTimestamp getTimestamp();
	unsigned int render(MT32Emu::Bit16s *buf, unsigned int len);

public:
	QSynth();
	~QSynth();
	int open();
	void close();
	int reset();
	bool pushMIDIShortMessage(MT32Emu::Bit32u msg);
	bool pushMIDISysex(MT32Emu::Bit8u *sysex, unsigned int sysexLen);

	void setMasterVolume(MT32Emu::Bit8u pMasterVolume);
	void setReverbEnabled(bool pReverbEnabled);
	void setDACInputMode(MT32Emu::DACInputMode pEmuDACInputMode);
	void setLatency(unsigned int newLatency);
	int setROMDir(QDir romDir);
	QDir getROMDir();
	void handleReport(MT32Emu::ReportType type, const void *reportData);
};

#endif
