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

#include <QtGui>

#include "Win32Driver.h"

#define CALLBACK_FUNCTION NULL

/*
static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
	Win32Driver *inst;

	inst = (Win32Driver *)dwInstance;

	LPMIDIHDR pMIDIhdr = (LPMIDIHDR)dwParam1;
	if (wMsg == MIM_LONGDATA) {
		inst->midiSynth->pushMIDISysex((Bit8u*)pMIDIhdr->lpData, pMIDIhdr->dwBytesRecorded);
		std::cout << "Play SysEx message " << pMIDIhdr->dwBytesRecorded << " bytes\n";

		//	Add SysEx Buffer for reuse
		if (midiInAddBuffer(hMidiIn, pMIDIhdr, sizeof(MIDIHDR)) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to add SysEx Buffer", NULL, MB_OK | MB_ICONEXCLAMATION);
			return;
		}
		return;
	}
	if (wMsg != MIM_DATA)
		return;
	inst->midiEventQueue->pushMIDI(dwParam1);
}
*/

int Win32Driver::init(MidiSynth *pMidiSynth, MidiEventQueue *pMidiEventQueue, unsigned int midiDevID) {
	int wResult;

	midiSynth = pMidiSynth;
	midiEventQueue = pMidiEventQueue;

	//	Init midiIn port
	wResult = midiInOpen(&hMidiIn, midiDevID, (DWORD_PTR)MidiInProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
	if (wResult != MMSYSERR_NOERROR) {
		QMessageBox::warning(NULL, "mt32emu-qt MIDI error", "Failed to open MIDI input device");
		return 5;
	}

	//	Prepare SysEx midiIn buffer
	MidiInHdr.lpData = (LPSTR)sysexbuf;
	MidiInHdr.dwBufferLength = 4096;
	MidiInHdr.dwFlags = 0L;
	wResult = midiInPrepareHeader(hMidiIn, &MidiInHdr, sizeof(MIDIHDR));
	if (wResult != MMSYSERR_NOERROR) {
		QMessageBox::warning(NULL, "mt32emu-qt MIDI error", "Failed to prepare MIDI buffer header");
		return 6;
	}

	//	Add SysEx Buffer
	wResult = midiInAddBuffer(hMidiIn, &MidiInHdr, sizeof(MIDIHDR));
	if (wResult != MMSYSERR_NOERROR) {
		QMessageBox::warning(NULL, "mt32emu-qt MIDI error", "Failed to prepare add sysex buffer");
		return 7;
	}
	return 0;
}

int Win32Driver::close() {
	int wResult;

	wResult = midiInReset(hMidiIn);
	if (wResult != MMSYSERR_NOERROR) {
		QMessageBox::warning(NULL, "mt32emu-qt MIDI error", "Failed to reset MidiIn port while closing");
		return 8;
	}

	wResult = midiInUnprepareHeader(hMidiIn, &MidiInHdr, sizeof(MIDIHDR));
	if (wResult != MMSYSERR_NOERROR) {
		QMessageBox::warning(NULL, "mt32emu-qt MIDI error", "Failed to unprepare MIDI header while closing");
		return 8;
	}

	wResult = midiInClose(hMidiIn);
	if (wResult != MMSYSERR_NOERROR) {
		QMessageBox::warning(NULL, "mt32emu-qt MIDI error", "Failed to close MidiIn port");
		return 8;
	}
	return 0;
}

int Win32Driver::start() {
	return midiInStart(hMidiIn);
}
