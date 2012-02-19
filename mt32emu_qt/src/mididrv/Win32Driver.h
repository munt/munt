#ifndef WIN32_MIDI_DRIVER_H
#define WIN32_MIDI_DRIVER_H

#include <windows.h>

#include "MidiDriver.h"
#include "../MidiSession.h"

class Win32MidiIn {
private:
	HMIDIIN hMidiIn;
	MIDIHDR MidiInHdr;
	BYTE sysexbuf[4096];

	static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

public:
	~Win32MidiIn();
	bool open(SynthRoute *synthRoute, unsigned int midiDevID);
	bool close();
	UINT getID();
};

class Win32MidiDriver : public MidiDriver {
private:
	static LRESULT CALLBACK MidiInProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void MessageLoop(void *);
	static qint64 TimeToMasterClockNanos(DWORD time);
	static void enumPorts(QList<QString> &midiInPortNames);

	QList<Win32MidiIn *> midiInPorts;
	QList<MidiSession *> midiInSessions;

public:
	Win32MidiDriver(Master *useMaster);
	void start();
	void stop();
	bool canCreatePort();
	bool canDeletePort(MidiSession *midiSession);
	bool canSetPortProperties(MidiSession *midiSession);
	bool createPort(MidiPropertiesDialog *mpd, MidiSession *midiSession);
	void deletePort(MidiSession *midiSession);
	bool setPortProperties(MidiPropertiesDialog *mpd, MidiSession *midiSession);
	QString getNewPortName(MidiPropertiesDialog *mpd);
};

#endif
