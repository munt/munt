/* Copyright (C) 2011, 2012 Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.h"

namespace MT32Emu {

#define	DRIVER_MODE

static MidiSynth &midiSynth = MidiSynth::getInstance();

static class MidiStream {
private:
	static const unsigned int maxPos = 1024;
	unsigned int startpos;
	unsigned int endpos;
	DWORD stream[maxPos][2];

public:
	MidiStream() {
		startpos = 0;
		endpos = 0;
	}

	DWORD PutMessage(DWORD msg, DWORD timestamp) {
		unsigned int newEndpos = endpos;

		newEndpos++;
		if (newEndpos == maxPos) // check for buffer rolloff
			newEndpos = 0;
		if (startpos == newEndpos) // check for buffer full
			return -1;
		stream[endpos][0] = msg;	// ok to put data and update endpos
		stream[endpos][1] = timestamp;
		endpos = newEndpos;
		return 0;
	}

	DWORD GetMessage() {
		if (startpos == endpos) // check for buffer empty
			return -1;
		DWORD msg = stream[startpos][0];
		startpos++;
		if (startpos == maxPos) // check for buffer rolloff
			startpos = 0;
		return msg;
	}

	DWORD PeekMessageTime() {
		if (startpos == endpos) // check for buffer empty
			return (DWORD)-1;
		return stream[startpos][1];
	}

	DWORD PeekMessageTimeAt(unsigned int pos) {
		if (startpos == endpos) // check for buffer empty
			return -1;
		unsigned int peekPos = (startpos + pos) % maxPos;
		return stream[peekPos][1];
	}
} midiStream;

static class SynthEventWin32 {
private:
	HANDLE hEvent;

public:
	int Init() {
		hEvent = CreateEvent(NULL, false, true, NULL);
		if (hEvent == NULL) {
			MessageBox(NULL, L"Can't create sync object", L"MT32", MB_OK | MB_ICONEXCLAMATION);
			return 1;
		}
		return 0;
	}

	void Close() {
		CloseHandle(hEvent);
	}

	void Wait() {
		WaitForSingleObject(hEvent, INFINITE);
	}

	void Release() {
		SetEvent(hEvent);
	}
} synthEvent;

static class WaveOutWin32 {
private:
	HWAVEOUT	hWaveOut;
	WAVEHDR		*WaveHdr;
	HANDLE		hEvent;
	DWORD		chunks;
	DWORD		prevPlayPos;
	DWORD		getPosWraps;
	bool		stopProcessing;

public:
	int Init(Bit16s *buffer, unsigned int bufferSize, unsigned int chunkSize, bool useRingBuffer, unsigned int sampleRate) {
		DWORD callbackType = CALLBACK_NULL;
		DWORD_PTR callback = NULL;
		hEvent = NULL;
		if (!useRingBuffer) {
			hEvent = CreateEvent(NULL, false, true, NULL);
			callback = (DWORD_PTR)hEvent;
			callbackType = CALLBACK_EVENT;
		}

		PCMWAVEFORMAT wFormat = {WAVE_FORMAT_PCM, 2, sampleRate, sampleRate * 4, 4, 16};

		// Open waveout device
		int wResult = waveOutOpen(&hWaveOut, WAVE_MAPPER, (LPWAVEFORMATEX)&wFormat, callback, (DWORD_PTR)&midiSynth, callbackType);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to open waveform output device", L"MT32", MB_OK | MB_ICONEXCLAMATION);
			return 2;
		}

		// Prepare headers
		chunks = useRingBuffer ? 1 : bufferSize / chunkSize;
		WaveHdr = new WAVEHDR[chunks];
		LPSTR chunkStart = (LPSTR)buffer;
		DWORD chunkBytes = 4 * chunkSize;
		for (UINT i = 0; i < chunks; i++) {
			if (useRingBuffer) {
				WaveHdr[i].dwBufferLength = 4 * bufferSize;
				WaveHdr[i].lpData = chunkStart;
				WaveHdr[i].dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
				WaveHdr[i].dwLoops = -1L;
			} else {
				WaveHdr[i].dwBufferLength = chunkBytes;
				WaveHdr[i].lpData = chunkStart;
				WaveHdr[i].dwFlags = 0L;
				WaveHdr[i].dwLoops = 0L;
				chunkStart += chunkBytes;
			}
			wResult = waveOutPrepareHeader(hWaveOut, &WaveHdr[i], sizeof(WAVEHDR));
			if (wResult != MMSYSERR_NOERROR) {
				MessageBox(NULL, L"Failed to Prepare Header", L"MT32", MB_OK | MB_ICONEXCLAMATION);
				return 3;
			}
		}
		stopProcessing = false;
		return 0;
	}

	int Close() {
		stopProcessing = true;
		SetEvent(hEvent);
		int wResult = waveOutReset(hWaveOut);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Reset WaveOut", L"MT32", MB_OK | MB_ICONEXCLAMATION);
			return 8;
		}

		for (UINT i = 0; i < chunks; i++) {
			wResult = waveOutUnprepareHeader(hWaveOut, &WaveHdr[i], sizeof(WAVEHDR));
			if (wResult != MMSYSERR_NOERROR) {
				MessageBox(NULL, L"Failed to Unprepare Wave Header", L"MT32", MB_OK | MB_ICONEXCLAMATION);
				return 8;
			}
		}
		delete[] WaveHdr;
		WaveHdr = NULL;

		wResult = waveOutClose(hWaveOut);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Close WaveOut", L"MT32", MB_OK | MB_ICONEXCLAMATION);
			return 8;
		}
		if (hEvent != NULL) {
			CloseHandle(hEvent);
			hEvent = NULL;
		}
		return 0;
	}

	int Start() {
		getPosWraps = 0;
		prevPlayPos = 0;
		for (UINT i = 0; i < chunks; i++) {
			if (waveOutWrite(hWaveOut, &WaveHdr[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
				MessageBox(NULL, L"Failed to write block to device", L"MT32", MB_OK | MB_ICONEXCLAMATION);
				return 4;
			}
		}
		_beginthread(RenderingThread, 16384, this);
		return 0;
	}

	int Pause() {
		if (waveOutPause(hWaveOut) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Pause wave playback", L"MT32", MB_OK | MB_ICONEXCLAMATION);
			return 9;
		}
		return 0;
	}

	int Resume() {
		if (waveOutRestart(hWaveOut) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Resume wave playback", L"MT32", MB_OK | MB_ICONEXCLAMATION);
			return 9;
		}
		return 0;
	}

	UINT64 GetPos() {
		MMTIME mmTime;
		mmTime.wType = TIME_SAMPLES;

		if (waveOutGetPosition(hWaveOut, &mmTime, sizeof MMTIME) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to get current playback position", L"MT32", MB_OK | MB_ICONEXCLAMATION);
			return 10;
		}
		if (mmTime.wType != TIME_SAMPLES) {
			MessageBox(NULL, L"Failed to get # of samples played", L"MT32", MB_OK | MB_ICONEXCLAMATION);
			return 10;
		}

		// Deal with waveOutGetPosition() wraparound. For 16-bit stereo output, it equals 2^27,
		// presumably caused by the internal 32-bit counter of bits played.
		// The output of that nasty waveOutGetPosition() isn't monotonically increasing
		// even during 2^27 samples playback, so we have to ensure the difference is big enough...
		int delta = mmTime.u.sample - prevPlayPos;
		if (delta < -(1 << 26)) {
			std::cout << "MT32: GetPos() wrap: " << delta << "\n";
			++getPosWraps;
		}
		prevPlayPos = mmTime.u.sample;
		return mmTime.u.sample + getPosWraps * (1 << 27);
	}

	static void RenderingThread(void *) {
		if (waveOut.chunks == 1) {
			// Rendering using single looped ring buffer
			while (!waveOut.stopProcessing) {
				midiSynth.RenderAvailableSpace();
			}
		} else {
			while (!waveOut.stopProcessing) {
				bool allBuffersRendered = true;
				for (UINT i = 0; i < waveOut.chunks; i++) {
					if (waveOut.WaveHdr[i].dwFlags & WHDR_DONE) {
						allBuffersRendered = false;
						midiSynth.Render((Bit16s *)waveOut.WaveHdr[i].lpData, waveOut.WaveHdr[i].dwBufferLength / 4);
						if (waveOutWrite(waveOut.hWaveOut, &waveOut.WaveHdr[i], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
							MessageBox(NULL, L"Failed to write block to device", L"MT32", MB_OK | MB_ICONEXCLAMATION);
						}
					}
				}
				if (allBuffersRendered) {
					WaitForSingleObject(waveOut.hEvent, INFINITE);
				}
			}
		}
	}
} waveOut;

#ifdef DRIVER_MODE
void printDebug(void *userData, const char *fmt, va_list list) {
}
#else
#define printDebug NULL
#endif

MidiSynth::MidiSynth() {}

int MidiSynth::MT32_Report(void *userData, ReportType type, const void *reportData) {
#if MT32EMU_USE_EXTINT == 1
	midiSynth.mt32emuExtInt->handleReport(midiSynth.synth, type, reportData);
#endif
	switch(type) {
	case MT32Emu::ReportType_errorControlROM:
		MessageBox(NULL, L"Couldn't open Control ROM file", L"MT32", MB_OK | MB_ICONEXCLAMATION);
		break;
	case MT32Emu::ReportType_errorPCMROM:
		MessageBox(NULL, L"Couldn't open PCM ROM file", L"MT32", MB_OK | MB_ICONEXCLAMATION);
		break;
	case MT32Emu::ReportType_lcdMessage:
		std::cout << "MT32: LCD-Message: " << (char *)reportData << "\n";
		break;
	default:
		//LOG(LOG_ALL,LOG_NORMAL)("MT32: Report %d",type);
		break;
	}
	return 0;
}

MidiSynth &MidiSynth::getInstance() {
	static MidiSynth *instance = new MidiSynth;
	return *instance;
}

// Renders all the available space in the single looped ring buffer
void MidiSynth::RenderAvailableSpace() {
	DWORD playPos = waveOut.GetPos() % bufferSize;
	DWORD framesToRender;

	if (playPos < framesRendered) {
		// Buffer wrap, render 'till the end of the buffer
		framesToRender = bufferSize - framesRendered;
	} else {
		framesToRender = playPos - framesRendered;
		if (framesToRender < chunkSize) {
			Sleep(1 + (chunkSize - framesToRender) * 1000 / sampleRate);
			return;
		}
	}
	midiSynth.Render(buffer + 2 * framesRendered, framesToRender);
}

// Renders totalFrames frames starting from bufpos
// The number of frames rendered is added to the global counter framesRendered
void MidiSynth::Render(Bit16s *bufpos, DWORD totalFrames) {
	while (totalFrames > 0) {
		DWORD timeStamp;
		// Incoming MIDI messages timestamped with the current audio playback position + midiLatency
		while ((timeStamp = midiStream.PeekMessageTime()) == framesRendered) {
			DWORD msg = midiStream.GetMessage();
			synthEvent.Wait();
			synth->playMsg(msg);
			synthEvent.Release();
		}

		// Find out how many frames to render. The value of timeStamp == -1 indicates the MIDI buffer is empty
		DWORD framesToRender = timeStamp - framesRendered;
		if (framesToRender > totalFrames) {
			// MIDI message is too far - render the rest of frames
			framesToRender = totalFrames;
		}
		synthEvent.Wait();
		synth->render(bufpos, framesToRender);
		synthEvent.Release();
		framesRendered += framesToRender;
		bufpos += 2 * framesToRender; // each frame consists of two samples for both the Left and Right channels
		totalFrames -= framesToRender;
	}

	// Wrap framesRendered counter
	if (framesRendered >= bufferSize) {
		framesRendered -= bufferSize;
	}
#if MT32EMU_USE_EXTINT == 1
	if (mt32emuExtInt != NULL) {
		mt32emuExtInt->doControlPanelComm(synth, 0);
	}
#endif
}

static int LoadIntValue(char *key, int nDefault) {
	return GetPrivateProfileIntA("mt32emu", key, nDefault, "mt32emu.ini");
}

static DWORD LoadStringValue(char *key, char *nDefault, char *destination, DWORD nSize) {
	return GetPrivateProfileStringA("mt32emu", key, nDefault, destination, nSize, "mt32emu.ini");
}

unsigned int MidiSynth::MillisToFrames(unsigned int millis) {
	return UINT(sampleRate * millis / 1000.f);
}

void MidiSynth::LoadSettings() {
	sampleRate = LoadIntValue("SampleRate", 32000);
	// Approx. bufferSize derived from latency
	bufferSize = MillisToFrames(LoadIntValue("Latency", 100));
	chunkSize = MillisToFrames(LoadIntValue("ChunkMs", 10));
	midiLatency = MillisToFrames(LoadIntValue("MIDILatency", 0));
	useRingBuffer = LoadIntValue("UseRingBuffer", 0) != 0;
	if (!useRingBuffer) {
		// Number of chunks should be ceil(bufferSize / chunkSize)
		DWORD chunks = (bufferSize + chunkSize - 1) / chunkSize;
		// Refine bufferSize as chunkSize * number of chunks, no less then the specified value
		bufferSize = chunks * chunkSize;
		std::cout << "MT32: Using " << chunks << " chunks, chunk size: " << chunkSize << " frames, buffer size: " << bufferSize << " frames." << std::endl;
	}
	ReloadSettings();
}

void MidiSynth::ReloadSettings() {
	resetEnabled = true;
	if (LoadIntValue("ResetEnabled", 1) == 0) {
		resetEnabled = false;
	}

	reverbEnabled = true;
	if (LoadIntValue("ReverbEnabled", 1) == 0) {
		reverbEnabled = false;
	}

	reverbOverridden = true;
	if (LoadIntValue("ReverbOverridden", 0) == 0) {
		reverbOverridden = false;
	}

	reverbMode = LoadIntValue("ReverbMode", 0);
	reverbTime = LoadIntValue("ReverbTime", 5);
	reverbLevel = LoadIntValue("ReverbLevel", 3);

	outputGain = (float)LoadIntValue("OutputGain", 100);
	if (outputGain < 0.0f) {
		outputGain = -outputGain;
	}
	if (outputGain > 1000.0f) {
		outputGain = 1000.0f;
	}

	reverbOutputGain = (float)LoadIntValue("ReverbOutputGain", 100);
	if (reverbOutputGain < 0.0f) {
		reverbOutputGain = -reverbOutputGain;
	}
	if (reverbOutputGain > 1000.0f) {
		reverbOutputGain = 1000.0f;
	}

	emuDACInputMode = (DACInputMode)LoadIntValue("DACInputMode", DACInputMode_GENERATION2);

	DWORD s = LoadStringValue("PathToROMFiles", "C:/WINDOWS/SYSTEM32/", pathToROMfiles, 254);
	pathToROMfiles[s] = '/';
	pathToROMfiles[s + 1] = 0;
}

void MidiSynth::ApplySettings() {
	synth->setReverbEnabled(reverbEnabled);
	synth->setDACInputMode(emuDACInputMode);
	synth->setOutputGain(outputGain / 100.0f);
	synth->setReverbOutputGain(reverbOutputGain / 147.0f);
	if (reverbOverridden) {
		Bit8u sysex[] = {0x10, 0x00, 0x01, reverbMode, reverbTime, reverbLevel};
		synth->setReverbOverridden(false);
		synth->writeSysex(16, sysex, 6);
		synth->setReverbOverridden(true);
	}
}

int MidiSynth::Init() {
	LoadSettings();

	buffer = new Bit16s[2 * bufferSize]; // each frame consists of two samples for both the Left and Right channels

	// Init synth
	if (synthEvent.Init()) {
		return 1;
	}
	synth = new Synth();
	SynthProperties synthProp = {sampleRate, true, true, 0, 0, 0, pathToROMfiles,
		NULL, MT32_Report, printDebug, NULL, NULL};
	if (!synth->open(synthProp)) {
		MessageBox(NULL, L"Can't open Synth", L"MT32", MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	ApplySettings();

#if MT32EMU_USE_EXTINT == 1
	// Init External Interface
	mt32emuExtInt = new MT32Emu::ExternalInterface();
	mt32emuExtInt->start();
#endif

	UINT wResult = waveOut.Init(buffer, bufferSize, chunkSize, useRingBuffer, sampleRate);
	if (wResult) return wResult;

	// Start playing stream
	synth->render(buffer, bufferSize);
	framesRendered = 0;

	wResult = waveOut.Start();
	return wResult;
}

int MidiSynth::Reset() {
#ifdef DRIVER_MODE
	ReloadSettings();
	if (!resetEnabled) {
		ApplySettings();
		return 0;
	}
#endif

	UINT wResult = waveOut.Pause();
	if (wResult) return wResult;

	synthEvent.Wait();
	synth->close();
	delete synth;
	synth = new Synth();
	SynthProperties synthProp = {sampleRate, true, true, 0, 0, 0, pathToROMfiles,
		NULL, MT32_Report, printDebug, NULL, NULL};
	if (!synth->open(synthProp)) {
		MessageBox(NULL, L"Can't open Synth", L"MT32", MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}
	ApplySettings();
	synthEvent.Release();

	wResult = waveOut.Resume();
	return wResult;
}

void MidiSynth::PushMIDI(DWORD msg) {
	midiStream.PutMessage(msg, (waveOut.GetPos() + midiLatency) % bufferSize);
}

void MidiSynth::PlaySysex(Bit8u *bufpos, DWORD len) {
	synthEvent.Wait();
	synth->playSysex(bufpos, len);
	synthEvent.Release();
}

void MidiSynth::Close() {
#if MT32EMU_USE_EXTINT == 1
	// Close External Interface
	if(mt32emuExtInt != NULL) {
		mt32emuExtInt->stop();
		delete mt32emuExtInt;
		mt32emuExtInt = NULL;
	}
#endif

	waveOut.Pause();
	waveOut.Close();
	synthEvent.Wait();
	synth->close();

	// Cleanup memory
	delete synth;
	delete buffer;

	synthEvent.Close();
}

}
