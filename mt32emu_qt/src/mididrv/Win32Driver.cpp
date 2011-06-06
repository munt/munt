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
#include <process.h>
#include <iostream>
#include "../ClockSync.h"
#include "../MasterClock.h"

#include "Win32Driver.h"

static MidiSession *midiSession = NULL;
static HWND hwnd = NULL;
static ClockSync clockSync;

LRESULT CALLBACK Win32MidiDriver::MidiInProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_APP: // closing session
		std::cout << "Session was closed\n";
			// reset the synth if necessary
		return 1;

	case WM_COPYDATA:
//		std::cout << "Incoming data! wParam = " << wParam << "\n";
		COPYDATASTRUCT *cds;
		cds = (COPYDATASTRUCT *)lParam;
//		std::cout << "Instance = " << cds->dwData << ", Data length = " << cds->cbData << "\n";
		DWORD *data;
		data = (DWORD *)cds->lpData;
//		std::cout << "Data = " << data[0] << ", " << data[1] << ", " << data[2] << ", " << data[3] << "\n";
		if (data[0] == 0) {
			if (data[1] == -1) {
				// Process handshaking message
				DWORD inst = (timeGetTime() & 255); // just a random value
				std::cout << "Instance = " << inst << ", Version = " << data[2] << "\n";
				std::cout << "Connected application = " << (char *)&data[3] << "\n";
				return inst;
			} else if (data[1] == 0) {
				// Process short MIDI message
//				std::cout << "Incoming message! msg = " << data[3] << ", timestamp = " << data[2] << "\n";
				midiSession->getSynthRoute()->pushMIDIShortMessage(data[3], ((qint64)data[2] * 1000));
				return 1;
			}
		} else {
			// Process Sysex
			midiSession->getSynthRoute()->pushMIDISysex((MT32Emu::Bit8u *)cds->lpData, cds->cbData, MasterClock::getClockNanos());
			return 1;
		}
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

void Win32MidiDriver::MessageLoop(void *) {
	HINSTANCE hInstance = GetModuleHandle(NULL);
	LPCTSTR mt32emuClassName = "mt32emu_class";
	WNDCLASS wc;
	wc.style = 0;
	wc.lpfnWndProc = &Win32MidiDriver::MidiInProc;
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

	hwnd = CreateWindow(mt32emuClassName, mt32emuClassName, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	if (hwnd != NULL) {
		qDebug() << "Window created\n";
	} else {
		DWORD err = GetLastError();
		qDebug() << "Error creating Window " << err << "\n";
	}
	MSG msg;
	GetMessage(&msg, hwnd, WM_QUIT, WM_QUIT);
}

Win32MidiDriver::Win32MidiDriver(Master *useMaster) : MidiDriver(useMaster) {
	if(midiSession == NULL) {
		midiSession = useMaster->createMidiSession(this, "Combined Win32msg Session");
	}
}

void Win32MidiDriver::start() {
	clockSync.reset();
	_beginthread(&MessageLoop, 16384, NULL);
}

void Win32MidiDriver::stop() {
	PostMessage(hwnd, WM_QUIT, 0, 0);
}
