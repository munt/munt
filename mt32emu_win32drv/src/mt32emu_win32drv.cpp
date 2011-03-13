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

// Implementation of DLL Exports (mt32emu_winmm.cpp contains DriverProc and modMessage)

#include "stdafx.h"

#include "resource.h"
#include "../mt32emu_win32drv.h"
#include "MT32DirectMusicSynth.h"

HRESULT RegisterSynth(REFGUID guid, const char szDescription[]);
HRESULT UnregisterSynth(REFGUID guid);

const char cszSynthRegRoot[] = REGSTR_PATH_SOFTWARESYNTHS "\\";
const char cszDescriptionKey[] = "Description";
const int CLSID_STRING_SIZE = 39;

LONG driverCount;

class CMT32EmuModule : public CAtlDllModuleT< CMT32EmuModule >
{
public :
	DECLARE_LIBID(LIBID_MT32EmuLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_MT32EMU, "{8F205822-B2CA-403A-B00C-50E4CD3ABBF9}")
};

CMT32EmuModule _AtlModule;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
	LOG_MSG("DllMain, dwReason=%d", dwReason);
	return _AtlModule.DllMain(dwReason, lpReserved); 
}

// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void) {
	HRESULT rc = _AtlModule.DllCanUnloadNow();
	LOG_MSG("DllCanUnloadNow(): Returning 0x%08x", rc);
	return rc;
}

// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
	HRESULT hr = _AtlModule.DllGetClassObject(rclsid, riid, ppv);
	LOG_MSG("DllGetClassObject(): Returning 0x%08x", hr);
	return hr;
}

// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void) {
	HRESULT hr = RegisterSynth(CLSID_MT32DirectMusicSynth, "MT-32 Synth Emulator");
	// registers object, typelib and all interfaces in typelib
	LOG_MSG("DllRegisterServer(): Returning 0x%08x", hr);
	return hr;
}

// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void) {
	HRESULT hr = UnregisterSynth(CLSID_MT32DirectMusicSynth);
	LOG_MSG("DllUnregisterServer(): Returning 0x%08x");
	return hr;
}

HRESULT ClassIDCat(char *str, const CLSID &rclsid) {
	str = str + strlen(str);
	LPWSTR lplpsz;
	HRESULT hr = StringFromCLSID(rclsid, &lplpsz);
	if (SUCCEEDED(hr)) {
		WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, lplpsz, CLSID_STRING_SIZE, str, CLSID_STRING_SIZE+1, NULL, NULL);
		CoTaskMemFree(lplpsz);
	} else {
		LOG_MSG("RegisterSynth() failed: StringFromCLSID() returned 0x%08x", hr);
	}
	return hr;
}

HRESULT RegisterSynth(REFGUID guid, const char szDescription[]) {
	HKEY hk;
	char szRegKey[256];

	strcpy(szRegKey, cszSynthRegRoot);

	HRESULT hr = ClassIDCat(szRegKey, guid);
	if (!SUCCEEDED(hr)) {
		LOG_MSG("RegisterSynth() failed transforming class ID");
		return hr;
	}
	hr = RegCreateKey(HKEY_LOCAL_MACHINE, szRegKey, &hk);
	if (!SUCCEEDED(hr)) {
		LOG_MSG("RegisterSynth() failed creating registry key");
		return hr;
	}

	hr = RegSetValueEx(hk, cszDescriptionKey, 0L, REG_SZ, (const unsigned char *)szDescription, (DWORD)strlen(szDescription) + 1);
	RegCloseKey(hk);
	if (!SUCCEEDED(hr)) {
		LOG_MSG("RegisterSynth() failed creating registry value");
		return hr;
	}

	hr = _AtlModule.DllRegisterServer(false);
	return hr;
}

HRESULT UnregisterSynth(REFGUID guid) {
	char szRegKey[256];

	strcpy(szRegKey, cszSynthRegRoot);

	HRESULT hr = ClassIDCat(szRegKey, guid);
	if (!SUCCEEDED(hr)) {
		LOG_MSG("UnregisterSynth() failed transforming class ID");
		return hr;
	}

	hr = RegDeleteKey(HKEY_LOCAL_MACHINE, szRegKey);
	if (!SUCCEEDED(hr)) {
		LOG_MSG("UnregisterSynth() failed deleting registry key");
		return hr;
	}
	hr = _AtlModule.DllUnregisterServer(false);
	return hr;
}
