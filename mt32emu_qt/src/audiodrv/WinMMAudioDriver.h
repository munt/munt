#ifndef WIN_MM_AUDIO_DRIVER_H
#define WIN_MM_AUDIO_DRIVER_H

#include <QtCore>
#include <windows.h>
#include <mt32emu/mt32emu.h>
#include "AudioDriver.h"
#include "../MasterClock.h"

class Master;
class QSynth;
class WinMMAudioDriver;
class WinMMAudioDevice;

class WinMMAudioStream : public AudioStream {
private:
	unsigned int chunkSize;
	unsigned int bufferSize;
	MasterClockNanos midiLatency; // the delay for MIDI events to damp timing deviations and ensure accurate rendering
	QSynth *synth;
	const unsigned int sampleRate;
	HWAVEOUT hWaveOut;
	WAVEHDR	 *waveHdr;
	HANDLE hEvent;
	unsigned int numberOfChunks;
	MT32Emu::Bit16s *buffer;
	bool volatile stopProcessing;
	uintptr_t processingThreadHandle;
	bool useRingBuffer;
	quint64 volatile prevPlayPosition;

	static void processingThread(void *);

	DWORD getCurrentPlayPosition();

public:
	WinMMAudioStream(const WinMMAudioDevice *device, QSynth *useSynth, unsigned int useSampleRate);
	~WinMMAudioStream();
	bool start(int deviceIndex);
	void close();
};

class WinMMAudioDevice : public AudioDevice {
friend class WinMMAudioDriver;
private:
	UINT deviceIndex;
	WinMMAudioDevice(WinMMAudioDriver * const driver, int useDeviceIndex, QString useDeviceName);
public:
	WinMMAudioStream *startAudioStream(QSynth *synth, unsigned int sampleRate) const;
};

class WinMMAudioDriver : public AudioDriver {
private:
	void validateAudioSettings(AudioDriverSettings &useSettings) const;
	void loadAudioSettings();
public:
	WinMMAudioDriver(Master *useMaster);
	~WinMMAudioDriver();
	const QList<const AudioDevice *> createDeviceList();
	void setAudioSettings(AudioDriverSettings &useSettings);
};

#endif
