/* Copyright (C) 2020 Sergey V. Mikayev
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

const char INSTALL_COMMAND[] = "install";
const char UNINSTALL_COMMAND[] = "uninstall";
const char PATH_SEPARATOR[] = "\\";

const char DEVICE_NAME_MEDIA[] = "MEDIA";
const char DEVICE_DESCRIPTION[] = "MT-32 Synth Emulator";
const char DEVICE_HARDWARE_IDS[] = "ROOT\\mt32emu\0";
const char DRIVER_INF_FILE_NAME[] = "mt32emu.inf";

const char SUCCESSFULLY_INSTALLED_MSG[] = "MT32Emu MIDI Driver successfully installed";
const char SUCCESSFULLY_UPDATED_MSG[] = "MT32Emu MIDI Driver successfully updated";
const char SUCCESSFULLY_UNINSTALLED_MSG[] = "MT32Emu MIDI Driver successfully uninstalled";
const char USAGE_MSG[] = "Usage:\n\n"
	"  infinstall install - install or update driver\n"
	"  infinstall uninstall - uninstall driver";

const char CANNOT_REGISTER_DEVICE_ERR_FMT[] = "Cannot register device\n Error while %s: %0x";
const char CANNOT_INSTALL_DRIVER_ERR_FMT[] = "Cannot install MT32Emu MIDI Driver\n Error: %0x";
const char CANNOT_INSTALL_PATH_TOO_LONG_ERR[] = "MT32Emu MIDI Driver cannot be installed:\n Installation path is too long";
const char CANNOT_REMOVE_DEVICE_ERR_FMT[] = "Cannot uninstalled device\n Error while %s: %0x";
const char CANNOT_UNINSTALL_NOT_FOUND[] = "MT32Emu MIDI Driver cannot be uninstalled:\n Compatible devices not found";

const char INFORMATION_TITLE[] = "Information";
const char ERROR_TITLE[] = "Error";

const char *registerDeviceStageName[] = {
	"getting class devices", "enumerating devices", "creating device info", "setting device property", "registering device in PnP manager"
};

const char *removeDeviceStageName[] = {
	"getting class devices", "enumerating devices", "removing device from PnP manager"
};

enum ProcessReturnCode {
	ProcessReturnCode_OK = 0,
	ProcessReturnCode_ERR_UNRECOGNISED_OPERATION_MODE = 1,
	ProcessReturnCode_ERR_PATH_TOO_LONG = 2,
	ProcessReturnCode_ERR_REGISTERING_DEVICE = 3,
	ProcessReturnCode_ERR_INSTALLING_DRIVER = 4,
	ProcessReturnCode_ERR_UNINSTALLING_DRIVER = 5
};

enum RegisterDeviceResult {
	RegisterDeviceResult_OK,
	RegisterDeviceResult_ALREADY_EXISTS,
	RegisterDeviceResult_GET_DEV_INFOS_ERROR,
	RegisterDeviceResult_FIND_DEVICE_ERROR,
	RegisterDeviceResult_CREATE_DEV_INFO_ERROR,
	RegisterDeviceResult_SET_DEVICE_PROPERTY_ERROR,
	RegisterDeviceResult_REGISTER_DEVICE_ERROR
};

enum RemoveDeviceResult {
	RemoveDeviceResult_OK,
	RemoveDeviceResult_NOT_FOUND,
	RemoveDeviceResult_GET_DEV_INFOS_ERROR,
	RemoveDeviceResult_FIND_DEVICE_ERROR,
	RemoveDeviceResult_REMOVE_DEVICE_ERROR
};

enum FindDeviceResult {
	FindDeviceResult_FOUND,
	FindDeviceResult_NOT_FOUND,
	FindDeviceResult_FAILED
};

static bool createFullInfPath(char *fullInfPath, const char *appLaunchPath) {
	const char *pathDelimPosition = strrchr(appLaunchPath, PATH_SEPARATOR[0]);
	int setupPathLen = int(pathDelimPosition - appLaunchPath);
	if (pathDelimPosition != NULL && setupPathLen > MAX_PATH - sizeof(DRIVER_INF_FILE_NAME) - 2) return false;
	char infPath[MAX_PATH + 1];
	if (pathDelimPosition == NULL) {
		if (GetCurrentDirectoryA(sizeof(infPath), infPath) > MAX_PATH - sizeof(DRIVER_INF_FILE_NAME) - 2) return false;
	} else {
		strncpy(infPath, appLaunchPath, setupPathLen);
		infPath[setupPathLen] = 0;
	}
	strncat(infPath, PATH_SEPARATOR, MAX_PATH - strlen(infPath));
	strncat(infPath, DRIVER_INF_FILE_NAME, MAX_PATH - strlen(infPath));
	return GetFullPathNameA(infPath, MAX_PATH, fullInfPath, NULL) < MAX_PATH;
}

static FindDeviceResult findMt32emuDevice(HDEVINFO &hDevInfo, SP_DEVINFO_DATA &deviceInfoData) {
	char hardwareId[1024];
	for (int deviceIx = 0; SetupDiEnumDeviceInfo(hDevInfo, deviceIx, &deviceInfoData); ++deviceIx) {
		if (!SetupDiGetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_HARDWAREID, NULL, (BYTE *)hardwareId, sizeof(hardwareId), NULL)) continue;
		if (!strncmp(hardwareId, DEVICE_HARDWARE_IDS, sizeof(DEVICE_HARDWARE_IDS) + 1)) return FindDeviceResult_FOUND;
	}
	return ERROR_NO_MORE_ITEMS == GetLastError() ? FindDeviceResult_NOT_FOUND : FindDeviceResult_FAILED;
}

static RegisterDeviceResult registerDevice(HDEVINFO &hDevInfo) {
	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	hDevInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_MEDIA, NULL, NULL, 0);
	if (INVALID_HANDLE_VALUE == hDevInfo) return RegisterDeviceResult_GET_DEV_INFOS_ERROR;
	FindDeviceResult findDeviceResult = findMt32emuDevice(hDevInfo, deviceInfoData);
	if (FindDeviceResult_FAILED == findDeviceResult) return RegisterDeviceResult_FIND_DEVICE_ERROR;
	if (FindDeviceResult_FOUND == findDeviceResult) return RegisterDeviceResult_ALREADY_EXISTS;
	if (!SetupDiCreateDeviceInfoA(hDevInfo, DEVICE_NAME_MEDIA, &GUID_DEVCLASS_MEDIA, DEVICE_DESCRIPTION, NULL, DICD_GENERATE_ID, &deviceInfoData)) return RegisterDeviceResult_CREATE_DEV_INFO_ERROR;
	if (!SetupDiSetDeviceRegistryPropertyA(hDevInfo, &deviceInfoData, SPDRP_HARDWAREID, (BYTE *)DEVICE_HARDWARE_IDS, sizeof(DEVICE_HARDWARE_IDS))) return RegisterDeviceResult_SET_DEVICE_PROPERTY_ERROR;
	if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE, hDevInfo, &deviceInfoData)) return RegisterDeviceResult_REGISTER_DEVICE_ERROR;
	return RegisterDeviceResult_OK;
}

static ProcessReturnCode infInstall(char *fullInfPath) {
	HDEVINFO hDevInfo;

	RegisterDeviceResult registerDeviceResult = registerDevice(hDevInfo);
	if (RegisterDeviceResult_GET_DEV_INFOS_ERROR <= registerDeviceResult) {
		DWORD errorCode = GetLastError();
		if (INVALID_HANDLE_VALUE != hDevInfo) SetupDiDestroyDeviceInfoList(hDevInfo);
		char message[1024];
		wsprintfA(message, CANNOT_REGISTER_DEVICE_ERR_FMT, registerDeviceStageName[registerDeviceResult - RegisterDeviceResult_GET_DEV_INFOS_ERROR], errorCode);
		MessageBoxA(NULL, message, ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_REGISTERING_DEVICE;
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);

	BOOL rebootRequired = FALSE;
	if (!UpdateDriverForPlugAndPlayDevicesA(NULL, DEVICE_HARDWARE_IDS, fullInfPath, 0, &rebootRequired)) {
		DWORD errorCode = GetLastError();
		char message[1024];
		wsprintfA(message, CANNOT_INSTALL_DRIVER_ERR_FMT, errorCode);
		MessageBoxA(NULL, message, ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_INSTALLING_DRIVER;
	}

	const char *message = RegisterDeviceResult_ALREADY_EXISTS == registerDeviceResult ? SUCCESSFULLY_UPDATED_MSG : SUCCESSFULLY_INSTALLED_MSG;
	MessageBoxA(NULL, message, INFORMATION_TITLE, MB_OK | MB_ICONINFORMATION);
	return ProcessReturnCode_OK;
}

static RemoveDeviceResult removeDevice(HDEVINFO &hDevInfo) {
	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	hDevInfo = SetupDiGetClassDevsA(&GUID_DEVCLASS_MEDIA, NULL, NULL, 0);
	if (INVALID_HANDLE_VALUE == hDevInfo) return RemoveDeviceResult_GET_DEV_INFOS_ERROR;
	FindDeviceResult findDeviceResult = findMt32emuDevice(hDevInfo, deviceInfoData);
	if (FindDeviceResult_FAILED == findDeviceResult) return RemoveDeviceResult_FIND_DEVICE_ERROR;
	if (FindDeviceResult_NOT_FOUND == findDeviceResult) return RemoveDeviceResult_NOT_FOUND;
	if (!SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, &deviceInfoData)) return RemoveDeviceResult_REMOVE_DEVICE_ERROR;
	return RemoveDeviceResult_OK;
}

static ProcessReturnCode uninstallDevice() {
	HDEVINFO hDevInfo;

	RemoveDeviceResult removeDeviceResult = removeDevice(hDevInfo);
	if (RemoveDeviceResult_GET_DEV_INFOS_ERROR <= removeDeviceResult) {
		DWORD errorCode = GetLastError();
		if (INVALID_HANDLE_VALUE != hDevInfo) SetupDiDestroyDeviceInfoList(hDevInfo);
		char message[1024];
		wsprintfA(message, CANNOT_REMOVE_DEVICE_ERR_FMT, removeDeviceStageName[removeDeviceResult - RemoveDeviceResult_GET_DEV_INFOS_ERROR], errorCode);
		MessageBoxA(NULL, message, ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_UNINSTALLING_DRIVER;
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);

	const char *message = RemoveDeviceResult_OK == removeDeviceResult ? SUCCESSFULLY_UNINSTALLED_MSG : CANNOT_UNINSTALL_NOT_FOUND;
	MessageBoxA(NULL, message, INFORMATION_TITLE, MB_OK | MB_ICONINFORMATION);
	return ProcessReturnCode_OK;
}

int main(int argc, char *argv[]) {
	if (argc == 2) {
		if (_stricmp(INSTALL_COMMAND, argv[1]) == 0) {
			char fullInfPath[MAX_PATH + 1];
			if (!createFullInfPath(fullInfPath, argv[0])) {
				MessageBoxA(NULL, CANNOT_INSTALL_PATH_TOO_LONG_ERR, ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
				return ProcessReturnCode_ERR_PATH_TOO_LONG;
			}
			return infInstall(fullInfPath);
		} else if (_stricmp(UNINSTALL_COMMAND, argv[1]) == 0) {
			return uninstallDevice();
		}
	}
	MessageBoxA(NULL, USAGE_MSG, INFORMATION_TITLE, MB_OK | MB_ICONINFORMATION);
	return ProcessReturnCode_ERR_UNRECOGNISED_OPERATION_MODE;
}
