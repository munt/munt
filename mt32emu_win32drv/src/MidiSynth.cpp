/* Copyright (C) 2011 Sergey V. Mikayev
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

MidiSynth *midiSynth;

class MidiStream {
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
			return -1;
		return stream[startpos][1];
	}

	DWORD PeekMessageTimeAt(unsigned int pos) {
		if (startpos == endpos) // check for buffer empty
			return -1;
		unsigned int peekPos = (startpos + pos) % maxPos;
		return stream[peekPos][1];
	}
} midiStream;

class SynthEventWin32 {
private:
	HANDLE hEvent;

public:
	int Init() {
		hEvent = CreateEvent(NULL, false, true, NULL);
		if (hEvent == NULL) {
			MessageBox(NULL, L"Can't create sync object", NULL, MB_OK | MB_ICONEXCLAMATION);
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

class WaveOutWin32 {
private:
	HWAVEOUT	hWaveOut;
	WAVEHDR		WaveHdr1;
	WAVEHDR		WaveHdr2;
	WAVEHDR		WaveHdr3;
	WAVEHDR		WaveHdr4;

	HANDLE		hEvent;

public:
static void waveOutProc(void *) {
	for (;;) {
		WaitForSingleObject(waveOut.hEvent, INFINITE);

		if (midiSynth->IsPendingClose()) break;

		if (waveOut.WaveHdr1.dwFlags & WHDR_DONE) {
			midiSynth->Render((Bit16s *)waveOut.WaveHdr1.lpData);
			if (waveOutWrite(waveOut.hWaveOut, &waveOut.WaveHdr1, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
				MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
			}
		}

		if (waveOut.WaveHdr2.dwFlags & WHDR_DONE) {
			midiSynth->Render((Bit16s *)waveOut.WaveHdr2.lpData);
			if (waveOutWrite(waveOut.hWaveOut, &waveOut.WaveHdr2, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
				MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
			}
		}

		if (waveOut.WaveHdr3.dwFlags & WHDR_DONE) {
			midiSynth->Render((Bit16s *)waveOut.WaveHdr3.lpData);
			if (waveOutWrite(waveOut.hWaveOut, &waveOut.WaveHdr3, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
				MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
			}
		}

		if (waveOut.WaveHdr4.dwFlags & WHDR_DONE) {
			midiSynth->Render((Bit16s *)waveOut.WaveHdr4.lpData);
			if (waveOutWrite(waveOut.hWaveOut, &waveOut.WaveHdr4, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
				MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
			}
		}
	}
}

	int Init(Bit16s *stream1, Bit16s *stream2, Bit16s *stream3, Bit16s *stream4, unsigned int len, unsigned int sampleRate) {
		int wResult;
		PCMWAVEFORMAT wFormat = {WAVE_FORMAT_PCM, 2, sampleRate, sampleRate * 4, 4, 16};

		hEvent = CreateEvent(NULL, false, true, NULL);
		if (hEvent == NULL) {
			MessageBox(NULL, L"Can't create waveOut sync object", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 1;
		}

		//	Open waveout device
		wResult = waveOutOpen(&hWaveOut, WAVE_MAPPER, (LPWAVEFORMATEX)&wFormat, (DWORD_PTR)hEvent, (DWORD_PTR)&midiSynth, CALLBACK_EVENT);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to open waveform output device.", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 2;
		}

		//	Prepare 4 Headers
		WaveHdr1.lpData = (LPSTR)stream1;
		WaveHdr1.dwBufferLength = 4 * len;
		WaveHdr1.dwFlags = 0L;
		WaveHdr1.dwLoops = 0L;
		wResult = waveOutPrepareHeader(hWaveOut, &WaveHdr1, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Prepare Header 1", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 3;
		}

		WaveHdr2.lpData = (LPSTR)stream2;
		WaveHdr2.dwBufferLength = 4 * len;
		WaveHdr2.dwFlags = 0L;
		WaveHdr2.dwLoops = 0L;
		wResult = waveOutPrepareHeader(hWaveOut, &WaveHdr2, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Prepare Header 2", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 3;
		}

		WaveHdr3.lpData = (LPSTR)stream3;
		WaveHdr3.dwBufferLength = 4 * len;
		WaveHdr3.dwFlags = 0L;
		WaveHdr3.dwLoops = 0L;
		wResult = waveOutPrepareHeader(hWaveOut, &WaveHdr3, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Prepare Header 3", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 3;
		}

		WaveHdr4.lpData = (LPSTR)stream4;
		WaveHdr4.dwBufferLength = 4 * len;
		WaveHdr4.dwFlags = 0L;
		WaveHdr4.dwLoops = 0L;
		wResult = waveOutPrepareHeader(hWaveOut, &WaveHdr4, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Prepare Header 4", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 3;
		}
		return 0;
	}

	int Close() {
		int wResult;

		SetEvent(waveOut.hEvent);

		wResult = waveOutReset(hWaveOut);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Reset WaveOut", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 8;
		}

		wResult = waveOutUnprepareHeader(hWaveOut, &WaveHdr1, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Unprepare Wave Header", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 8;
		}

		wResult = waveOutUnprepareHeader(hWaveOut, &WaveHdr2, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Unprepare Wave Header", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 8;
		}

		wResult = waveOutClose(hWaveOut);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Close WaveOut", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 8;
		}

		CloseHandle(hEvent);
		return 0;
	}

	int Start() {
		if (waveOutWrite(hWaveOut, &WaveHdr1, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 4;
		}
		if (waveOutWrite(hWaveOut, &WaveHdr2, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 4;
		}
		if (waveOutWrite(hWaveOut, &WaveHdr3, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 4;
		}
		if (waveOutWrite(hWaveOut, &WaveHdr4, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 4;
		}
		return 0;
	}

	int Pause() {
		if (waveOutPause(hWaveOut) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Pause wave playback", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 9;
		}
		return 0;
	}

	int Resume() {
		if (waveOutRestart(hWaveOut) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Pause wave playback", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 9;
		}
		return 0;
	}

	int GetPos() {
		MMTIME mmTime;
		mmTime.wType = TIME_SAMPLES;

		if (waveOutGetPosition(hWaveOut, &mmTime, sizeof MMTIME) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to get current playback position", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 10;
		}
		return mmTime.u.sample;
	}
} waveOut;

int MT32_Report(void *userData, ReportType type, const void *reportData) {
#if MT32EMU_USE_EXTINT == 1
	midiSynth->handleReport(type, reportData);
#endif
	return 0;
}

void MidiSynth::handleReport(ReportType type, const void *reportData) {
#if MT32EMU_USE_EXTINT == 1
	mt32emuExtInt->handleReport(synth, type, reportData);
#endif
}

void MidiSynth::Render(Bit16s *startpos) {
	DWORD msg, timeStamp;
	int buflen = len;
	int playlen;
	Bit16s *bufpos = startpos;

	for(;;) {
		timeStamp = midiStream.PeekMessageTime();
		if (timeStamp == -1)	// if midiStream is empty - exit
			break;

		//	render samples from playCursor to current midiMessage timeStamp
		playlen = int(timeStamp - playCursor);
		if (playlen > buflen)	// if midiMessage is too far - exit
			break;
		if (playlen < 0) {
			std::cout << "Late MIDI message for " << playlen << " samples, " << playlen / 32.f << " ms\n";
		}
		if (playlen > 0) {		// if midiMessage with same timeStamp - skip rendering
			synthEvent.Wait();
			synth->render(bufpos, playlen);
			synthEvent.Release();
			playCursor += playlen;
			bufpos += 2 * playlen;
			buflen -= playlen;
		}

		// play midiMessage
		msg = midiStream.GetMessage();
		synthEvent.Wait();
		synth->playMsg(msg);
		synthEvent.Release();
	}

	//	render rest of samples
	synthEvent.Wait();
	synth->render(bufpos, buflen);
	synthEvent.Release();
	playCursor += buflen;
#if MT32EMU_USE_EXTINT == 1
	if (mt32emuExtInt != NULL) {
		mt32emuExtInt->doControlPanelComm(synth, 4 * len);
	}
#endif
}

MidiSynth::MidiSynth() {
	midiSynth = this;
}

int LoadIntValue(char *key, int nDefault) {
	return GetPrivateProfileIntA("mt32emu", key, nDefault, "mt32emu.ini");
}

DWORD LoadStringValue(char *key, char *nDefault, char *destination, DWORD nSize) {
	return GetPrivateProfileStringA("mt32emu", key, nDefault, destination, nSize, "mt32emu.ini");
}

void MidiSynth::LoadSettings() {
	sampleRate = LoadIntValue("SampleRate", 32000);
	latency = LoadIntValue("Latency", 100);
	len = UINT(sampleRate * latency / 4000.f);
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

	emuDACInputMode = (DACInputMode)LoadIntValue("DACInputMode", DACInputMode_GENERATION2);
	DWORD s = LoadStringValue("PathToROMFiles", "C:/WINDOWS/SYSTEM32/", pathToROMfiles, 254);
	pathToROMfiles[s] = '/';
	pathToROMfiles[s + 1] = 0;
}

void MidiSynth::ApplySettings() {
	synth->setReverbEnabled(reverbEnabled);
	synth->setDACInputMode(emuDACInputMode);
}

int MidiSynth::Init() {
	UINT wResult;

	LoadSettings();

	stream1 = new Bit16s[2 * len];
	stream2 = new Bit16s[2 * len];
	stream3 = new Bit16s[2 * len];
	stream4 = new Bit16s[2 * len];

	//	Init synth
	if (synthEvent.Init()) {
		return 1;
	}
	synth = new Synth();
	SynthProperties synthProp = {sampleRate, true, true, 0, 0, 0, pathToROMfiles,
		NULL, MT32_Report, NULL, NULL, NULL};
	if (!synth->open(synthProp)) {
		MessageBox(NULL, L"Can't open Synth", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	ApplySettings();

	//	Init External Interface
#if MT32EMU_USE_EXTINT == 1
	if (mt32emuExtInt == NULL) {
		mt32emuExtInt = new MT32Emu::ExternalInterface();
	}
	if (mt32emuExtInt != NULL) {
		mt32emuExtInt->start();
	}
#endif

	wResult = waveOut.Init(stream1, stream2, stream3, stream4, len, sampleRate);
	if (wResult) return wResult;

	//	Start playing 4 streams
	synth->render(stream1, len);
	synth->render(stream2, len);
	synth->render(stream3, len);
	synth->render(stream4, len);

	pendingClose = false;

	wResult = waveOut.Start();
	if (wResult) return wResult;

	playCursor = 0;
	_beginthread(&WaveOutWin32::waveOutProc, 16384, NULL);
	return 0;
}

void MidiSynth::SetMasterVolume(UINT masterVolume) {
	Bit8u sysex[] = {0x10, 0x00, 0x16, 0x01};

	sysex[3] = (Bit8u)masterVolume;
	synthEvent.Wait();
	synth->writeSysex(16, sysex, 4);
	synthEvent.Release();
}

int MidiSynth::Reset() {
	UINT wResult;

	ReloadSettings();
	if (!resetEnabled) {
		ApplySettings();
		return 0;
	}

	wResult = waveOut.Pause();
	if (wResult) return wResult;

	synthEvent.Wait();
	synth->close();
	delete synth;

	synth = new Synth();
	SynthProperties synthProp = {sampleRate, true, true, 0, 0, 0, pathToROMfiles,
		NULL, MT32_Report, NULL, NULL, NULL};
	if (!synth->open(synthProp)) {
		MessageBox(NULL, L"Can't open Synth", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}
	ApplySettings();
	synthEvent.Release();

	wResult = waveOut.Resume();
	if (wResult) return wResult;

	return wResult;
}

bool MidiSynth::IsPendingClose() {
	return pendingClose;
}

void MidiSynth::PushMIDI(DWORD msg) {
	midiStream.PutMessage(msg, waveOut.GetPos());
}

void MidiSynth::PlaySysex(Bit8u *bufpos, DWORD len) {
	synthEvent.Wait();
	synth->playSysex(bufpos, len);
	synthEvent.Release();
}

int MidiSynth::Close() {
	UINT wResult;
	pendingClose = true;

	// Close External Interface
#if MT32EMU_USE_EXTINT == 1
	if(mt32emuExtInt != NULL) {
		mt32emuExtInt->stop();
		delete mt32emuExtInt;
		mt32emuExtInt = NULL;
	}
#endif

	synthEvent.Wait();
	wResult = waveOut.Pause();
	if (wResult) return wResult;

	wResult = waveOut.Close();
	if (wResult) return wResult;

	synth->close();

	// Cleanup memory
	delete synth;
	delete stream1;
	delete stream2;

	synthEvent.Close();
	return 0;
}

}
