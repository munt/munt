#ifndef WIN_MM_AUDIO_DRIVER_H
#define WIN_MM_AUDIO_DRIVER_H

#include <QtCore>
#include <windows.h>
#include <mt32emu/mt32emu.h>
#include "AudioDriver.h"
#include "../MasterClock.h"

class QSynth;

class WinMMAudioDriver : public AudioDriver {
private:
	QSynth *synth;
	unsigned int sampleRate;
	HWAVEOUT hWaveOut;
	WAVEHDR	 WaveHdr;
	MT32Emu::Bit16s *buffer;
	bool pendingClose;

	qint64 getPlayedAudioNanosPlusLatency();
	static void processingThread(void *);

public:
	WinMMAudioDriver(QSynth *useSynth, unsigned int useSampleRate);
	~WinMMAudioDriver();
	bool start(int deviceIndex);
	void close();
	QList<QString> getDeviceNames();
};

#endif
