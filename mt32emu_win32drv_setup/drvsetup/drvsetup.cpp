/* Copyright (C) 2011-2022 Sergey V. Mikayev
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

const char MT32EMU_DRIVER_NAME[] = "mt32emu.dll";
const char MT32EMU_DRIVER_NAME_X64[] = "mt32emu_x64.dll";
const char MIDI_REGISTRY_ENTRY_TEMPLATE[] = "midi?";
const int MIDI_REGISTRY_ENTRY_TEMPLATE_VAR_INDEX = sizeof(MIDI_REGISTRY_ENTRY_TEMPLATE) - 2;
const char WDM_DRIVER_NAME[] = "wdmaud.drv";
const char SYSTEM_DIR_NAME[] = "SYSTEM32";
const char SYSTEM_WOW64_DIR_NAME[] = "SysWOW64";
const char SYSTEM_X64_DIR_NAME[] = "Sysnative";
const char SYSTEM_ROOT_ENV_NAME[] = "SYSTEMROOT";
const char INSTALL_COMMAND[] = "install";
const char UNINSTALL_COMMAND[] = "uninstall";
const char REPAIR_COMMAND[] = "repair";
const char DRIVERS_REGISTRY_KEY[] = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32";
const char PATH_SEPARATOR[] = "\\";

const char DEVICE_NAME_MEDIA[] = "MEDIA";
const char DEVICE_DESCRIPTION[] = "MT-32 Synth Emulator";
const char DRIVER_PROVIDER_NAME[] = "muntemu.org";
const char DEVICE_HARDWARE_IDS[] = "ROOT\\mt32emu\0";

const char DRIVER_CLASS_PROP_DRIVER_DESC[] = "DriverDesc";
const char DRIVER_CLASS_PROP_PROVIDER_NAME[] = "ProviderName";
const char DRIVER_CLASS_SUBKEY_DRIVERS[] = "Drivers";
const char DRIVER_CLASS_PROP_SUBCLASSES[] = "SubClasses";
const char DRIVER_CLASS_SUBCLASSES[] = "MIDI";
const char DRIVER_SUBCLASS_SUBKEYS[] = "MIDI\\mt32emu.dll";
const char DRIVER_SUBCLASS_PROP_DRIVER[] = "Driver";
const char DRIVER_SUBCLASS_PROP_DESCRIPTION[] = "Description";
const char DRIVER_SUBCLASS_PROP_ALIAS[] = "Alias";

const char SUCCESSFULLY_INSTALLED_MSG[] = "MT32Emu MIDI Driver successfully installed";
const char SUCCESSFULLY_UPDATED_MSG[] = "MT32Emu MIDI Driver successfully updated";
const char SUCCESSFULLY_UNINSTALLED_MSG[] = "MT32Emu MIDI Driver successfully uninstalled";
const char USAGE_MSG[] = "Usage:\n\n"
	"  drvsetup install - install or update driver\n"
	"  drvsetup uninstall - uninstall driver\n"
	"  drvsetup repair - repair registry entry for installed driver";

const char CANNOT_OPEN_REGISTRY_ERR[] = "Cannot open registry key";
const char CANNOT_OPEN_REGISTRY_32_ERR[] = "Cannot open 32-bit registry key";
const char CANNOT_INSTALL_NO_PORTS_ERR[] = "Cannot install MT32Emu MIDI driver:\n There is no MIDI ports available";
const char CANNOT_REGISTER_ERR[] = "Cannot register driver";
const char CANNOT_REGISTER_32_ERR[] = "Cannot register 32-bit driver";
const char CANNOT_REGISTER_CLASS_ERR[] = "Cannot register driver class";
const char CANNOT_REGISTER_DEVICE_ERR[] = "Cannot register device";
const char CANNOT_REMOVE_DEVICE_ERR[] = "Cannot remove device";
const char CANNOT_UNINSTALL_ERR[] = "Cannot uninstall MT32Emu MIDI driver";
const char CANNOT_UNINSTALL_32_ERR[] = "Cannot uninstall 32-bit MT32Emu MIDI driver";
const char CANNOT_UNINSTALL_NOT_FOUND_ERR[] = "Cannot uninstall MT32Emu MIDI driver:\n There is no driver registry entry found";
const char CANNOT_INSTALL_PATH_TOO_LONG_ERR[] = "MT32Emu MIDI Driver cannot be installed:\n Installation path is too long";
const char CANNOT_INSTALL_FILE_COPY_ERR[] = "MT32Emu MIDI Driver failed to install:\n File copying error";
const char CANNOT_INSTALL_32_FILE_COPY_ERR[] = "32-bit MT32Emu MIDI Driver failed to install:\n File copying error";

const char INFORMATION_TITLE[] = "Information";
const char ERROR_TITLE[] = "Error";
const char REGISTRY_ERROR_TITLE[] = "Registry error";
const char FILE_ERROR_TITLE[] = "File error";

enum ProcessReturnCode {
	ProcessReturnCode_OK = 0,
	ProcessReturnCode_ERR_UNRECOGNISED_OPERATION_MODE = 1,
	ProcessReturnCode_ERR_PATH_TOO_LONG = 2,
	ProcessReturnCode_ERR_REGISTERING_DRIVER = 3,
	ProcessReturnCode_ERR_COPYING_FILE = 4,
	ProcessReturnCode_ERR_REGISTERING_DRIVER_CLASS = 5,
	ProcessReturnCode_ERR_REGISTERING_DEVICE = 6,
	ProcessReturnCode_ERR_REMOVING_DEVICE = 7
};

enum OperationMode {
	OperationMode_UNKNOWN,
	OperationMode_INSTALL,
	OperationMode_UNINSTALL,
	OperationMode_REPAIR
};

enum RegisterDriverResult {
	RegisterDriverResult_OK,
	RegisterDriverResult_FAILED,
	RegisterDriverResult_ALREADY_EXISTS
};

enum FindDeviceResult {
	FindDeviceResult_FOUND,
	FindDeviceResult_NOT_FOUND,
	FindDeviceResult_FAILED
};

class MidiRegistryEntryName {
public:
	MidiRegistryEntryName() {
		strncpy(entryName, MIDI_REGISTRY_ENTRY_TEMPLATE, sizeof(MIDI_REGISTRY_ENTRY_TEMPLATE));
	}

	MidiRegistryEntryName *withIndex(int index) {
		entryName[MIDI_REGISTRY_ENTRY_TEMPLATE_VAR_INDEX] = 0 < index && index < 10 ? '0' + index : 0;
		return this;
	}

	const char *toCString() {
		return entryName;
	}

private:
	char entryName[sizeof(MIDI_REGISTRY_ENTRY_TEMPLATE)];
};

typedef WINBASEAPI BOOL(WINAPI *IsWow64Process)(_In_ HANDLE hProcess, _Out_ PBOOL wow64Process);

// Function IsWow64Process() is only available since _WIN32_WINNT 0x0501, so it is linked lazily.
static bool isWow64Process() {
	const IsWow64Process fnIsWow64Process = (IsWow64Process)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
	// If IsWow64Process is undefined, this is a 32-bit system.
	if (NULL == fnIsWow64Process) {
		return false;
	}
	BOOL wow64Process;
	// If this call fails, just assume a 32-bit system.
	if (!fnIsWow64Process(GetCurrentProcess(), &wow64Process)) {
		return false;
	}
	return wow64Process != FALSE;
}

// Ideally, there should be a free entry to use but WDM entries can also be used when they fill up all available entries.
// Although, the first entry shouldn't be modified. Besides, this is not 100% safe since the WDM entry may be removed
// by the system when the mapped WDM driver is deinstalled. But this is better than nothing in this case.
static bool findFreeMidiRegEntry(int &entryIx, HKEY hReg, MidiRegistryEntryName &entryName) {
	int freeEntryIx = -1;
	int wdmEntryIx = -1;
	char str[255];
	for (int i = 0; i < 10; i++) {
		DWORD len = 255;
		LSTATUS res = RegQueryValueExA(hReg, entryName.withIndex(i)->toCString(), NULL, NULL, (LPBYTE)str, &len);
		if (res != ERROR_SUCCESS) {
			if (res == ERROR_FILE_NOT_FOUND && freeEntryIx == -1) {
				freeEntryIx = i;
			}
			continue;
		}
		if (_stricmp(str, MT32EMU_DRIVER_NAME) == 0) {
			entryIx = i;
			return false;
		}
		if (freeEntryIx == -1) {
			if (strlen(str) == 0) {
				freeEntryIx = i;
				continue;
			}
			if (i > 0 && wdmEntryIx == -1 && _stricmp(str, WDM_DRIVER_NAME) == 0) {
				wdmEntryIx = i;
			}
		}
	}
	// Fall back to using a WDM entry if there is no free one.
	entryIx = freeEntryIx != -1 ? freeEntryIx : wdmEntryIx;
	return entryIx != -1;
}

static int findMt32emuRegEntry(HKEY hReg, MidiRegistryEntryName &entryName) {
	char str[255];
	for (int i = 0; i < 10; i++) {
		DWORD len = 255;
		LSTATUS res = RegQueryValueExA(hReg, entryName.withIndex(i)->toCString(), NULL, NULL, (LPBYTE)str, &len);
		if (res == ERROR_SUCCESS && _stricmp(str, MT32EMU_DRIVER_NAME) == 0) {
			return i;
		}
	}
	return -1;
}

static bool registerDriverInWow(const char *mt32emuEntryName) {
	HKEY hReg;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, DRIVERS_REGISTRY_KEY, 0L, KEY_ALL_ACCESS | KEY_WOW64_32KEY, &hReg)) {
		MessageBoxA(NULL, CANNOT_OPEN_REGISTRY_32_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	LSTATUS res = RegSetValueExA(hReg, mt32emuEntryName, NULL, REG_SZ, (LPBYTE)MT32EMU_DRIVER_NAME, sizeof(MT32EMU_DRIVER_NAME));
	RegCloseKey(hReg);
	if (res != ERROR_SUCCESS) {
		MessageBoxA(NULL, CANNOT_REGISTER_32_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	return true;
}

static RegisterDriverResult registerDriver(const bool wow64Process, int &entryIx) {
	HKEY hReg;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, DRIVERS_REGISTRY_KEY, 0L, (wow64Process ? (KEY_ALL_ACCESS | KEY_WOW64_64KEY) : KEY_ALL_ACCESS), &hReg)) {
		MessageBoxA(NULL, CANNOT_OPEN_REGISTRY_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return RegisterDriverResult_FAILED;
	}
	MidiRegistryEntryName entryName;
	if (!findFreeMidiRegEntry(entryIx, hReg, entryName)) {
		RegCloseKey(hReg);
		if (entryIx == -1) {
			MessageBoxA(NULL, CANNOT_INSTALL_NO_PORTS_ERR, ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
			return RegisterDriverResult_FAILED;
		}
		const char *mt32emuEntryName = entryName.withIndex(entryIx)->toCString();
		if (wow64Process && !registerDriverInWow(mt32emuEntryName)) {
			return RegisterDriverResult_FAILED;
		}
		return RegisterDriverResult_ALREADY_EXISTS;
	}
	const char *freeEntryName = entryName.withIndex(entryIx)->toCString();
	LSTATUS res = RegSetValueExA(hReg, freeEntryName, NULL, REG_SZ, (LPBYTE)MT32EMU_DRIVER_NAME, sizeof(MT32EMU_DRIVER_NAME));
	RegCloseKey(hReg);
	if (res != ERROR_SUCCESS) {
		MessageBoxA(NULL, CANNOT_REGISTER_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return RegisterDriverResult_FAILED;
	}
	if (wow64Process && !registerDriverInWow(freeEntryName)) {
		return RegisterDriverResult_FAILED;
	}
	return RegisterDriverResult_OK;
}

static void unregisterDriverInWow(const char *mt32emuEntryName) {
	HKEY hReg;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, DRIVERS_REGISTRY_KEY, 0L, KEY_ALL_ACCESS | KEY_WOW64_32KEY, &hReg)) {
		MessageBoxA(NULL, CANNOT_OPEN_REGISTRY_32_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	char str[255];
	DWORD len = 255;
	LSTATUS res = RegQueryValueExA(hReg, mt32emuEntryName, NULL, NULL, (LPBYTE)str, &len);
	if (res != ERROR_SUCCESS || _stricmp(str, MT32EMU_DRIVER_NAME)) {
		MessageBoxA(NULL, CANNOT_UNINSTALL_32_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		RegCloseKey(hReg);
		return;
	}
	res = RegDeleteValueA(hReg, mt32emuEntryName);
	RegCloseKey(hReg);
	if (res != ERROR_SUCCESS) {
		MessageBoxA(NULL, CANNOT_UNINSTALL_32_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
	}
}

static bool unregisterDriver(const bool wow64Process) {
	HKEY hReg;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, DRIVERS_REGISTRY_KEY, 0L, (wow64Process ? (KEY_ALL_ACCESS | KEY_WOW64_64KEY) : KEY_ALL_ACCESS), &hReg)) {
		MessageBoxA(NULL, CANNOT_OPEN_REGISTRY_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	MidiRegistryEntryName entryName;
	const int entryIx = findMt32emuRegEntry(hReg, entryName);
	if (entryIx == -1) {
		MessageBoxA(NULL, CANNOT_UNINSTALL_NOT_FOUND_ERR, ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		RegCloseKey(hReg);
		return false;
	}
	const char *mt32emuEntryName = entryName.withIndex(entryIx)->toCString();
	LSTATUS res = RegDeleteValueA(hReg, mt32emuEntryName);
	RegCloseKey(hReg);
	if (wow64Process) {
		unregisterDriverInWow(mt32emuEntryName);
	}
	if (res != ERROR_SUCCESS) {
		MessageBoxA(NULL, CANNOT_UNINSTALL_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	return true;
}

static void constructFullSystemDirName(char *pathName, const char *systemDirName) {
	char sysRoot[MAX_PATH + 1];
	GetEnvironmentVariableA(SYSTEM_ROOT_ENV_NAME, sysRoot, MAX_PATH);
	strncpy(pathName, sysRoot, MAX_PATH);
	strncat(pathName, PATH_SEPARATOR, MAX_PATH - strlen(pathName));
	strncat(pathName, systemDirName, MAX_PATH - strlen(pathName));
	strncat(pathName, PATH_SEPARATOR, MAX_PATH - strlen(pathName));
}

static void constructDriverPathName(char *pathName, const char *systemDirName) {
	constructFullSystemDirName(pathName, systemDirName);
	strncat(pathName, MT32EMU_DRIVER_NAME, MAX_PATH - strlen(pathName));
}

static void deleteFileReliably(char *pathName, const char *systemDirName) {
	if (DeleteFileA(pathName)) {
		return;
	}
	// File doesn't exist, nothing to do
	if (ERROR_FILE_NOT_FOUND == GetLastError()) {
		return;
	}
	// File can't be deleted, rename it and register pending deletion
	char tmpFilePrefix[sizeof(MT32EMU_DRIVER_NAME) + 1];
	strncpy(tmpFilePrefix, MT32EMU_DRIVER_NAME, sizeof(MT32EMU_DRIVER_NAME));
	strncat(tmpFilePrefix, ".", 1);
	char tmpDirName[MAX_PATH + 1];
	constructFullSystemDirName(tmpDirName, systemDirName);
	char tmpPathName[MAX_PATH + 1];
	GetTempFileNameA(tmpDirName, tmpFilePrefix, 0, tmpPathName);
	DeleteFileA(tmpPathName);
	MoveFileA(pathName, tmpPathName);
	MoveFileExA(tmpPathName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
}

static void deleteDriverFile(char *driverPathName, const char *systemDirName) {
	constructDriverPathName(driverPathName, systemDirName);
	deleteFileReliably(driverPathName, systemDirName);
}

static bool installDriverFile(const char *setupPath, const char *driverFileName, const char *systemDirName) {
	char driverPathName[MAX_PATH + 1];
	deleteDriverFile(driverPathName, systemDirName);
	char setupPathName[MAX_PATH + 1];
	strncpy(setupPathName, setupPath, MAX_PATH);
	strncat(setupPathName, PATH_SEPARATOR, MAX_PATH - strlen(setupPathName));
	strncat(setupPathName, driverFileName, MAX_PATH - strlen(setupPathName));
	return CopyFileA(setupPathName, driverPathName, FALSE) != FALSE;
}

static bool registerDriverClass(HKEY devRegKey, int legacyMidiEntryIx) {
	if (ERROR_SUCCESS != RegSetValueExA(devRegKey, DRIVER_CLASS_PROP_DRIVER_DESC, NULL, REG_SZ, (LPBYTE)DEVICE_DESCRIPTION, sizeof(DEVICE_DESCRIPTION))) return false;
	if (ERROR_SUCCESS != RegSetValueExA(devRegKey, DRIVER_CLASS_PROP_PROVIDER_NAME, NULL, REG_SZ, (LPBYTE)DRIVER_PROVIDER_NAME, sizeof(DRIVER_PROVIDER_NAME))) return false;
	HKEY driversSubkey;
	if (ERROR_SUCCESS != RegCreateKeyExA(devRegKey, DRIVER_CLASS_SUBKEY_DRIVERS, NULL, NULL, 0, KEY_ALL_ACCESS, NULL, &driversSubkey, NULL)) return false;
	if (ERROR_SUCCESS != RegSetValueExA(driversSubkey, DRIVER_CLASS_PROP_SUBCLASSES, NULL, REG_SZ, (LPBYTE)DRIVER_CLASS_SUBCLASSES, sizeof(DRIVER_CLASS_SUBCLASSES))) {
		RegCloseKey(driversSubkey);
		return false;
	}
	HKEY driverSubkey;
	if (ERROR_SUCCESS != RegCreateKeyExA(driversSubkey, DRIVER_SUBCLASS_SUBKEYS, NULL, NULL, 0, KEY_ALL_ACCESS, NULL, &driverSubkey, NULL)) {
		RegCloseKey(driversSubkey);
		return false;
	}
	RegCloseKey(driversSubkey);
	if (ERROR_SUCCESS != RegSetValueExA(driverSubkey, DRIVER_SUBCLASS_PROP_DRIVER, NULL, REG_SZ, (LPBYTE)MT32EMU_DRIVER_NAME, sizeof(MT32EMU_DRIVER_NAME))) {
		RegCloseKey(driverSubkey);
		return false;
	}
	if (ERROR_SUCCESS != RegSetValueExA(driverSubkey, DRIVER_SUBCLASS_PROP_DESCRIPTION, NULL, REG_SZ, (LPBYTE)DEVICE_DESCRIPTION, sizeof(DEVICE_DESCRIPTION))) {
		RegCloseKey(driverSubkey);
		return false;
	}
	MidiRegistryEntryName entryName;
	const char *mt32emuEntryName = entryName.withIndex(legacyMidiEntryIx)->toCString();
	if (ERROR_SUCCESS != RegSetValueExA(driverSubkey, DRIVER_SUBCLASS_PROP_ALIAS, NULL, REG_SZ, (LPBYTE)mt32emuEntryName, sizeof(MIDI_REGISTRY_ENTRY_TEMPLATE))) {
		RegCloseKey(driverSubkey);
		return false;
	}
	return true;
}

static FindDeviceResult findMt32emuDevice(HDEVINFO &hDevInfo, SP_DEVINFO_DATA &deviceInfoData) {
	char property[1024];
	for (int deviceIx = 0; SetupDiEnumDeviceInfo(hDevInfo, deviceIx, &deviceInfoData); ++deviceIx) {
		if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_HARDWAREID, NULL, (BYTE *)property, sizeof(property), NULL)) {
			if (!_strnicmp(property, DEVICE_HARDWARE_IDS, sizeof(DEVICE_HARDWARE_IDS) + 1)) return FindDeviceResult_FOUND;
			continue;
		}
		if (ERROR_INVALID_DATA != GetLastError()) continue;
		if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_DEVICEDESC, NULL, (BYTE *)property, sizeof(property), NULL)) {
			if (_strnicmp(property, DEVICE_DESCRIPTION, sizeof(DEVICE_DESCRIPTION))) continue;
			SetupDiSetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_HARDWAREID, (BYTE *)DEVICE_HARDWARE_IDS, sizeof(DEVICE_HARDWARE_IDS));
			return FindDeviceResult_FOUND;
		}
	}
	return ERROR_NO_MORE_ITEMS == GetLastError() ? FindDeviceResult_NOT_FOUND : FindDeviceResult_FAILED;
}

static ProcessReturnCode registerDevice(HDEVINFO &hDevInfo, SP_DEVINFO_DATA &deviceInfoData, bool wow64Process) {
	hDevInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_MEDIA, NULL, NULL, 0);
	if (INVALID_HANDLE_VALUE == hDevInfo) {
		MessageBoxA(NULL, CANNOT_REGISTER_DEVICE_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_REGISTERING_DEVICE;
	}

	FindDeviceResult findDeviceResult = findMt32emuDevice(hDevInfo, deviceInfoData);
	if (FindDeviceResult_FAILED == findDeviceResult) return ProcessReturnCode_ERR_REGISTERING_DEVICE;
	if (FindDeviceResult_FOUND == findDeviceResult) return ProcessReturnCode_OK;

	if (!SetupDiCreateDeviceInfoA(hDevInfo, DEVICE_NAME_MEDIA, &GUID_DEVCLASS_MEDIA, DEVICE_DESCRIPTION, NULL, DICD_GENERATE_ID, &deviceInfoData)) {
		SetupDiDestroyDeviceInfoList(hDevInfo);
		MessageBoxA(NULL, CANNOT_REGISTER_DEVICE_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_REGISTERING_DEVICE;
	}
	if (!SetupDiSetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_HARDWAREID, (BYTE *)DEVICE_HARDWARE_IDS, sizeof(DEVICE_HARDWARE_IDS))) {
		SetupDiDestroyDeviceInfoList(hDevInfo);
		MessageBoxA(NULL, CANNOT_REGISTER_DEVICE_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_REGISTERING_DEVICE;
	}

	// The proper way to register device in PnP manager is to call SetupDiCallClassInstaller but that doesn't work in WOW.
	BOOL deviceRegistrationResult;
	if (wow64Process) {
		deviceRegistrationResult = SetupDiRegisterDeviceInfo(hDevInfo, &deviceInfoData, 0, NULL, NULL, NULL);
	} else {
		deviceRegistrationResult = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, hDevInfo, &deviceInfoData);
	}
	if (!deviceRegistrationResult) {
		SetupDiDestroyDeviceInfoList(hDevInfo);
		MessageBoxA(NULL, CANNOT_REGISTER_DEVICE_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_REGISTERING_DEVICE;
	}
	return ProcessReturnCode_OK;
}

static ProcessReturnCode registerDeviceAndDriverClass(bool wow64Process, int legacyMidiEntryIx) {
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	ProcessReturnCode returnCode = registerDevice(hDevInfo, deviceInfoData, wow64Process);
	if (ProcessReturnCode_OK != returnCode) return returnCode;

	DWORD configClass = CONFIGFLAG_MANUAL_INSTALL | CONFIGFLAG_NEEDS_FORCED_CONFIG;
	if (!SetupDiSetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_CONFIGFLAGS, (BYTE *)&configClass, sizeof(configClass))) {
		SetupDiDestroyDeviceInfoList(hDevInfo);
		MessageBoxA(NULL, CANNOT_REGISTER_DEVICE_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_REGISTERING_DEVICE;
	}
	if (!SetupDiSetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_MFG, (BYTE *)DRIVER_PROVIDER_NAME, sizeof(DRIVER_PROVIDER_NAME))) {
		SetupDiDestroyDeviceInfoList(hDevInfo);
		MessageBoxA(NULL, CANNOT_REGISTER_DEVICE_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_REGISTERING_DEVICE;
	}

	HKEY devRegKey = SetupDiCreateDevRegKeyA(hDevInfo, &deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);
	SetupDiDestroyDeviceInfoList(hDevInfo);
	if (INVALID_HANDLE_VALUE == devRegKey) {
		MessageBoxA(NULL, CANNOT_REGISTER_CLASS_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_REGISTERING_DRIVER_CLASS;
	}
	if (!registerDriverClass(devRegKey, legacyMidiEntryIx)) {
		RegCloseKey(devRegKey);
		MessageBoxA(NULL, CANNOT_REGISTER_CLASS_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_REGISTERING_DRIVER_CLASS;
	}
	RegCloseKey(devRegKey);

	return ProcessReturnCode_OK;
}

static ProcessReturnCode removeDevice(bool wow64Process) {
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	hDevInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_MEDIA, NULL, NULL, 0);
	if (INVALID_HANDLE_VALUE == hDevInfo) {
		MessageBoxA(NULL, CANNOT_REMOVE_DEVICE_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_REMOVING_DEVICE;
	}

	FindDeviceResult findDeviceResult = findMt32emuDevice(hDevInfo, deviceInfoData);
	if (FindDeviceResult_FAILED == findDeviceResult) {
		SetupDiDestroyDeviceInfoList(hDevInfo);
		MessageBoxA(NULL, CANNOT_REMOVE_DEVICE_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_REMOVING_DEVICE;
	}
	if (FindDeviceResult_NOT_FOUND == findDeviceResult) {
		SetupDiDestroyDeviceInfoList(hDevInfo);
		MessageBoxA(NULL, CANNOT_REMOVE_DEVICE_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_OK;
	}

	// The proper way to remove device in PnP manager is to call SetupDiCallClassInstaller but that doesn't work in WOW.
	BOOL deviceRemovalResult;
	if (wow64Process) {
		deviceRemovalResult = SetupDiRemoveDevice(hDevInfo, &deviceInfoData);
	} else {
		deviceRemovalResult = SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, &deviceInfoData);
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);
	if (!deviceRemovalResult) {
		MessageBoxA(NULL, CANNOT_REMOVE_DEVICE_ERR, REGISTRY_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
	}
	return ProcessReturnCode_OK;
}

int main(int argc, char *argv[]) {
	bool wow64Process = isWow64Process();

	OperationMode mode = OperationMode_UNKNOWN;
	if (argc == 2) {
		if (_stricmp(INSTALL_COMMAND, argv[1]) == 0) {
			mode = OperationMode_INSTALL;
		} else if (_stricmp(UNINSTALL_COMMAND, argv[1]) == 0) {
			mode = OperationMode_UNINSTALL;
		} else if (_stricmp(REPAIR_COMMAND, argv[1]) == 0) {
			mode = OperationMode_REPAIR;
		}
	}

	switch (mode) {
		case OperationMode_INSTALL: {
			const char *pathDelimPosition = strrchr(argv[0], PATH_SEPARATOR[0]);
			int setupPathLen = int(pathDelimPosition - argv[0]);
			if (pathDelimPosition != NULL && setupPathLen > MAX_PATH - sizeof(MT32EMU_DRIVER_NAME) - 2) {
				MessageBoxA(NULL, CANNOT_INSTALL_PATH_TOO_LONG_ERR, ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
				return ProcessReturnCode_ERR_PATH_TOO_LONG;
			}
			int legacyMidiEntryIx;
			const RegisterDriverResult registerDriverResult = registerDriver(wow64Process, legacyMidiEntryIx);
			if (registerDriverResult == RegisterDriverResult_FAILED) return ProcessReturnCode_ERR_REGISTERING_DRIVER;
			ProcessReturnCode returnCode = registerDeviceAndDriverClass(wow64Process, legacyMidiEntryIx);
			if (ProcessReturnCode_OK != returnCode) return returnCode;
			char setupPath[MAX_PATH + 1];
			if (pathDelimPosition == NULL) {
				GetCurrentDirectoryA(sizeof(setupPath), setupPath);
			} else {
				strncpy(setupPath, argv[0], setupPathLen);
				setupPath[setupPathLen] = 0;
			}
			if (wow64Process) {
				if (!installDriverFile(setupPath, MT32EMU_DRIVER_NAME_X64, SYSTEM_X64_DIR_NAME)) {
					MessageBoxA(NULL, CANNOT_INSTALL_FILE_COPY_ERR, FILE_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
					return ProcessReturnCode_ERR_COPYING_FILE;
				}
				if (!installDriverFile(setupPath, MT32EMU_DRIVER_NAME, SYSTEM_WOW64_DIR_NAME)) {
					MessageBoxA(NULL, CANNOT_INSTALL_32_FILE_COPY_ERR, FILE_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
					return ProcessReturnCode_ERR_COPYING_FILE;
				}
			} else if (!installDriverFile(setupPath, MT32EMU_DRIVER_NAME, SYSTEM_DIR_NAME)) {
				MessageBoxA(NULL, CANNOT_INSTALL_FILE_COPY_ERR, FILE_ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
				return ProcessReturnCode_ERR_COPYING_FILE;
			}
			const char *successMessage = registerDriverResult == RegisterDriverResult_OK ? SUCCESSFULLY_INSTALLED_MSG : SUCCESSFULLY_UPDATED_MSG;
			MessageBoxA(NULL, successMessage, INFORMATION_TITLE, MB_OK | MB_ICONINFORMATION);
			return ProcessReturnCode_OK;
		}

		case OperationMode_REPAIR: {
			int legacyMidiEntryIx;
			const RegisterDriverResult registerDriverResult = registerDriver(wow64Process, legacyMidiEntryIx);
			if (registerDriverResult == RegisterDriverResult_FAILED) return ProcessReturnCode_ERR_REGISTERING_DRIVER;
			return registerDeviceAndDriverClass(wow64Process, legacyMidiEntryIx);
		}

		case OperationMode_UNINSTALL: {
			char pathName[MAX_PATH + 1];
			if (wow64Process) {
				deleteDriverFile(pathName, SYSTEM_X64_DIR_NAME);
				deleteDriverFile(pathName, SYSTEM_WOW64_DIR_NAME);
			} else {
				deleteDriverFile(pathName, SYSTEM_DIR_NAME);
			}
			if (!unregisterDriver(wow64Process)) return ProcessReturnCode_OK;
			ProcessReturnCode returnCode = removeDevice(wow64Process);
			if (ProcessReturnCode_OK != returnCode) return returnCode;
			MessageBoxA(NULL, SUCCESSFULLY_UNINSTALLED_MSG, INFORMATION_TITLE, MB_OK | MB_ICONINFORMATION);
			return ProcessReturnCode_OK;
		}

		default:
			MessageBoxA(NULL, USAGE_MSG, INFORMATION_TITLE, MB_OK | MB_ICONINFORMATION);
			return ProcessReturnCode_ERR_UNRECOGNISED_OPERATION_MODE;
	}
}
