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

char mt32emuDriverName[] = "mt32emu.dll";

void RegisterDriver() {
	HKEY hReg;
	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32", &hReg)) {
		MessageBoxA(NULL, "Cannot open registry key", "Registry error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	char str[255];
	char drvName[] = "MIDI0";
	DWORD len, res;
	int freeDriver = -1;
	int mt32emuDriver = -1;
	for (int i = 0; i < 10; i++) {
		len = 255;
		if (i) {
			drvName[4] = '0' + i;
		} else {
			drvName[4] = 0;
		}
		res = RegQueryValueExA(hReg, drvName, NULL, NULL, (LPBYTE)str, &len);
		if (res != ERROR_SUCCESS) {
			if ((freeDriver == -1) && (res == ERROR_FILE_NOT_FOUND)) {
				freeDriver = i;
			}
			continue;
		}
		if (!_stricmp(str, mt32emuDriverName)) {
			mt32emuDriver = i;
		}
	}
	if (mt32emuDriver != -1) {
		MessageBoxA(NULL, "Driver already installed.", "Error", MB_OK | MB_ICONEXCLAMATION);
		RegCloseKey(hReg);
		return;
	}
	if (freeDriver == -1) {
		MessageBoxA(NULL, "Cannot install driver. There is no MIDI ports available.", "Error", MB_OK | MB_ICONEXCLAMATION);
		RegCloseKey(hReg);
		return;
	}
	if (freeDriver) {
		drvName[4] = '0' + freeDriver;
	} else {
		drvName[4] = 0;
	}
	res = RegSetValueExA(hReg, drvName, NULL, REG_SZ, (LPBYTE)mt32emuDriverName, sizeof(mt32emuDriverName));
	if (res != ERROR_SUCCESS) {
		MessageBoxA(NULL, "Cannot register driver", "Registry error", MB_OK | MB_ICONEXCLAMATION);
		RegCloseKey(hReg);
		return;
	}
	MessageBoxA(NULL, "Driver successfully installed.", "Information", MB_OK | MB_ICONINFORMATION);
	RegCloseKey(hReg);
}

void UnregisterDriver() {
	HKEY hReg;
	if (RegOpenKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32", &hReg)) {
		MessageBoxA(NULL, "Cannot open registry key", "Registry error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	char str[255];
	char drvName[] = "MIDI0";
	DWORD len, res;
	int mt32emuDriver = -1;
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
		if (!_stricmp(str, mt32emuDriverName)) {
			mt32emuDriver = i;
			res = RegDeleteValueA(hReg, drvName);
			if (res != ERROR_SUCCESS) {
				MessageBoxA(NULL, "Cannot uninstall driver", "Registry error", MB_OK | MB_ICONEXCLAMATION);
				RegCloseKey(hReg);
				return;
			}
		}
	}
	if (mt32emuDriver == -1) {
		MessageBoxA(NULL, "Cannot uninstall driver. There is no driver found.", "Error", MB_OK | MB_ICONEXCLAMATION);
		RegCloseKey(hReg);
		return;
	}
	MessageBoxA(NULL, "Driver successfully uninstalled.", "Information", MB_OK | MB_ICONINFORMATION);
	RegCloseKey(hReg);
}

int main() {
	char sysRoot[MAX_PATH];
	char pathName[MAX_PATH];
	char oldName[MAX_PATH];
	GetEnvironmentVariableA("SYSTEMROOT", sysRoot, MAX_PATH);
	strncpy(pathName, sysRoot, MAX_PATH);
	strncat(pathName, "/mt32emu.ini", MAX_PATH);
	CopyFileA("mt32emu.ini", pathName, TRUE);
	strncpy(pathName, sysRoot, MAX_PATH);
	strncat(pathName, "/SYSTEM32/mt32emu.dll", MAX_PATH);
	strncpy(oldName, sysRoot, MAX_PATH);
	strncat(oldName, "/SYSTEM32/mt32emu.old", MAX_PATH);
	DeleteFileA(oldName);
	MoveFileA(pathName, oldName);
	CopyFileA("mt32emu.dll", pathName, TRUE);
	RegisterDriver();
	return 0;
}
