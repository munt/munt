/* Copyright (C) 2020-2022 Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
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

#include "pki_helper.h"

const char INSTALL_COMMAND[] = "install";
const char UNINSTALL_COMMAND[] = "uninstall";
const char SIGN_COMMAND[] = "sign";
const char SIGN_OPTION[] = "-sign";
const char PATH_SEPARATOR[] = "\\";

const char DEVICE_NAME_MEDIA[] = "MEDIA";
const char DEVICE_DESCRIPTION[] = "MT-32 Synth Emulator";
const char DEVICE_HARDWARE_IDS[] = "ROOT\\mt32emu\0";
const char DRIVER_INF_FILE_NAME[] = "mt32emu.inf";
const char DRIVER_CAT_FILE_NAME[] = "mt32emu.cat";
const char CERT_SUBJECT[] = "CN=Not digitally signed";

const char SUCCESSFULLY_INSTALLED_MSG[] = "MT32Emu MIDI Driver successfully installed";
const char SUCCESSFULLY_UPDATED_MSG[] = "MT32Emu MIDI Driver successfully updated";
const char SUCCESSFULLY_UNINSTALLED_MSG[] = "MT32Emu MIDI Driver successfully uninstalled";
const char SUCCESSFULLY_SIGNED_MSG[] = "MT32Emu MIDI Driver package successfully signed";
const char USAGE_MSG[] = "Usage:\n\n"
	"  infinstall install [-sign] - install or update driver,\n"
	"    if -sign option is specified, sign driver on-the-fly\n"
	"  infinstall uninstall - uninstall driver\n"
	"  infinstall sign - create self-signed certificate and sign driver";

const char CANNOT_REGISTER_DEVICE_ERR_FMT[] = "Cannot register device\n Error while %s: %0x";
const char CANNOT_INSTALL_DRIVER_ERR_FMT[] = "Cannot install MT32Emu MIDI Driver\n Error: %0x";
const char CANNOT_INSTALL_PATH_TOO_LONG_ERR[] = "MT32Emu MIDI Driver cannot be installed:\n Installation path is too long";
const char CANNOT_REMOVE_DEVICE_ERR_FMT[] = "Cannot uninstalled device\n Error while %s: %0x";
const char CANNOT_UNINSTALL_NOT_FOUND[] = "MT32Emu MIDI Driver cannot be uninstalled:\n Compatible devices not found";
const char CANNOT_SIGN_DRIVER_ERR[] = "Failure while signing MT32Emu MIDI Driver";

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
	ProcessReturnCode_ERR_UNRECOGNISED_OPTION = 2,
	ProcessReturnCode_ERR_PATH_TOO_LONG = 3,
	ProcessReturnCode_ERR_REGISTERING_DEVICE = 4,
	ProcessReturnCode_ERR_INSTALLING_DRIVER = 5,
	ProcessReturnCode_ERR_UNINSTALLING_DRIVER = 6,
	ProcessReturnCode_ERR_SIGNING_DRIVER = 7
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

static bool createDriverFilePath(char *filePath, const size_t filePathSize, const char *appLaunchPath, const char *fileName) {
	const char *pathDelimPosition = strrchr(appLaunchPath, PATH_SEPARATOR[0]);
	const size_t fileNameSize = strlen(fileName);
	if (pathDelimPosition == NULL) {
		DWORD currentDirSize = GetCurrentDirectoryA((DWORD)filePathSize, filePath);
		if (currentDirSize == 0 || filePathSize < currentDirSize) return false;
		if (filePathSize < currentDirSize + fileNameSize + /* path separator + NULL */ 2) return false;
	} else {
		int setupPathLen = int(pathDelimPosition - appLaunchPath);
		if (filePathSize < setupPathLen + fileNameSize + /* path separator + NULL */ 2) return false;
		strncpy(filePath, appLaunchPath, setupPathLen);
		filePath[setupPathLen] = 0;
	}
	strncat(filePath, PATH_SEPARATOR, filePathSize - strlen(filePath));
	strncat(filePath, fileName, filePathSize - strlen(filePath));
	return true;
}

static bool createFullInfPath(char *fullInfPath, const size_t fullInfPathSize, const char *appLaunchPath) {
	char infPath[MAX_PATH];
	createDriverFilePath(infPath, sizeof infPath, appLaunchPath, DRIVER_INF_FILE_NAME);
	DWORD actualPathSize = GetFullPathNameA(infPath, (DWORD)fullInfPathSize, fullInfPath, NULL);
	return actualPathSize != 0 && actualPathSize < fullInfPathSize;
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

static RegisterDeviceResult registerDevice(HDEVINFO &hDevInfo, SP_DEVINFO_DATA &deviceInfoData) {
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

static ProcessReturnCode infInstall(const char *appLaunchPath, bool signOption) {
	char fullInfPath[MAX_PATH];
	if (!createFullInfPath(fullInfPath, sizeof fullInfPath, appLaunchPath)) {
		MessageBoxA(NULL, CANNOT_INSTALL_PATH_TOO_LONG_ERR, ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_PATH_TOO_LONG;
	}

	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA deviceInfoData;
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	RegisterDeviceResult registerDeviceResult = registerDevice(hDevInfo, deviceInfoData);
	if (RegisterDeviceResult_GET_DEV_INFOS_ERROR <= registerDeviceResult) {
		DWORD errorCode = GetLastError();
		if (INVALID_HANDLE_VALUE != hDevInfo) SetupDiDestroyDeviceInfoList(hDevInfo);
		char message[1024];
		wsprintfA(message, CANNOT_REGISTER_DEVICE_ERR_FMT, registerDeviceStageName[registerDeviceResult - RegisterDeviceResult_GET_DEV_INFOS_ERROR], errorCode);
		MessageBoxA(NULL, message, ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_REGISTERING_DEVICE;
	}

	if (signOption) {
		char filePath[MAX_PATH];
		if (!createDriverFilePath(filePath, sizeof filePath, appLaunchPath, DRIVER_CAT_FILE_NAME)) {
			SetupDiDestroyDeviceInfoList(hDevInfo);
			MessageBoxA(NULL, CANNOT_INSTALL_PATH_TOO_LONG_ERR, ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
			return ProcessReturnCode_ERR_PATH_TOO_LONG;
		}
		if (!SelfSignFile(DRIVER_CAT_FILE_NAME, CERT_SUBJECT)) {
			SetupDiDestroyDeviceInfoList(hDevInfo);
			MessageBoxA(NULL, CANNOT_SIGN_DRIVER_ERR, ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
			return ProcessReturnCode_ERR_SIGNING_DRIVER;
		}
	}

	BOOL rebootRequired = FALSE;
	if (!UpdateDriverForPlugAndPlayDevicesA(NULL, DEVICE_HARDWARE_IDS, fullInfPath, 0, &rebootRequired)) {
		DWORD errorCode = GetLastError();

		// Cleanup the failed device in PnP manager.
		SetupDiCallClassInstaller(DIF_REMOVE, hDevInfo, &deviceInfoData);
		SetupDiDestroyDeviceInfoList(hDevInfo);

		// Delete certificate which we no longer need.
		if (signOption) {
			RemoveCertFromStore(CERT_SUBJECT, ROOT_CERT_STORE_NAME);
			RemoveCertFromStore(CERT_SUBJECT, TRUSTED_PUBLISHER_CERT_STORE_NAME);
		}

		char message[1024];
		wsprintfA(message, CANNOT_INSTALL_DRIVER_ERR_FMT, errorCode);
		MessageBoxA(NULL, message, ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_INSTALLING_DRIVER;
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);

	// Delete certificate which we no longer need.
	if (signOption) {
		RemoveCertFromStore(CERT_SUBJECT, ROOT_CERT_STORE_NAME);
		RemoveCertFromStore(CERT_SUBJECT, TRUSTED_PUBLISHER_CERT_STORE_NAME);
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

static ProcessReturnCode signDriverPackage() {
	if (!SelfSignFile(DRIVER_CAT_FILE_NAME, CERT_SUBJECT)) {
		MessageBoxA(NULL, CANNOT_SIGN_DRIVER_ERR, ERROR_TITLE, MB_OK | MB_ICONEXCLAMATION);
		return ProcessReturnCode_ERR_SIGNING_DRIVER;
	}
	MessageBoxA(NULL, SUCCESSFULLY_SIGNED_MSG, INFORMATION_TITLE, MB_OK | MB_ICONINFORMATION);
	return ProcessReturnCode_OK;
}

int main(int argc, char *argv[]) {
	if (argc != 2 && argc != 3) {
		MessageBoxA(NULL, USAGE_MSG, INFORMATION_TITLE, MB_OK | MB_ICONINFORMATION);
		return argc > 3 ? ProcessReturnCode_ERR_UNRECOGNISED_OPTION : ProcessReturnCode_ERR_UNRECOGNISED_OPERATION_MODE;
	}
	if (_stricmp(INSTALL_COMMAND, argv[1]) == 0) {
		bool signOption = false;
		if (argc == 3) {
			if (_stricmp(SIGN_OPTION, argv[2]) != 0) {
				MessageBoxA(NULL, USAGE_MSG, INFORMATION_TITLE, MB_OK | MB_ICONINFORMATION);
				return ProcessReturnCode_ERR_UNRECOGNISED_OPTION;
			}
			signOption = true;
		}
		return infInstall(argv[0], signOption);
	} else if (_stricmp(UNINSTALL_COMMAND, argv[1]) == 0) {
		return uninstallDevice();
	} else if (_stricmp(SIGN_COMMAND, argv[1]) == 0) {
		return signDriverPackage();
	}
	MessageBoxA(NULL, USAGE_MSG, INFORMATION_TITLE, MB_OK | MB_ICONINFORMATION);
	return ProcessReturnCode_ERR_UNRECOGNISED_OPERATION_MODE;
}
