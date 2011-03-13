/* Copyright (C) 2003, 2004, 2005 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

#ifndef __MT32WIN32DRV_MT32DIRECTMUSICSYNTH_H__
#define __MT32WIN32DRV_MT32DIRECTMUSICSYNTH_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include <ks.h>
#include <ksproxy.h>
#include <initguid.h>
#include <dmusics.h>

#include "mt32emu.h"
#if MT32EMU_USE_EXTINT == 1
#include "externalInterface.h"
#endif

#include "MidiEvent.h"

#include "resource.h"

// This magic lets the ATL macros know about DirectX classes
struct __declspec(uuid("{09823661-5C85-11D2-AFA6-00AA0024D8B6}")) IDirectMusicSynth;
struct __declspec(uuid("{28F54685-06FD-11D2-B27A-00A0C9223196}")) IKsControl;

class MT32DirectMusicSynth : 
	public CComObjectRoot,
	public CComCoClass<MT32DirectMusicSynth,&CLSID_MT32DirectMusicSynth>,
	public IDispatchImpl<IMT32DirectMusicSynth, &IID_IMT32DirectMusicSynth, &LIBID_MT32EmuLib>, 
	public ISupportErrorInfo,
	public IDirectMusicSynth,
	public IKsControl
{
private:
	LPDMUS_PORTPARAMS portParams;
	IDirectMusicSynthSink *sink;
	IReferenceClock  *masterClock;
	BOOL enabled;
	bool open;

	MT32Emu::Synth *mt32emuSynth;
#if MT32EMU_USE_EXTINT == 1
	MT32Emu::ExternalInterface *mt32emuExtInt;
#endif

	HANDLE eventsMutex;
	MidiEvent *events;

	long long PeekNextMidiEventTime(long long def);
	MidiEvent *DequeueMidiEvent(long long maxTime);
	void EnqueueMidiEvent(MidiEvent *event);
public:
	MT32DirectMusicSynth();
	~MT32DirectMusicSynth();

	MT32Emu::Synth *getMT32EmuSynth() const;
#if MT32EMU_USE_EXTINT == 1
	MT32Emu::ExternalInterface *getMT32EmuExternalInterface() const;
#endif

	BEGIN_COM_MAP(MT32DirectMusicSynth)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IMT32DirectMusicSynth)
		COM_INTERFACE_ENTRY(ISupportErrorInfo)
		COM_INTERFACE_ENTRY(IDirectMusicSynth)
		COM_INTERFACE_ENTRY(IKsControl)
	END_COM_MAP()

	DECLARE_REGISTRY_RESOURCEID(IDR_MT32DIRECTMUSICSYNTH)

	// ISupportErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	// IDirectMusicSynth
	STDMETHOD(Open)                 (THIS_ LPDMUS_PORTPARAMS pPortParams);
	STDMETHOD(Close)                (THIS);
	STDMETHOD(SetNumChannelGroups)  (THIS_ DWORD dwGroups);
	STDMETHOD(Download)             (THIS_ LPHANDLE phDownload, 
											LPVOID pvData, 
											LPBOOL pbFree );
	STDMETHOD(Unload)               (THIS_ HANDLE hDownload, 
											HRESULT ( CALLBACK *lpFreeHandle)(HANDLE,HANDLE), 
											HANDLE hUserData ); 
	STDMETHOD(PlayBuffer)           (THIS_ REFERENCE_TIME rt, 
											LPBYTE pbBuffer, 
											DWORD cbBuffer);
	STDMETHOD(GetRunningStats)      (THIS_ LPDMUS_SYNTHSTATS pStats);
	STDMETHOD(GetPortCaps)          (THIS_ LPDMUS_PORTCAPS pCaps);
	STDMETHOD(SetMasterClock)       (THIS_ IReferenceClock *pClock);
	STDMETHOD(GetLatencyClock)      (THIS_ IReferenceClock **ppClock);
	STDMETHOD(Activate)             (THIS_ BOOL fEnable);
	STDMETHOD(SetSynthSink)         (THIS_ IDirectMusicSynthSink *pSynthSink);
	STDMETHOD(Render)               (THIS_ short *pBuffer, 
											DWORD dwLength, 
											LONGLONG llPosition);
	STDMETHOD(SetChannelPriority)   (THIS_ DWORD dwChannelGroup,
											DWORD dwChannel,
											DWORD dwPriority);
	STDMETHOD(GetChannelPriority)   (THIS_ DWORD dwChannelGroup,
											DWORD dwChannel,
											LPDWORD pdwPriority);
	STDMETHOD(GetFormat)            (THIS_ LPWAVEFORMATEX pWaveFormatEx,
											LPDWORD pdwWaveFormatExSize);
	STDMETHOD(GetAppend)            (THIS_ DWORD* pdwAppend);
	    
	// IKsControl

	STDMETHOD(KsProperty)			(THIS_ IN PKSPROPERTY Property,
										IN ULONG PropertyLength,
										IN OUT LPVOID PropertyData,
										IN ULONG DataLength,
										OUT ULONG* BytesReturned
									);
	STDMETHOD(KsMethod)				(THIS_ IN PKSMETHOD Method,
										IN ULONG MethodLength,
										IN OUT LPVOID MethodData,
										IN ULONG DataLength,
										OUT ULONG* BytesReturned
									);
	STDMETHOD(KsEvent)				(THIS_ IN PKSEVENT Event OPTIONAL,
										IN ULONG EventLength,
										IN OUT LPVOID EventData,
										IN ULONG DataLength,
										OUT ULONG* BytesReturned
									);
};

OBJECT_ENTRY_AUTO(__uuidof(MT32DirectMusicSynth), MT32DirectMusicSynth)

void LOG_MSG(char *fmt, ...);

#endif
