/* Copyright (c) 2003-2004 Various contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "stdafx.h"

//#define BENCHMARK

#ifdef BENCHMARK
#include <time.h>
#endif
#include <ks.h>
#include <initguid.h>
#include <dmusics.h>
#include <dmksctrl.h>
#include <dmusprop.h>

#include "../mt32emu_win32drv.h"
#include "MT32DirectMusicSynth.h"

// Common values: 8.0 kHz, 11.025 kHz, 22.05 kHz, and 44.1 kHz
// The tuning is slightly off for 22050 (probably anything other than 32KHz)
//#define SAMPLERATE 22050
#define SAMPLERATE 44100
//#define SAMPLERATE 32000

int MT32_Report(void *userData, MT32Emu::ReportType type, const void *reportData) {
#if MT32EMU_USE_EXTINT == 1
	MT32DirectMusicSynth *dms = (MT32DirectMusicSynth *)userData;
	dms->getMT32EmuExternalInterface()->handleReport(dms->getMT32EmuSynth(), type, reportData);
#endif
	return 0;
}

void MT32_PrintDebug(void *userData, const char *fmt, va_list args) {
#ifdef _DEBUG
	char buf[1024];
	vsprintf(buf, fmt, args);
	strcat(buf, "\n");
	OutputDebugString(buf);
#endif
}

void LOG_MSG(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	MT32_PrintDebug(NULL, fmt, ap);
	va_end(ap);
}

MT32DirectMusicSynth::MT32DirectMusicSynth() {
	//LOG_MSG("MT32DirectMusicSynth::MT32DirectMusicSynth()");
	dwUsageCount = 0;
	enabled = false;
	open = false;

	int i;
	// Couldn't figure out a better way to do this.  VC6.0 doesn't see that the
	// first element in KSPROPERTY is a GUID for some reason and was giving me
	// errors trying to convert it to a ULONG.  This works though;
	for(i=0;i<10;i++) {
		switch(i) {
		case 0:
			ksSynthProperties[i].Set = GUID_DMUS_PROP_GM_Hardware;
			ksSynthProperties[i].Id = 0;
			ksSynthProperties[i].Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
			//ksSynthProperties[i].Alignment = 0;
 			break;
		case 1:
			ksSynthProperties[i].Set = GUID_DMUS_PROP_Effects;
			ksSynthProperties[i].Id = 0;
			ksSynthProperties[i].Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT;
			//ksSynthProperties[i].Alignment = 0;
 			break;
		case 2:
			ksSynthProperties[i].Set = KSPROPSETID_Synth;
			ksSynthProperties[i].Id = KSPROPERTY_SYNTH_CAPS;
			ksSynthProperties[i].Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
			//ksSynthProperties[i].Alignment = 0;
 			break;
		case 3:
			ksSynthProperties[i].Set = KSPROPSETID_Synth;
			ksSynthProperties[i].Id = KSPROPERTY_SYNTH_PORTPARAMETERS;
			ksSynthProperties[i].Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
			//ksSynthProperties[i].Alignment = 0;
 			break;
		case 4:
			ksSynthProperties[i].Set = KSPROPSETID_Synth;
			ksSynthProperties[i].Id = KSPROPERTY_SYNTH_VOLUME;
			ksSynthProperties[i].Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
			//ksSynthProperties[i].Alignment = 0;
 			break;
		case 5:
			ksSynthProperties[i].Set = KSPROPSETID_Synth;
			ksSynthProperties[i].Id = KSPROPERTY_SYNTH_VOLUMEBOOST;
			ksSynthProperties[i].Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
			//ksSynthProperties[i].Alignment = 0;
 			break;
		case 6:
			ksSynthProperties[i].Set = KSPROPSETID_Synth;
			ksSynthProperties[i].Id = KSPROPERTY_SYNTH_CHANNELGROUPS;
			ksSynthProperties[i].Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
			//ksSynthProperties[i].Alignment = 0;
 			break;
		case 7:
			ksSynthProperties[i].Set = KSPROPSETID_Synth;
			ksSynthProperties[i].Id = KSPROPERTY_SYNTH_VOICEPRIORITY;
			ksSynthProperties[i].Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
			//ksSynthProperties[i].Alignment = 0;
 			break;
		case 8:
			ksSynthProperties[i].Set = KSPROPSETID_Synth;
			ksSynthProperties[i].Id = KSPROPERTY_SYNTH_RUNNINGSTATS;
			ksSynthProperties[i].Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
			//ksSynthProperties[i].Alignment = 0;
 			break;
		case 9:
			ksSynthProperties[i].Set = KSPROPSETID_Synth;
			ksSynthProperties[i].Id = KSPROPERTY_SYNTH_LATENCYCLOCK;
			ksSynthProperties[i].Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
			//ksSynthProperties[i].Alignment = 0;
 			break;

		default:
			break;
		}
	}

	myMT32 = new MT32Emu::Synth();
#if MT32EMU_USE_EXTINT == 1
	extComm = new MT32Emu::ExternalInterface();
#endif
	eventsMutex = CreateMutex( NULL, FALSE, NULL );
	events = NULL;

	pSink = NULL;
}

MT32DirectMusicSynth::~MT32DirectMusicSynth() {
	//LOG_MSG("MT32DirectMusicSynth::~MT32DirectMusicSynth()");
#if MT32EMU_USE_EXTINT == 1
	if(extComm != NULL) {
		extComm->stop();
		delete extComm;
	}
#endif
	myMT32->close();
	delete myMT32;
	myMT32 = NULL;
	if (pSink != NULL)
		pSink->Release();
	while (events) {
		MidiEvent *next = events->getNext();
		delete events;
		events = next;
	}
	CloseHandle(eventsMutex);
}

MT32Emu::Synth *MT32DirectMusicSynth::getMT32EmuSynth() const {
	return myMT32;
}

#if MT32EMU_USE_EXTINT == 1
MT32Emu::ExternalInterface *MT32DirectMusicSynth::getMT32EmuExternalInterface() const {
	return extComm;
}
#endif

STDMETHODIMP MT32DirectMusicSynth::InterfaceSupportsErrorInfo(REFIID riid) {
	//LOG_MSG("MT32DirectMusicSynth::InterfaceSupportsErrorInfo()");
	static const IID* arr[] = {
		&IID_IMT32DirectMusicSynth,
	};

	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++) {
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

HRESULT MT32DirectMusicSynth::Open(THIS_ LPDMUS_PORTPARAMS pPortParams) {
	char dataPath[2048];
	if(!open) {
		pSynthParams = pPortParams;
		if(getenv("SystemRoot") == NULL) {
			if(getenv("windir") == NULL) {
				// Eventually we'll query from the registry
				strcpy(&dataPath[0], "c:\\windows");
			}
			else
				strcpy(&dataPath[0], getenv("windir"));
		}
		else
			strcpy(&dataPath[0], getenv("SystemRoot"));
		strcat(&dataPath[0],"\\system32\\");

		MT32Emu::SynthProperties synthp;
		memset(&synthp, 0, sizeof(synthp));
		synthp.sampleRate = SAMPLERATE;
		synthp.useReverb = true;
		synthp.useDefaultReverb = false;
		synthp.reverbType = 0;
		synthp.reverbTime = 5;
		synthp.reverbLevel = 3;
		synthp.userData = this;
		synthp.printDebug = MT32_PrintDebug;
		synthp.report = MT32_Report;
		synthp.baseDir = &dataPath[0];
		if(!myMT32->open(synthp))
			return DMUS_E_DRIVER_FAILED;
#if MT32EMU_USE_EXTINT == 1
		if (extComm != NULL) {
			extComm->start();
		}
#endif
		open = true;
		return S_OK;
	}
	else
		return DMUS_E_ALREADYOPEN;
}

HRESULT MT32DirectMusicSynth::Close(THIS) {
	if(open) {
		open = false;
#if MT32EMU_USE_EXTINT == 1
		if (extComm != NULL) {
			extComm->stop();
		}
#endif
		myMT32->close();
		return S_OK;
	} else {
		return DMUS_E_ALREADYCLOSED;
	}
}

HRESULT MT32DirectMusicSynth::GetAppend(THIS_ DWORD *pdwAppend) {
	// MT-32 doesn't need to do DLS downloads
	return E_NOTIMPL;
}

HRESULT MT32DirectMusicSynth::Download(THIS_ LPHANDLE phDownload, LPVOID pvData, LPBOOL pbFree) {
	return E_NOTIMPL;
}

HRESULT MT32DirectMusicSynth::Unload(THIS_ HANDLE hDownload,HRESULT(CALLBACK *lpFreeHandle)(HANDLE,HANDLE),HANDLE hUserData) {
	return E_NOTIMPL;
}

HRESULT MT32DirectMusicSynth::GetFormat(THIS_ LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize) {
	if(pWaveFormatEx==NULL) {
		return sizeof(LPWAVEFORMATEX);
	} else {
		pWaveFormatEx->cbSize = 0;
		pWaveFormatEx->nAvgBytesPerSec = SAMPLERATE * 2 * 2;
		pWaveFormatEx->nBlockAlign = (2*16)/8;
		pWaveFormatEx->wFormatTag = WAVE_FORMAT_PCM;
		pWaveFormatEx->nChannels = 2;
		pWaveFormatEx->nSamplesPerSec = SAMPLERATE;
		pWaveFormatEx->wBitsPerSample = 16;
		return S_OK;
	}
}

HRESULT MT32DirectMusicSynth::SetChannelPriority(THIS_ DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority) {
	return S_OK;
}

HRESULT MT32DirectMusicSynth::GetChannelPriority(THIS_ DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority) {
	return E_NOTIMPL;
}

HRESULT MT32DirectMusicSynth::SetNumChannelGroups(THIS_ DWORD dwGroups) {
	return S_OK;
}

HRESULT MT32DirectMusicSynth::GetLatencyClock(THIS_ IReferenceClock **pClock) {
	pSink->GetLatencyClock(pClock);
	return S_OK;
}

class BufferReader {
public:
	LPBYTE pbBufPtr;
	signed long dwLen;
	REFERENCE_TIME sTime;

	BufferReader(REFERENCE_TIME startTime, LPBYTE pbBuffer, DWORD len) {
		sTime = startTime;
		pbBufPtr = pbBuffer;
		dwLen = len;
	}

	~BufferReader() {
	}

	HRESULT GetNextEvent(LPREFERENCE_TIME prt, LPDWORD pdwChannelGroup, LPDWORD pdwLength, LPBYTE* ppData) {
		if (dwLen<=0)
			return S_FALSE;

		DMUS_EVENTHEADER *evtPtr = (DMUS_EVENTHEADER *)pbBufPtr;
		DWORD blockSize = DMUS_EVENT_SIZE(evtPtr->cbEvent);

		*prt = sTime + evtPtr->rtDelta;
		*pdwChannelGroup = evtPtr->dwChannelGroup;
		*pdwLength = evtPtr->cbEvent;
		*ppData = (pbBufPtr + sizeof(DMUS_EVENTHEADER));

		dwLen -= blockSize;
		pbBufPtr += blockSize;

		return S_OK;
	}
};

long long MT32DirectMusicSynth::PeekNextMidiEventTime(long long def) {
	WaitForSingleObject(eventsMutex, INFINITE);
	LONGLONG nextTime;
	if (events) {
		nextTime = events->getTime();
	} else {
		nextTime = def;
	}
	ReleaseMutex(eventsMutex);
	return nextTime;
}

MidiEvent *MT32DirectMusicSynth::DequeueMidiEvent(long long maxTime) {
	WaitForSingleObject(eventsMutex, INFINITE);
	MidiEvent *result;
	if (events != NULL && events->getTime() <= maxTime) {
		result = events;
		events = events->getNext();
	} else {
		result = NULL;
	}
	ReleaseMutex(eventsMutex);
	return result;
}

void MT32DirectMusicSynth::EnqueueMidiEvent(MidiEvent *event) {
	WaitForSingleObject(eventsMutex, INFINITE);
	MidiEvent *lastEvent = NULL;
	MidiEvent *nextEvent = events;
	for (;;) {
		if (nextEvent == NULL || nextEvent->getTime() > event->getTime()) {
			event->chainEvent(nextEvent);
			if (lastEvent == NULL) {
				events = event;
			} else {
				lastEvent->chainEvent(event);
			}
			break;
		}
		lastEvent = nextEvent;
		nextEvent = nextEvent->getNext();
	}
	ReleaseMutex(eventsMutex);
}

HRESULT MT32DirectMusicSynth::PlayBuffer(THIS_ REFERENCE_TIME rt, LPBYTE pbBuffer, DWORD cbBuffer) {
#ifdef BENCHMARK
	LARGE_INTEGER freq;
	if (!QueryPerformanceFrequency(&freq)) {
		LOG_MSG("QueryPerformanceFrequency() failed!");
		freq.QuadPart = 0;
	}
	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);
#endif
	LONGLONG stTime;
	MidiEvent *tmpEvent;
	BufferReader *myReader = new BufferReader(rt, pbBuffer, cbBuffer);

	REFERENCE_TIME bufRt;
	DWORD chanGroup;
	DWORD bufLen;
	LPBYTE bufData;

	while(myReader->GetNextEvent(&bufRt, &chanGroup, &bufLen, &bufData) != S_FALSE) {
		pSink->RefTimeToSample(bufRt, &stTime);
		if(bufLen <= sizeof(DWORD)) {
			tmpEvent = new MidiEvent();
			tmpEvent->assignMsg(*(DWORD *)bufData, stTime);
			EnqueueMidiEvent(tmpEvent);
		} else {
			tmpEvent = new MidiEvent();
			tmpEvent->assignSysex(bufData,bufLen,stTime);
			EnqueueMidiEvent(tmpEvent);
		}
	}

	delete myReader;

#ifdef BENCHMARK
	LARGE_INTEGER end;
	QueryPerformanceCounter(&end);
	long long duration = end.QuadPart - start.QuadPart;
	duration /= freq.QuadPart / 1000;
	if (duration > 0)
		LOG_MSG("PlayBuffer took %I64d", duration);
#endif
	return S_OK;
}

HRESULT MT32DirectMusicSynth::SetMasterClock(THIS_ IReferenceClock *pClock) {
	this->pClock = pClock;
	return S_OK;
}

HRESULT MT32DirectMusicSynth::GetPortCaps(THIS_ LPDMUS_PORTCAPS pCaps) {
	pCaps->dwClass = DMUS_PC_OUTPUTCLASS;
	pCaps->dwEffectFlags = DMUS_EFFECT_REVERB;
	pCaps->dwFlags = DMUS_PC_DIRECTSOUND | DMUS_PC_GMINHARDWARE; //| DMUS_PC_SOFTWARESYNTH;
	pCaps->dwMaxAudioChannels = 2;
	pCaps->dwMaxChannelGroups = 1;
	pCaps->dwMaxVoices = 32;
	pCaps->dwMemorySize = 0;
	pCaps->dwSize = sizeof(DMUS_PORTCAPS);
	//pCaps->dwType = DMUS_PORT_USER_MODE_SYNTH;
	pCaps->dwType = DMUS_PORT_WINMM_DRIVER;
	pCaps->guidPort = CLSID_MT32DirectMusicSynth;
	lstrcpyW(&pCaps->wszDescription[0], L"MT-32 Synth Emulator");
	return S_OK;
}

HRESULT MT32DirectMusicSynth::GetRunningStats(THIS_ LPDMUS_SYNTHSTATS pStats) {
	return E_NOTIMPL;
}

HRESULT MT32DirectMusicSynth::Activate(THIS_ BOOL fEnable) {
	pSink->Activate(fEnable);
	if((!fEnable) && (!enabled)) {
		return S_FALSE;
	} else if (fEnable && enabled) {
		return DMUS_E_SYNTHACTIVE;
	} else {
		enabled = fEnable;
		return S_OK;
	}
}

HRESULT MT32DirectMusicSynth::Render(THIS_ short *pBuffer, DWORD dwLength, LONGLONG llPosition) {
	if(!enabled) {
		return DMUS_E_SYNTHINACTIVE;
	}
#ifdef BENCHMARK
	LARGE_INTEGER freq;
	if (!QueryPerformanceFrequency(&freq)) {
		LOG_MSG("QueryPerformanceFrequency() failed!");
		freq.QuadPart = 0;
	}
	double div = (double)(freq.QuadPart / 1000);
#endif
	while (dwLength > 0) {
		DWORD thisLength = dwLength;

#ifdef BENCHMARK
		double durationMIDI;
		double durationSample;
		LARGE_INTEGER start;
		LARGE_INTEGER end;
		QueryPerformanceCounter(&start);
#endif
		MidiEvent *event;
		while ((event = DequeueMidiEvent(llPosition)) != NULL) {
			switch(event->getType()) {
			case ShortMsg:
				myMT32->playMsg(event->midiMsg);
				break;
			case SysexData:
				myMT32->playSysex(event->sysexInfo, event->sysexLen);
				break;
			default:
				// Shouldn't ever get here
				LOG_MSG("*** Error: Render() hit an unknown message type");
				break;
			}
			delete event;
		}
		LONGLONG nextMidiEventIn = PeekNextMidiEventTime(llPosition + thisLength) - llPosition;
		if (nextMidiEventIn <= 0) {
			LOG_MSG("*** Warning: nextMidiEventIn==0");
		}
		else if (nextMidiEventIn < thisLength) {
			LOG_MSG("*** Warning: MIDI required in %ld, but samples want to play for %ld", (DWORD)nextMidiEventIn, thisLength);
			thisLength = (DWORD)nextMidiEventIn;
		}
#ifdef BENCHMARK
		QueryPerformanceCounter(&end);
		durationMIDI = (end.QuadPart - start.QuadPart) / div;
		QueryPerformanceCounter(&start);
#endif

#if MT32EMU_USE_EXTINT == 1
		if (extComm != NULL) {
			extComm->doControlPanelComm(getMT32EmuSynth(), dwLength);
		}
#endif
		myMT32->render(pBuffer, thisLength);
		pBuffer += thisLength * 2; // One for each channel
		llPosition += thisLength;
		dwLength -= thisLength;
#ifdef BENCHMARK
		QueryPerformanceCounter(&end);
		durationSample = (end.QuadPart - start.QuadPart) / div;
		double durationTotal = durationMIDI + durationSample;
		if (durationTotal > 0.0) {
			double samplesPerMilli = thisLength / durationTotal;
			if (samplesPerMilli < 70.0)
				LOG_MSG("%04ld samples at %.2f s/milli (MIDI: %.2f millis, Sample: %.2f millis)", thisLength, samplesPerMilli, durationMIDI, durationSample);
		}
#endif
	}
	return S_OK;
}

HRESULT MT32DirectMusicSynth::SetSynthSink(THIS_ IDirectMusicSynthSink *pSynthSink){
	if (pSink != NULL)
		pSink->Release();
	pSink = pSynthSink;
	pSink->AddRef();
	pSink->Init((IDirectMusicSynth *)this);
	pSink->SetMasterClock(pClock);
	return S_OK;
}

HRESULT MT32DirectMusicSynth::KsProperty(THIS_ IN PKSPROPERTY Property, IN ULONG PropertyLength, IN OUT LPVOID PropertyData, IN ULONG DataLength, OUT ULONG* BytesReturned) {
	const WORD c_wMaxProps = SIZEOF_ARRAY(ksSynthProperties);
	WORD wPropIdx;

	for (wPropIdx = 0; wPropIdx < c_wMaxProps; wPropIdx++) {
		if ((ksSynthProperties[wPropIdx].Set == Property->Set) && (ksSynthProperties[wPropIdx].Id == Property->Id)) {
			// if return buffer can hold a ULONG, return the access flags
			PULONG AccessFlags = (PULONG)PropertyData;
			*AccessFlags = ksSynthProperties[wPropIdx].Flags;
			// set the return value size
			*BytesReturned = sizeof(ULONG);
			break;
		}
	}

	if (wPropIdx == c_wMaxProps) {
		*BytesReturned = 0;
		return DMUS_E_UNKNOWN_PROPERTY;
	}
	return S_OK;
}

HRESULT MT32DirectMusicSynth::KsEvent(THIS_ IN PKSEVENT Event OPTIONAL, IN ULONG EventLength, IN OUT LPVOID EventData, IN ULONG DataLength, OUT ULONG* BytesReturned) {
	return E_NOTIMPL;
}
							
HRESULT MT32DirectMusicSynth::KsMethod(THIS_ IN PKSMETHOD Method, IN ULONG MethodLength, IN OUT LPVOID MethodData, IN ULONG DataLength, OUT ULONG* BytesReturned) {
	return E_NOTIMPL;
}
