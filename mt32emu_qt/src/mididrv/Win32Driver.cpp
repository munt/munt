/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
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

#include <QtGlobal>

#include <windows.h>

#include "Win32Driver.h"

#define CALLBACK_FUNCTION NULL

static qint64 getCurrentNanos() {
	return (qint64)clock() * NANOS_PER_SECOND / CLOCKS_PER_SEC;
}

LRESULT CALLBACK Win32MidiDriver::MidiInProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	// FIXME: Get synthManager from somewhere...
	switch (uMsg) {
	case WM_APP:
		synthManager->pushMIDIShortMessage(lParam, getCurrentNanos());
		return 1;
	case WM_COPYDATA:
		COPYDATASTRUCT *cds;
		cds = (COPYDATASTRUCT *)lParam;
		synthManager->pushMIDISysex((Bit8u *)cds->lpData, cds->cbData);
		return 1;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

void Win32MidiDriver::MessageLoop(SynthManager *synthManager) {
	HINSTANCE hInstance = GetModuleHandle(NULL);
	LPCTSTR mt32emuClassName = L"mt32emu_class";
	WNDCLASS wc;
	wc.style = 0;
	wc.lpfnWndProc = &MidiInWin32::MidiInProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = mt32emuClassName;
	if (RegisterClass(&wc)) {
		qDebug() << "Class registered\n";
	} else {
		qDebug() << "Error registering class\n";
	}

	HWND hwnd = CreateWindow(mt32emuClassName, mt32emuClassName, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	if (hwnd != NULL) {
		qDebug() << "Window created\n";
	} else {
		DWORD err = GetLastError();
		qDebug() << "Error creating Window " << err << "\n";
	}
	MSG msg;
	GetMessage(&msg, hwnd, WM_QUIT, WM_QUIT);
}

Win32MidiDriver::Win32MidiDriver(SynthManager *useSynthManager) : synthManager(useSynthManager) {
	_beginthread(&MessageLoop, 16384, synthManager);
}
