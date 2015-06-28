/* Copyright (C) 2011, 2012, 2013, 2014 Jerome Fisher, Sergey V. Mikayev
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

#include <QMessageBox>

#include <windows.h>
#include <process.h>

#include "Win32Driver.h"

#include "../MidiPropertiesDialog.h"

static Win32MidiDriver *driver;
static HWND hwnd = NULL;
static MasterClockNanos startMasterClock; // FIXME: Should actually be per-session but doesn't seem to be a real win

LRESULT CALLBACK Win32MidiDriver::midiInProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_APP: {
		// Closing session
		quint32 midiSessionID = (quint32)wParam;
		MidiSession *midiSession = driver->findMidiSession(midiSessionID);
		driver->midiSessionIDs.removeAll(midiSessionID);
		if (!midiSession) {
			qDebug() << "Win32MidiDriver: Invalid midiSession ID supplied:" << "0x" + QString::number(midiSessionID, 16);
			return 0;
		}
		qDebug() << "Win32MidiDriver: Session ID =" << "0x" + QString::number(midiSessionID, 16) << "finished";
		driver->deleteMidiSession(midiSession);
		return 1;
	}

	case WM_COPYDATA: {
		COPYDATASTRUCT *cds = (COPYDATASTRUCT *)lParam;
		quint32 midiSessionID = (quint32)cds->dwData;
		DWORD *data = (DWORD *)cds->lpData;
		if (data[0] == 0) { // Special value, mark of a non-Sysex message
			if (data[1] == (DWORD)-1) { // Special value, mark of a handshaking message
				// Sync the timesource in the driver with MasterClock
				LARGE_INTEGER t = {{data[3], (LONG)data[4]}};
				startMasterClock = t.QuadPart - MasterClock::getClockNanos();
				// Process handshaking message
				QString appName = QFileInfo(QString((const char *)&data[5])).fileName();
				MidiSession *midiSession = driver->createMidiSession(appName);
				if (!midiSession) {
					qDebug() << "Win32MidiDriver: Failed to create new session";
					return 0;
				}
				do
					midiSessionID = (quint32)qrand();
				while (driver->midiSessionIDs.indexOf(midiSessionID) >= 0);
				driver->midiSessionIDs.append(midiSessionID);
				driver->showBalloon("Connected application:", appName);
				qDebug() << "Win32MidiDriver: Connected application" << appName;
				qDebug() << "Win32MidiDriver: Session ID:" << "0x" + QString::number(midiSessionID, 16) << "with protocol version" << data[2];
				return (LRESULT)midiSessionID;
			} else if (data[1] == 0) { // Special value, mark of a short MIDI message
				// Process short MIDI message
				MidiSession *midiSession = driver->findMidiSession(midiSessionID);
				if (!midiSession) {
					qDebug() << "Win32MidiDriver: Invalid midiSession ID supplied:" << "0x" + QString::number(midiSessionID, 16);
					return 0;
				}
				LARGE_INTEGER t = { { data[2], (LONG)data[3] } };
//				qDebug() << "D" << 1e-6 * ((t.QuadPart - startMasterClock) - MasterClock::getClockNanos());
				midiSession->getSynthRoute()->pushMIDIShortMessage(data[4], t.QuadPart - startMasterClock);
				return 1;
			}
		} else {
			// Process Sysex
			MidiSession *midiSession = driver->findMidiSession(midiSessionID);
			if (!midiSession) {
				qDebug() << "Win32MidiDriver: Invalid midiSession ID supplied:" << "0x" + QString::number(midiSessionID, 16);
				return 0;
			}
			midiSession->getSynthRoute()->pushMIDISysex((MT32Emu::Bit8u *)cds->lpData, cds->cbData, MasterClock::getClockNanos());
			return 1;
		}
	}

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

MidiSession *Win32MidiDriver::findMidiSession(quint32 midiSessionID) {
	int midiSessionIx = midiSessionIDs.indexOf(midiSessionID);
	return ((midiSessionIx < 0) || (midiSessions.size() <= midiSessionIx)) ? NULL : midiSessions.at(midiSessionIx);
}

void Win32MidiInProcessor::run() {
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
	hwnd = NULL;
}

Win32MidiDriver::Win32MidiDriver(Master *useMaster) : MidiDriver(useMaster) {
	master = useMaster;
	driver = this;
	name = "Win32_MIDI";
}

void Win32MidiDriver::start() {
	midiInProcessor.start(QThread::TimeCriticalPriority);
}

void Win32MidiDriver::stop() {
	if (hwnd != NULL) {
		PostMessage(hwnd, WM_QUIT, 0, 0);
		waitForProcessingThread(midiInProcessor, 10 * MasterClock::NANOS_PER_MILLISECOND);
	}
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
	if (midiInPort->open(midiSession, mpd->getCurrentMidiPortIndex())) {
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
		int i = midiSessions.indexOf(midiSession);
		if ((0 <= i) && (i < midiSessionIDs.size())) {
			quint32 id = midiSessionIDs.at(i);
			midiSessionIDs.removeAll(id);
			midiSessions.removeAt(i);
		}
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
	if (midiInPort->open(midiSession, mpd->getCurrentMidiPortIndex())) {
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

	MidiSession *midiSession = (MidiSession *)dwInstance;

	LPMIDIHDR pMIDIhdr = (LPMIDIHDR)dwParam1;
	if (wMsg == MIM_LONGDATA) {
		if (pMIDIhdr->dwBytesRecorded == 0) {
			// 0 length means returning the buffer to the application when closing
			return;
		}
		// Use QMidiStreamParser to extract well-formed SysEx messages from buffer
		QMidiStreamParser &qMidiStreamParser = *midiSession->getQMidiStreamParser();
		qMidiStreamParser.setTimestamp(MasterClock::getClockNanos());
		qMidiStreamParser.parseStream((BYTE *)pMIDIhdr->lpData, pMIDIhdr->dwBytesRecorded);

		// Add SysEx Buffer for reuse
		if (midiInAddBuffer(hMidiIn, pMIDIhdr, sizeof(MIDIHDR)) != MMSYSERR_NOERROR) {
			QMessageBox::critical(NULL, "Win32MidiIn Error", "Failed to add SysEx Buffer for reuse");
			return;
		}
		return;
	}
	if (wMsg == MIM_DATA) {
		// No need to use QMidiStreamParser for short messages: they are guaranteed to have explicit status byte
		// if ((dwParam1 & 0xF8) == 0xF8) // No support for System Realtime yet
		midiSession->getSynthRoute()->pushMIDIShortMessage(dwParam1, MasterClock::getClockNanos());
	}
}

bool Win32MidiIn::open(MidiSession *midiSession, unsigned int midiDevID) {
	int wResult;

	// Init midiIn port
	wResult = midiInOpen(&hMidiIn, midiDevID, (DWORD_PTR)midiInProc, (DWORD_PTR)midiSession, CALLBACK_FUNCTION);
	if (wResult != MMSYSERR_NOERROR) {
		QMessageBox::critical(NULL, "Win32MidiIn Error", "Failed to open MIDI input port");
		return false;
	}

	// Prepare SysEx midiIn buffer
	MidiInHdr.lpData = (LPSTR)sysexbuf;
	MidiInHdr.dwBufferLength = 4096;
	MidiInHdr.dwFlags = 0L;
	wResult = midiInPrepareHeader(hMidiIn, &MidiInHdr, sizeof(MIDIHDR));
	if (wResult != MMSYSERR_NOERROR) {
		QMessageBox::critical(NULL, "Win32MidiIn Error", "Failed to prepare MIDI buffer header");
		return false;
	}

	// Add SysEx Buffer
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
