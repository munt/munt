#ifndef WIN32DRIVER_H
#define WIN32DRIVER_H

#include "MidiSynth.h"
#include "MidiEventQueue.h"

// FIXME: Hack
typedef int HMIDIIN;
typedef int MIDIHDR;
typedef MIDIHDR *LPMIDIHDR;

typedef unsigned int UINT;
typedef int *DWORD_PTR;

static const int MIM_LONGDATA = 1;
#define CALLBACK

class Win32Driver {
private:
	MidiSynth *midiSynth;
	MidiEventQueue *midiEventQueue;

	HMIDIIN hMidiIn;
	MIDIHDR MidiInHdr;
	unsigned char sysexBuf[4096];
	static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
public:
	int init(MidiSynth *pMidiSynth, MidiEventQueue *pMidiStream, unsigned int midiDevID);

	int close();
	int start();
};

#endif
