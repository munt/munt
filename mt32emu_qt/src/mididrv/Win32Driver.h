#ifndef WIN32_MIDI_DRIVER_H
#define WIN32_MIDI_DRIVER_H

#include <windows.h>

#include "MidiDriver.h"

class Win32MidiDriver : public MidiDriver {
private:
	SynthManager *synthManager;

	static LRESULT CALLBACK MidiInProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void MessageLoop(SynthManager *synthManager);

public:
	Win32MidiDriver(SynthManager *useSynthManager);
};

#endif
