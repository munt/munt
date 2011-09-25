#ifndef WIN_MM_AUDIO_DRIVER_H
#define WIN_MM_AUDIO_DRIVER_H

#include <QtCore>
#include <windows.h>
#include <mt32emu/mt32emu.h>
#include "AudioDriver.h"
#include "../MasterClock.h"

class Master;
class QSynth;

class WinMMAudioStream : public AudioStream {
private:
	QSynth *synth;
	unsigned int sampleRate;
	HWAVEOUT hWaveOut;
	WAVEHDR	 WaveHdr;
	MT32Emu::Bit16s *buffer;
	bool pendingClose;

	static void processingThread(void *);

public:
	WinMMAudioStream(QSynth *useSynth, unsigned int useSampleRate);
	~WinMMAudioStream();
	bool start(int deviceIndex);
	void close();
};

class WinMMAudioDevice : public AudioDevice {
friend class WinMMAudioDriver;
private:
	UINT deviceIndex;
	WinMMAudioDevice(WinMMAudioDriver *driver, int useDeviceIndex, QString useDeviceName);
public:
	WinMMAudioStream *startAudioStream(QSynth *synth, unsigned int sampleRate) const;
};

class WinMMAudioDriver : public AudioDriver {
public:
	WinMMAudioDriver(Master *useMaster);
	~WinMMAudioDriver();
	QList<AudioDevice *> getDeviceList();
};

#endif
