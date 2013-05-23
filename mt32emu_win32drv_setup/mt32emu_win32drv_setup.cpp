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

const char MT32EMU_DRIVER_NAME[] = "mt32emu.dll";
const char WDM_DRIVER_NAME[] = "wdmaud.drv";
const char SYSTEM_DIR_NAME[] = "SYSTEM32";
const char SYSTEM_ROOT_ENV_NAME[] = "SYSTEMROOT";
const char INSTALL_COMMAND[] = "install";
const char UNINSTALL_COMMAND[] = "uninstall";

bool registerDriver() {
	HKEY hReg;
	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32", &hReg)) {
		MessageBoxA(NULL, "Cannot open registry key", "Registry error", MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
	char str[255];
	char drvName[] = "midi0";
	DWORD len, res;
	bool wdmEntryFound = false;
	int freeEntry = -1;
	for (int i = 0; i < 10; i++) {
		len = 255;
		if (i) {
			drvName[4] = '0' + i;
		} else {
			drvName[4] = 0;
		}
		res = RegQueryValueExA(hReg, drvName, NULL, NULL, (LPBYTE)str, &len);
		if (res != ERROR_SUCCESS) {
			if ((freeEntry == -1) && (res == ERROR_FILE_NOT_FOUND)) {
				freeEntry = i;
			}
			continue;
		}
		if (!_stricmp(str, MT32EMU_DRIVER_NAME)) {
			MessageBoxA(NULL, "MT32Emu MIDI Driver successfully updated.", "Information", MB_OK | MB_ICONINFORMATION);
			RegCloseKey(hReg);
			return true;
		}
		if (freeEntry != -1) continue;
		if (strlen(str) == 0) {
			freeEntry = i;
		} else if (!_stricmp(str, WDM_DRIVER_NAME)) {
			// Considering multiple WDM entries are just garbage, though one entry shouldn't be modified
			if (wdmEntryFound) {
				freeEntry = i;
			} else {
				wdmEntryFound = true;
			}
		}
	}
	if (freeEntry == -1) {
		MessageBoxA(NULL, "Cannot install MT32Emu MIDI driver. There is no MIDI ports available.", "Error", MB_OK | MB_ICONEXCLAMATION);
		RegCloseKey(hReg);
		return false;
	}
	if (freeEntry) {
		drvName[4] = '0' + freeEntry;
	} else {
		drvName[4] = 0;
	}
	res = RegSetValueExA(hReg, drvName, NULL, REG_SZ, (LPBYTE)MT32EMU_DRIVER_NAME, sizeof(MT32EMU_DRIVER_NAME));
	if (res != ERROR_SUCCESS) {
		MessageBoxA(NULL, "Cannot register driver", "Registry error", MB_OK | MB_ICONEXCLAMATION);
		RegCloseKey(hReg);
		return false;
	}
	MessageBoxA(NULL, "MT32Emu MIDI Driver successfully installed.", "Information", MB_OK | MB_ICONINFORMATION);
	RegCloseKey(hReg);
	return true;
}

void unregisterDriver() {
	HKEY hReg;
	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32", &hReg)) {
		MessageBoxA(NULL, "Cannot open registry key", "Registry error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	char str[255];
	char drvName[] = "MIDI0";
	DWORD len, res;
	for (int i = 0; i < 10; i++) {
		len = 255;
		if (i) {
			drvName[4] = '0' + i;
		} else {
			drvName[4] = 0;
		}
		res = RegQueryValueExA(hReg, drvName, NULL, NULL, (LPBYTE)str, &len);
		if (res != ERROR_SUCCESS) {
			continue;
		}
		if (!_stricmp(str, MT32EMU_DRIVER_NAME)) {
			res = RegDeleteValueA(hReg, drvName);
			if (res != ERROR_SUCCESS) {
				MessageBoxA(NULL, "Cannot uninstall MT32Emu MIDI driver", "Registry error", MB_OK | MB_ICONEXCLAMATION);
				RegCloseKey(hReg);
				return;
			}
			MessageBoxA(NULL, "MT32Emu MIDI Driver successfully uninstalled.", "Information", MB_OK | MB_ICONINFORMATION);
			RegCloseKey(hReg);
			return;
		}
	}
	MessageBoxA(NULL, "Cannot uninstall MT32Emu MIDI driver. There is no driver found.", "Error", MB_OK | MB_ICONEXCLAMATION);
	RegCloseKey(hReg);
}

void constructSystemDirName(char *pathName) {
	char sysRoot[MAX_PATH + 1];
	GetEnvironmentVariableA(SYSTEM_ROOT_ENV_NAME, sysRoot, MAX_PATH);
	strncpy(pathName, sysRoot, MAX_PATH);
	strncat(pathName, "/", MAX_PATH - strlen(pathName));
	strncat(pathName, SYSTEM_DIR_NAME, MAX_PATH - strlen(pathName));
	strncat(pathName, "/", MAX_PATH - strlen(pathName));
}

void constructDriverPathName(char *pathName) {
	constructSystemDirName(pathName);
	strncat(pathName, MT32EMU_DRIVER_NAME, MAX_PATH - strlen(pathName));
}

void deleteFileReliably(char *pathName) {
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
	constructSystemDirName(tmpDirName);
	char tmpPathName[MAX_PATH + 1];
	GetTempFileNameA(tmpDirName, tmpFilePrefix, 0, tmpPathName);
	DeleteFileA(tmpPathName);
	MoveFileA(pathName, tmpPathName);
	MoveFileExA(tmpPathName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
}

int main(int argc, char *argv[]) {
	if (argc != 2 || _stricmp(INSTALL_COMMAND, argv[1]) != 0 && _stricmp(UNINSTALL_COMMAND, argv[1]) != 0) {
		MessageBoxA(NULL, "Usage:\n  drvsetup install - install driver\n  drvsetup uninstall - uninstall driver\n", "Information", MB_OK | MB_ICONINFORMATION);
		return 1;
	}
	if (_stricmp(UNINSTALL_COMMAND, argv[1]) == 0) {
		unregisterDriver();
		char pathName[MAX_PATH + 1];
		constructDriverPathName(pathName);
		deleteFileReliably(pathName);
		return 0;
	}
	int setupPathLen = strrchr(argv[0], '\\') - argv[0];
	if (setupPathLen > MAX_PATH - sizeof(MT32EMU_DRIVER_NAME) - 2) {
		MessageBoxA(NULL, "MT32Emu MIDI Driver cannot be installed:\n Installation path is too long", "Error", MB_OK | MB_ICONEXCLAMATION);
		return 2;
	}
	if (!registerDriver()) {
		return 3;
	}
	char driverPathName[MAX_PATH + 1];
	constructDriverPathName(driverPathName);
	deleteFileReliably(driverPathName);
	char setupPathName[MAX_PATH + 1];
	strncpy(setupPathName, argv[0], setupPathLen);
	setupPathName[setupPathLen] = 0;
	strncat(setupPathName, "/", MAX_PATH - strlen(setupPathName));
	strncat(setupPathName, MT32EMU_DRIVER_NAME, MAX_PATH - strlen(setupPathName));
	if (!CopyFileA(setupPathName, driverPathName, FALSE)) {
		MessageBoxA(NULL, "MT32Emu MIDI Driver failed to install:\n File copying error", "File error", MB_OK | MB_ICONEXCLAMATION);
		return 4;
	}
	return 0;
}
