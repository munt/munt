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

///////////////////////////////////////////////////////////////////////////////
// MMSystem legacy stuff
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <initguid.h>
#include <dmusics.h>
#include <dmusici.h>
#include <mmddk.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <ks.h>

#include "../mt32emu_win32drv.h"

#include "MT32DirectMusicSynth.h" // Just for LOG_MSG

#define MAX_DRIVERS 8
#define MAX_CLIENTS 8 // Per driver

HRESULT RegisterSynth(REFGUID guid, const char szDescription[]);
HRESULT UnregisterSynth(REFGUID guid);

struct Driver {
	bool open;
	int clientCount;
	HDRVR hdrvr;
	struct Client {
		bool allocated;
		DWORD instance;
		DWORD flags;
		DWORD_PTR callback;
	} clients[MAX_CLIENTS];
} drivers[MAX_DRIVERS];

int driverCount;

IDirectMusicPerformance *myPerf = NULL;
IDirectMusic *myDM = NULL;
IDirectMusicPort *myDMPort = NULL;
IDirectMusicBuffer *myDMBuffer = NULL;
IReferenceClock *myLatentClock = NULL;

INT_PTR CALLBACK DialogHandler(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
	case WM_INITDIALOG:
		CheckDlgButton(hwndDlg,IDC_CHECKREVENABLE, BST_CHECKED);
		return 1;

	case WM_COMMAND:
		if (wParam == IDOK) { 
			EndDialog(hwndDlg, 1); 
			return 1; 
		}
		return 0;

	default:
		break;
	}
	return 0;
}

STDAPI_(LONG) DriverProc(DWORD dwDriverID, HDRVR hdrvr, WORD wMessage, DWORD dwParam1, DWORD dwParam2) {
	switch(wMessage) {
	case DRV_LOAD:
		LOG_MSG("DriverProc DRV_LOAD");
		memset(drivers, 0, sizeof(drivers));
		driverCount = 0;
		return DRV_OK;
	case DRV_ENABLE:
		LOG_MSG("DriverProc DRV_ENABLE");
		return DRV_OK;
	case DRV_OPEN:
		LOG_MSG("DriverProc DRV_OPEN driverCount before=%d", driverCount);
		int driverNum;
		if (driverCount == MAX_DRIVERS) {
			LOG_MSG(" Denying open since we've already reached our arbitrary MAX_DRIVERS limit of %d", MAX_DRIVERS);
			return 0;
		} else {
			for (driverNum = 0; driverNum < MAX_DRIVERS; driverNum++) {
				if (!drivers[driverNum].open) {
					break;
				}
				if (driverNum == MAX_DRIVERS) {
					LOG_MSG(" Apparently all our driver slots are allocated, though our open/close counter says they shouldn't be");
					return 0;
				}
			}
		}
		drivers[driverNum].open = true;
		drivers[driverNum].clientCount = 0;
		drivers[driverNum].hdrvr = hdrvr;
		driverCount++;
		return DRV_OK;
	case DRV_INSTALL:
	case DRV_PNPINSTALL:
		if (wMessage == DRV_PNPINSTALL)
			LOG_MSG("DriverProc DRV_PNPINSTALL");
		else
			LOG_MSG("DriverProc DRV_INSTALL");
		RegisterSynth(CLSID_MT32DirectMusicSynth, "MT-32 Synth Emulator");
		return DRV_OK;
	case DRV_QUERYCONFIGURE:
		LOG_MSG("DriverProc DRV_QUERYCONFIGURE");
		return DRV_OK;
	case DRV_CONFIGURE:
		LOG_MSG("DriverProc DRV_CONFIGURE");
		DialogBox(_AtlBaseModule.GetModuleInstance(), (LPCTSTR)IDD_DIALOG1, (HWND)dwParam1, &DialogHandler);
		return DRVCNF_OK;
	case DRV_CLOSE:
		LOG_MSG("DriverProc DRV_CLOSE driverCount before=%ld", driverCount);
		for (int i = 0; i < MAX_DRIVERS; i++) {
			if (drivers[i].open && drivers[i].hdrvr == hdrvr) {
				drivers[i].open = false;
				--driverCount;
				return DRV_OK;
			}
		}
		LOG_MSG(" Denied attempt to close a driver with a handle that doesn't match any we have open");
		return DRV_CANCEL;
	case DRV_DISABLE:
		LOG_MSG("DriverProc DRV_DISABLE");
		return DRV_OK;
	case DRV_FREE:
		LOG_MSG("DriverProc DRV_FREE");
		return DRV_OK;
	case DRV_REMOVE:
		LOG_MSG("DriverProc DRV_REMOVE");
		UnregisterSynth(CLSID_MT32DirectMusicSynth);
		return DRV_OK;
	}
	LOG_MSG("DriverProc DRV_0x%d - unknown", wMessage);
	return DRV_OK;
}


HRESULT modGetCaps(PVOID capsPtr, DWORD capsSize) {
	MIDIOUTCAPSA * myCapsA;
	MIDIOUTCAPSW * myCapsW;
	MIDIOUTCAPS2A * myCaps2A;
	MIDIOUTCAPS2W * myCaps2W;

	CHAR synthName[] = "MT-32 Synth Emulator\0";
	WCHAR synthNameW[] = L"MT-32 Synth Emulator\0";

	LOG_MSG("modMessage GetDevCaps: size %d, caps1 %d caps2 %d", capsSize, sizeof(MIDIOUTCAPS), sizeof(MIDIOUTCAPS2));
	
	switch (capsSize) {
	case (sizeof(MIDIOUTCAPSA)):
		myCapsA = (MIDIOUTCAPSA *)capsPtr;
		myCapsA->wMid = MM_UNMAPPED;
		myCapsA->wPid = MM_MPU401_MIDIOUT;
		memcpy(myCapsA->szPname, synthName, sizeof(synthName));
		myCapsA->wTechnology = MOD_MIDIPORT;
		myCapsA->vDriverVersion = 0x0090;
		myCapsA->wVoices = 0;
		myCapsA->wNotes = 0;
		myCapsA->wChannelMask = 0xffff;
		myCapsA->dwSupport = 0;
		return MMSYSERR_NOERROR;

	case (sizeof(MIDIOUTCAPSW)):
		myCapsW = (MIDIOUTCAPSW *)capsPtr;
		myCapsW->wMid = MM_UNMAPPED;
		myCapsW->wPid = MM_MPU401_MIDIOUT;
		memcpy(myCapsW->szPname, synthNameW, sizeof(synthNameW));
		myCapsW->wTechnology = MOD_MIDIPORT;
		myCapsW->vDriverVersion = 0x0090;
		myCapsW->wVoices = 0;
		myCapsW->wNotes = 0;
		myCapsW->wChannelMask = 0xffff;
		myCapsW->dwSupport = 0;
		return MMSYSERR_NOERROR;

	case (sizeof(MIDIOUTCAPS2A)):
		myCaps2A = (MIDIOUTCAPS2A *)capsPtr;
		myCaps2A->wMid = MM_UNMAPPED;
		myCaps2A->wPid = MM_MPU401_MIDIOUT;
		memcpy(myCaps2A->szPname, synthName, sizeof(synthName));
		myCaps2A->wTechnology = MOD_MIDIPORT;
		myCaps2A->vDriverVersion = 0x0090;
		myCaps2A->wVoices = 0;
		myCaps2A->wNotes = 0;
		myCaps2A->wChannelMask = 0xffff;
		myCaps2A->dwSupport = 0;
		myCaps2A->ManufacturerGuid = CLSID_SynthClassFactory;
		myCaps2A->ProductGuid = CLSID_MT32DirectMusicSynth;
		myCaps2A->NameGuid = CLSID_MT32DirectMusicSynth;
		return MMSYSERR_NOERROR;

	case (sizeof(MIDIOUTCAPS2W)):
		myCaps2W = (MIDIOUTCAPS2W *)capsPtr;
		myCaps2W->wMid = MM_UNMAPPED;
		myCaps2W->wPid = MM_MPU401_MIDIOUT;
		memcpy(myCaps2W->szPname, synthNameW, sizeof(synthNameW));
		myCaps2W->wTechnology = MOD_MIDIPORT;
		myCaps2W->vDriverVersion = 0x0090;
		myCaps2W->wVoices = 0;
		myCaps2W->wNotes = 0;
		myCaps2W->wChannelMask = 0xffff;
		myCaps2W->dwSupport = 0;
		myCaps2W->ManufacturerGuid = CLSID_SynthClassFactory;
		myCaps2W->ProductGuid = CLSID_MT32DirectMusicSynth;
		myCaps2W->NameGuid = CLSID_MT32DirectMusicSynth;
		return MMSYSERR_NOERROR;

	default:
		LOG_MSG("modMessage GetDevCaps: *** Error: Unknown Caps size");
		return MMSYSERR_ERROR;
	}
}

void WriteLog(const char * charMsg, unsigned int uMsg) {
	//LOG_MSG("modMessage '%s' Msg: 0x%x", charMsg, uMsg);
}

bool SetupDirectMusicInterface() {
	HRESULT ntStatus;

	DMUS_PORTPARAMS dummyParms;
	memset(&dummyParms,0,sizeof(dummyParms));
	dummyParms.dwSize = sizeof(DMUS_PORTPARAMS);

	DMUS_BUFFERDESC dummyBuffer;
	dummyBuffer.dwSize = sizeof(DMUS_BUFFERDESC);
	dummyBuffer.dwFlags = 0;
	dummyBuffer.guidBufferFormat = GUID_NULL;
	dummyBuffer.cbBuffer = 2048;

	myPerf = NULL;
	CoInitialize(NULL);
	ntStatus = CoCreateInstance(CLSID_DirectMusicPerformance, NULL, CLSCTX_INPROC, IID_IDirectMusicPerformance8, (void**)&myPerf);
	if(ntStatus == S_OK) {
		myDM = NULL;
		ntStatus = myPerf->Init(&myDM, NULL, NULL);
		if(myDM != NULL) {
			myDMPort = NULL;
			ntStatus = myDM->CreatePort(CLSID_MT32DirectMusicSynth, &dummyParms, &myDMPort, NULL);
			if (myDMPort != NULL) {
				myDMBuffer = NULL;
				ntStatus = myDM->CreateMusicBuffer(&dummyBuffer, &myDMBuffer, NULL);
				if (myDMBuffer != NULL) {
					myDMPort->Activate(true);
					return true;
				} else {
					LOG_MSG("myDM->CreateMusicBuffer() failed (0x%08X)", ntStatus);
				}
				myDMPort->Release();
				myDMPort = NULL;
			} else {
				LOG_MSG("myDM->CreatePort() failed (%08X)", ntStatus);
			}
			myPerf->CloseDown();
			myDM = NULL;
		} else {
			LOG_MSG("myPerf->Init() failed (%08X)", ntStatus);
		}
		myPerf->Release();
		myPerf = NULL;
	} else {
		LOG_MSG("CoCreateInstance() failed (%08X: %s)", ntStatus, ntStatus == REGDB_E_CLASSNOTREG ? "REGDB_E_CLASSNOTREG" : ntStatus == CLASS_E_NOAGGREGATION ? "CLASS_E_NOAGGREGATION" : ntStatus == E_NOINTERFACE ? "E_NOINTERFACE" : "Unknown");
	}
	CoUninitialize();
	return false;
}

void ShutdownDirectMusicInterface() {
	if (myPerf == NULL)
		return;
	myDMPort->Activate(false);
	myDMPort->Release();
	myDMPort = NULL;
	myDMBuffer->Release();
	myDMBuffer = NULL;
	myDM->Release();
	myDM = NULL;
	myPerf->CloseDown();
	myPerf->Release();
	myPerf = NULL;
	CoUninitialize();
}

void DoCallback(int driverNum, int clientNum, DWORD msg, DWORD param1, DWORD param2) {
	Driver::Client *client = &drivers[driverNum].clients[clientNum];
	LOG_MSG("DoCallback(%d, %d, %d, %d, %d): callback=0x%08d, flags=0x%08d, hdrvr=0x%08d, instance=0x%08d", driverNum, clientNum, msg, param1, param2, 
		client->callback, client->flags, drivers[driverNum].hdrvr, client->instance);
	DriverCallback(client->callback, client->flags, drivers[driverNum].hdrvr, msg, client->instance, param1, param2);
}

LONG OpenDriver(Driver *driver, UINT uDeviceID, UINT uMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2) {
	int clientNum;
	if ((driver->clientCount == 0) || (myDMPort == NULL)) {
		if (!SetupDirectMusicInterface()) {
			WriteLog("Setup of DirectMusic interface for MMSystem failed", uMsg);
			return MMSYSERR_ERROR;
		}
		WriteLog("Setup of DirectMusic interface for MMSystem succeeded", uMsg);
		clientNum = 0;
	} else if (driver->clientCount == MAX_CLIENTS) {
		WriteLog("Attempt to open more than the MAX_CLIENTS we arbitrarily support denied", uMsg);
		return MMSYSERR_ALLOCATED;
	} else {
		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (!driver->clients[i].allocated) {
				break;
			}
		}
		if (i == MAX_CLIENTS) {
			WriteLog("Something went disastrously wrong, we have more clients in the table than is correct for our open/close counts", uMsg);
			return MMSYSERR_ALLOCATED;
		}
		clientNum = i;
	}
	MIDIOPENDESC *desc = (MIDIOPENDESC *)dwParam1;
	driver->clients[clientNum].allocated = true;
	driver->clients[clientNum].flags = HIWORD(dwParam2);
	driver->clients[clientNum].callback = desc->dwCallback;
	driver->clients[clientNum].instance = desc->dwInstance;
	*(LONG *)dwUser = clientNum;
	driver->clientCount++;
	DoCallback(uDeviceID, clientNum, MOM_OPEN, NULL, NULL);
	return MMSYSERR_NOERROR;
}

LONG CloseDriver(Driver *driver, UINT uDeviceID, UINT uMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2) {
	if (!driver->clients[dwUser].allocated) {
		WriteLog("MODM_CLOSE for a client we don't have marked as allocated", uMsg);
		return MMSYSERR_INVALPARAM;
	}
	driver->clients[dwUser].allocated = false;
	driver->clientCount--;
	if(driver->clientCount <= 0) {
		WriteLog("Close with deinitialization", uMsg);
		ShutdownDirectMusicInterface();
	} else {
		WriteLog("Close without deinitialization", uMsg);
	}
	DoCallback(uDeviceID, dwUser, MOM_CLOSE, NULL, NULL);
	return MMSYSERR_NOERROR;
}

STDAPI_(LONG) midMessage(UINT uDeviceID, UINT uMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2) {
	Driver *driver = &drivers[uDeviceID];
	switch (uMsg) {
	case MIDM_OPEN:
		return OpenDriver(driver, uDeviceID, uMsg, dwUser, dwParam1, dwParam2);

	case MIDM_CLOSE:
		return CloseDriver(driver, uDeviceID, uMsg, dwUser, dwParam1, dwParam2);

	default:
		WriteLog("Unhandled message", uMsg);
		return MMSYSERR_NOERROR;
		break;
	}
}

STDAPI_(LONG) modMessage(UINT uDeviceID, UINT uMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2) {
	REFERENCE_TIME rt;
	MIDIHDR *midiHdr;
	Driver *driver = &drivers[uDeviceID];
	switch (uMsg) {
	case MODM_OPEN:
		return OpenDriver(driver, uDeviceID, uMsg, dwUser, dwParam1, dwParam2);

	case MODM_CLOSE:
		return CloseDriver(driver, uDeviceID, uMsg, dwUser, dwParam1, dwParam2);

	case MODM_PREPARE:
		return MMSYSERR_NOTSUPPORTED;

	case MODM_UNPREPARE:
		return MMSYSERR_NOTSUPPORTED;

	case MODM_GETDEVCAPS:
		WriteLog("Query of device caps",uMsg);
		return modGetCaps((PVOID)dwParam1, dwParam2);

	case MODM_DATA:
		if (myPerf == NULL) {
			WriteLog("MODM_DATA before MODM_OPEN!", uMsg);
			return MMSYSERR_ERROR;
		}
		WriteLog("MODM_DATA", uMsg);
		WriteLog("Sent data",dwParam1);
		myDMBuffer->Flush();
		myDMPort->GetLatencyClock(&myLatentClock);
		myLatentClock->GetTime(&rt);
		myDMBuffer->PackStructured(rt, 1, dwParam1);
		myDMPort->PlayBuffer((LPDIRECTMUSICBUFFER)myDMBuffer);
		return MMSYSERR_NOERROR;
	case MODM_LONGDATA:
		WriteLog("MODM_LONGDATA", uMsg);
		if (myPerf == NULL) {
			WriteLog("MODM_LONGDATA before MODM_OPEN!", uMsg);
			return MMSYSERR_ERROR;
		}
		midiHdr = (MIDIHDR *)dwParam1;
		if ((midiHdr->dwFlags & MHDR_PREPARED) == 0) {
			WriteLog("MODM_LONGDATA with unprepared buffer!", uMsg);
			return MIDIERR_UNPREPARED;
		}
		myDMBuffer->Flush();
		myDMPort->GetLatencyClock(&myLatentClock);
		myLatentClock->GetTime(&rt);
		LOG_MSG(" Size=%ld", midiHdr->dwBufferLength);
		myDMBuffer->PackUnstructured(rt, 1, midiHdr->dwBufferLength, (unsigned char *)midiHdr->lpData);
		myDMPort->PlayBuffer((LPDIRECTMUSICBUFFER)myDMBuffer);
		midiHdr->dwFlags = MHDR_DONE;
		midiHdr->dwFlags &= ~MHDR_INQUEUE;
		DoCallback(uDeviceID, dwUser, MOM_DONE, dwParam1, NULL);
 		return MMSYSERR_NOERROR;

	case MODM_GETNUMDEVS:
		return 0x1;

	default:
		WriteLog("Unhandled message",uMsg);
		return MMSYSERR_NOERROR;
		break;
	}
}
