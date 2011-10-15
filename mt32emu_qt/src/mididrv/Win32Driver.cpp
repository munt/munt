/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtGlobal>

#include <windows.h>
#include <process.h>

#include "../MasterClock.h"
#include "Win32Driver.h"

static Win32MidiDriver *driver;
static HWND hwnd = NULL;
static qint64 startMasterClock;
static bool disableReset = true;

qint64 Win32MidiDriver::TimeToMasterClockNanos(DWORD time) {
	return (qint64)time * MasterClock::NANOS_PER_MILLISECOND - startMasterClock;
}

LRESULT CALLBACK Win32MidiDriver::MidiInProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static MidiSession *newMidiSession = NULL;
	MidiSession *midiSession;
	switch (uMsg) {
	case WM_APP: // closing session
		midiSession = (MidiSession*)wParam;
		if (!disableReset) {
			Master::getInstance()->deleteMidiSession(midiSession);
			if (midiSession == newMidiSession) {
				newMidiSession = NULL;
			}
		}
		qDebug() << "Session" << midiSession << "finished";
		return 1;

	case WM_COPYDATA:
		COPYDATASTRUCT *cds;
		cds = (COPYDATASTRUCT *)lParam;
		midiSession = (MidiSession*)cds->dwData;
		DWORD *data;
		data = (DWORD *)cds->lpData;
		if (data[0] == 0) {				// special value, mark of a non-Sysex message
			if (data[1] == (DWORD)-1) {		// special value, mark of a handshaking message
				// Sync the timesource in the driver with MasterClock
				startMasterClock = (qint64)cds->dwData * MasterClock::NANOS_PER_MILLISECOND - MasterClock::getClockNanos();
				// Process handshaking message
				if (!newMidiSession || !disableReset) {
					newMidiSession = driver->master->createMidiSession(driver, "Combined Win32msg Session");
				}
				midiSession = newMidiSession;
				driver->master->showBalloon("Connected application:", (const char *)&data[3]);
				qDebug() << "Connected application" << (char *)&data[3];
				qDebug() << "Session" << midiSession << "protocol version" << data[2];
				if (!midiSession) {
					qDebug() << "Failed to create new session";
					return 0;
				}
				return (LRESULT)midiSession;
			} else if (data[1] == 0) {	// special value, mark of a short MIDI message
				// Process short MIDI message
				midiSession->getSynthRoute()->pushMIDIShortMessage(data[3], TimeToMasterClockNanos(data[2]));
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
	if (!RegisterClass(&wc)) {
		qDebug() << "Error registering message class";
	}

#ifndef HWND_MESSAGE
#define HWND_MESSAGE ((HWND)-3)
#endif

	hwnd = CreateWindow(mt32emuClassName, "mt32emu_message_window", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	if (hwnd == NULL) {
		DWORD err = GetLastError();
		qDebug() << "Error creating message window " << err << "\n";
	}
	MSG msg;
	GetMessage(&msg, hwnd, WM_QUIT, WM_QUIT);
}

Win32MidiDriver::Win32MidiDriver(Master *useMaster) : MidiDriver(useMaster) {
	master = useMaster;
	driver = this;
}

void Win32MidiDriver::start() {
	_beginthread(&MessageLoop, 16384, NULL);
}

void Win32MidiDriver::stop() {
	PostMessage(hwnd, WM_QUIT, 0, 0);
}

void DisableReset(bool useDisableReset) {
	disableReset = useDisableReset;
}