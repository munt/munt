/* Copyright (C) 2011, 2012, 2013 Jerome Fisher, Sergey V. Mikayev
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
#include <QMessageBox>

#include <windows.h>
#include <process.h>

#include "Win32Driver.h"

#include "../MidiPropertiesDialog.h"

static Win32MidiDriver *driver;
static HWND hwnd = NULL;
static MasterClockNanos startMasterClock;

MasterClockNanos Win32MidiDriver::timeToMasterClockNanos(MasterClockNanos time) {
	return time - startMasterClock;
}

LRESULT CALLBACK Win32MidiDriver::midiInProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	MidiSession *midiSession;
	switch (uMsg) {
	case WM_APP: // closing session
		midiSession = (MidiSession*)wParam;
		if (driver->midiSessions.indexOf(midiSession) < 0) {
			qDebug() << "Win32MidiDriver: Invalid midiSession handle supplied";
			return 0;
		}
		qDebug() << "Win32MidiDriver: Session" << midiSession << "finished";
		driver->deleteMidiSession(midiSession);
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
				LARGE_INTEGER t = {{data[3], (LONG)data[4]}};
				startMasterClock = t.QuadPart - MasterClock::getClockNanos();
				// Process handshaking message
				QString appName = QFileInfo(QString((const char *)&data[5])).fileName();
				midiSession = driver->createMidiSession(appName);
				driver->showBalloon("Connected application:", appName);
				qDebug() << "Win32MidiDriver: Connected application" << appName;
				qDebug() << "Win32MidiDriver: Session" << midiSession << "protocol version" << data[2];
				if (!midiSession) {
					qDebug() << "Win32MidiDriver: Failed to create new session";
					return 0;
				}
				return (LRESULT)midiSession;
			} else if (data[1] == 0) {	// special value, mark of a short MIDI message
				// Process short MIDI message
				if (driver->midiSessions.indexOf(midiSession) < 0) {
					qDebug() << "Win32MidiDriver: Invalid midiSession handle supplied";
					return 0;
				}
				LARGE_INTEGER t = {{data[2], (LONG)data[3]}};
//				qDebug() << "D" << 1e-6 * (MasterClock::getClockNanos() - timeToMasterClockNanos(t.QuadPart));
				midiSession->getSynthRoute()->pushMIDIShortMessage(data[4], timeToMasterClockNanos(t.QuadPart));
				return 1;
			}
		} else {
			// Process Sysex
			if (driver->midiSessions.indexOf(midiSession) < 0) {
				qDebug() << "Win32MidiDriver: Invalid midiSession handle supplied";
				return 0;
			}
			midiSession->getSynthRoute()->pushMIDISysex((MT32Emu::Bit8u *)cds->lpData, cds->cbData, MasterClock::getClockNanos());
			return 1;
		}
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

void Win32MidiDriver::messageLoop(void *) {
	HINSTANCE hInstance = GetModuleHandle(NULL);
	LPCTSTR mt32emuClassName = "mt32emu_class";
	WNDCLASS wc;
	wc.style = 0;
	wc.lpfnWndProc = &Win32MidiDriver::midiInProc;
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
	name = "Win32_MIDI";
}

void Win32MidiDriver::start() {
	_beginthread(&messageLoop, 16384, NULL);
}

void Win32MidiDriver::stop() {
	PostMessage(hwnd, WM_QUIT, 0, 0);
	for (int i = 0; i < midiInPorts.size(); i++) {
		delete midiInPorts[i];
		deleteMidiSession(midiInSessions[i]);
	}
	midiInPorts.clear();
	midiInSessions.clear();
}

bool Win32MidiDriver::canCreatePort() {
	return midiInGetNumDevs() > 0;
}

bool Win32MidiDriver::canDeletePort(MidiSession *midiSession) {
	return midiInSessions.indexOf(midiSession) >= 0 || midiSessions.indexOf(midiSession) >= 0;
}

bool Win32MidiDriver::canSetPortProperties(MidiSession *midiSession) {
	return midiInSessions.indexOf(midiSession) >= 0;
}

bool Win32MidiDriver::createPort(MidiPropertiesDialog *mpd, MidiSession *midiSession) {
	Win32MidiIn *midiInPort = new Win32MidiIn;
	if (midiInPort->open(midiSession->getSynthRoute(), mpd->getCurrentMidiPortIndex())) {
		midiInPorts.append(midiInPort);
		midiInSessions.append(midiSession);
		return true;
	}
	delete midiInPort;
	return false;
}

void Win32MidiDriver::deletePort(MidiSession *midiSession) {
	int portIx = midiInSessions.indexOf(midiSession);
	if (portIx < 0) {
		midiSessions.removeOne(midiSession);
		return;
	}
	delete midiInPorts.takeAt(portIx);
	midiInSessions.removeAt(portIx);
}

bool Win32MidiDriver::setPortProperties(MidiPropertiesDialog *mpd, MidiSession *midiSession) {
	int id = -1;
	Win32MidiIn *midiInPort = NULL;
	if (midiSession != NULL) {
		int portIx = midiInSessions.indexOf(midiSession);
		if (portIx < 0) return false;
		midiInPort = midiInPorts[portIx];
		id = midiInPort->getID();
	}
	QList<QString> midiInPortNames;
	Win32MidiDriver::enumPorts(midiInPortNames);
	mpd->setMidiList(midiInPortNames, id);
	mpd->setMidiPortNameEditorEnabled(false);
	if (mpd->exec() != QDialog::Accepted) return false;
	if (midiSession == NULL) {
		if (mpd->getCurrentMidiPortIndex() < 0) return false;
		return true;
	}
	if (id == mpd->getCurrentMidiPortIndex()) return false;
	midiInPort->close();
	if (midiInPort->open(midiSession->getSynthRoute(), mpd->getCurrentMidiPortIndex())) {
		midiSession->getSynthRoute()->setMidiSessionName(midiSession, mpd->getMidiPortName());
		return true;
	}
	master->deleteMidiPort(midiSession);
	return false;
}

QString Win32MidiDriver::getNewPortName(MidiPropertiesDialog *mpd) {
	mpd->setMidiPortNameEditorEnabled(false);
	if (setPortProperties(mpd, NULL)) {
		return mpd->getMidiPortName();
	}
	return "";
}

void Win32MidiDriver::enumPorts(QList<QString> &midiInPortNames) {
	UINT inPortNum = midiInGetNumDevs();
	for (UINT i = 0; i < inPortNum; i++) {
		MIDIINCAPS mic;
		if (midiInGetDevCaps(i, &mic, sizeof(MIDIINCAPS)) != MMSYSERR_NOERROR) {
			midiInPortNames.append("");
		} else {
			midiInPortNames.append("MidiIn" + QString().setNum(i) + ": " + QString().fromLocal8Bit(mic.szPname));
		}
	}
}

void CALLBACK Win32MidiIn::midiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
	Q_UNUSED(dwParam2);

	SynthRoute *synthRoute = (SynthRoute *)dwInstance;

	LPMIDIHDR pMIDIhdr = (LPMIDIHDR)dwParam1;
	if (wMsg == MIM_LONGDATA) {
		if (pMIDIhdr->dwBytesRecorded == 0) {
			// 0 length means returning the buffer to the application when closing
			return;
		}
		synthRoute->pushMIDISysex((BYTE *)pMIDIhdr->lpData, pMIDIhdr->dwBytesRecorded, MasterClock::getClockNanos());

		//	Add SysEx Buffer for reuse
		if (midiInAddBuffer(hMidiIn, pMIDIhdr, sizeof(MIDIHDR)) != MMSYSERR_NOERROR) {
			QMessageBox::critical(NULL, "Win32MidiIn Error", "Failed to add SysEx Buffer for reuse");
			return;
		}
		return;
	}
	if (wMsg == MIM_DATA) synthRoute->pushMIDIShortMessage(dwParam1, MasterClock::getClockNanos());
}

bool Win32MidiIn::open(SynthRoute *synthRoute, unsigned int midiDevID) {
	int wResult;

	//	Init midiIn port
	wResult = midiInOpen(&hMidiIn, midiDevID, (DWORD_PTR)midiInProc, (DWORD_PTR)synthRoute, CALLBACK_FUNCTION);
	if (wResult != MMSYSERR_NOERROR) {
		QMessageBox::critical(NULL, "Win32MidiIn Error", "Failed to open MIDI input port");
		return false;
	}

	//	Prepare SysEx midiIn buffer
	MidiInHdr.lpData = (LPSTR)sysexbuf;
	MidiInHdr.dwBufferLength = 4096;
	MidiInHdr.dwFlags = 0L;
	wResult = midiInPrepareHeader(hMidiIn, &MidiInHdr, sizeof(MIDIHDR));
	if (wResult != MMSYSERR_NOERROR) {
		QMessageBox::critical(NULL, "Win32MidiIn Error", "Failed to prepare MIDI buffer header");
		return false;
	}

	//	Add SysEx Buffer
	wResult = midiInAddBuffer(hMidiIn, &MidiInHdr, sizeof(MIDIHDR));
	if (wResult != MMSYSERR_NOERROR) {
		QMessageBox::critical(NULL, "Win32MidiIn Error", "Failed to add SysEx buffer");
		return false;
	}
	return midiInStart(hMidiIn) == MMSYSERR_NOERROR;
}

bool Win32MidiIn::close() {
	int wResult;

	wResult = midiInReset(hMidiIn);
	if (wResult != MMSYSERR_NOERROR) {
		qDebug() << "Win32MidiIn: Failed to reset MIDI input port";
	}

	wResult = midiInUnprepareHeader(hMidiIn, &MidiInHdr, sizeof(MIDIHDR));
	if (wResult != MMSYSERR_NOERROR) {
		qDebug() << "Win32MidiIn: Failed to unprepare MIDI header";
	}

	wResult = midiInClose(hMidiIn);
	if (wResult != MMSYSERR_NOERROR) {
		qDebug() << "Win32MidiIn: Failed to close MIDI input port";
	}

	return true;
}

UINT Win32MidiIn::getID() {
	UINT id;
	if (midiInGetID(hMidiIn, &id) == MMSYSERR_NOERROR) return id;
	return -1;
}

Win32MidiIn::~Win32MidiIn() {
	close();
}
