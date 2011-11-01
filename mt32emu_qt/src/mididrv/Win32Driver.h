#ifndef WIN32_MIDI_DRIVER_H
#define WIN32_MIDI_DRIVER_H

#include <windows.h>

#include "../MidiSession.h"
#include "MidiDriver.h"

class Win32MidiDriver : public MidiDriver {
private:
	static LRESULT CALLBACK MidiInProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void MessageLoop(void *);
	static qint64 TimeToMasterClockNanos(DWORD time);

public:
	Win32MidiDriver(Master *useMaster);
	void start();
	void stop();
};

#endif
