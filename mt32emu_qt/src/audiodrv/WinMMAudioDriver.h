#ifndef WIN_MM_AUDIO_DRIVER_H
#define WIN_MM_AUDIO_DRIVER_H

#include <QtCore>
#include <windows.h>
#include <mt32emu/mt32emu.h>
#include "AudioDriver.h"
#include "../MasterClock.h"

#ifndef _UINTPTR_T_DEFINED
// For MinGW
#include <stdint.h>
#endif

class Master;
class WinMMAudioDriver;
class WinMMAudioDevice;
class WinMMAudioStream;

class WinMMAudioProcessor : public QThread {
public:
	WinMMAudioProcessor(WinMMAudioStream &stream);

protected:
	void run();

private:
	WinMMAudioStream &stream;
};

class WinMMAudioStream : public AudioStream {
	friend class WinMMAudioProcessor;
private:
	HWAVEOUT hWaveOut;
	WAVEHDR	 *waveHdr;
	HANDLE hEvent;
	HANDLE hWaitableTimer;

	uint numberOfChunks;
	uint chunkSize;
	MT32Emu::Bit16s *buffer;
	bool volatile stopProcessing;
	WinMMAudioProcessor processor;
	bool ringBufferMode;
	quint64 prevPlayPosition;

	DWORD getCurrentPlayPosition();

public:
	WinMMAudioStream(const AudioDriverSettings &useSettings, bool ringBufferMode, QSynth &useSynth, uint useSampleRate);
	~WinMMAudioStream();
	bool start(int deviceIndex);
	void close();
};

class WinMMAudioDevice : public AudioDevice {
friend class WinMMAudioDriver;
private:
	UINT deviceIndex;
	WinMMAudioDevice(WinMMAudioDriver &driver, int useDeviceIndex, QString useDeviceName);
public:
	AudioStream *startAudioStream(QSynth &synth, const uint sampleRate) const;
};

class WinMMAudioDriver : public AudioDriver {
private:
	AudioDriverSettings streamSettings;
	bool ringBufferMode;

	void validateAudioSettings(AudioDriverSettings &useSettings) const;
	void loadAudioSettings();
public:
	WinMMAudioDriver(Master *useMaster);
	const QList<const AudioDevice *> createDeviceList();
	void setAudioSettings(AudioDriverSettings &useSettings);
	const AudioDriverSettings &getAudioStreamSettings();
	bool isRingBufferMode();
};

#endif
