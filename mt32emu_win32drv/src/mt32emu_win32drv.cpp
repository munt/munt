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
	LOG_MSG("DllCanUnloadNow(): Returning 0x%08d", rc);
	return rc;
}

// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
	HRESULT hr = _AtlModule.DllGetClassObject(rclsid, riid, ppv);
	LOG_MSG("DllGetClassObject(): Returning 0x%08d", hr);
	return hr;
}

// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void) {
	RegisterSynth(CLSID_MT32DirectMusicSynth, "MT-32 Synth Emulator");
	// registers object, typelib and all interfaces in typelib
	HRESULT hr = _AtlModule.DllRegisterServer(false);
	LOG_MSG("DllRegisterServer(): Returning 0x%08d", hr);
	return hr;
}

// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void) {
	UnregisterSynth(CLSID_MT32DirectMusicSynth);
	HRESULT hr = _AtlModule.DllUnregisterServer(false);
	LOG_MSG("DllUnregisterServer(): Returning 0x%08d");
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
		LOG_MSG("RegisterSynth() failed: StringFromCLSID() returned 0x%08d", hr);
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
	if (!SUCCEEDED(hr)) {
		LOG_MSG("RegisterSynth() failed creating registry value");
	}

	RegCloseKey(hk);

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
	}
	return hr;
}
