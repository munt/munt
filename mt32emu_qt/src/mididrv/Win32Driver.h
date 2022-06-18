#ifndef WIN32_MIDI_DRIVER_H
#define WIN32_MIDI_DRIVER_H

#include <windows.h>

#include "MidiDriver.h"
#include "../MasterClock.h"
#include "../MidiSession.h"

class Win32MidiIn {
private:
	HMIDIIN hMidiIn;
	MIDIHDR MidiInHdr;
	BYTE sysexbuf[4096];

	static void CALLBACK midiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

public:
	~Win32MidiIn();
	bool open(MidiSession *midiSession, uint midiDevID);
	bool close();
};

class Win32MidiInProcessor : public QThread {
	Q_OBJECT

protected:
	void run();
};

class Win32MidiDriver : public MidiDriver {
	friend class Win32MidiInProcessor;
private:
	static LRESULT CALLBACK midiInProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	Win32MidiInProcessor midiInProcessor;
	QList<unsigned int> midiSessionIDs;
	QList<Win32MidiIn *> midiInPorts;
	QList<MidiSession *> midiInSessions;

	MidiSession *findMidiSession(quint32 midiSessionID);

public:
	Win32MidiDriver(Master *useMaster);
	void start();
	void stop();
	bool canCreatePort();
	bool canDeletePort(MidiSession *midiSession);
	bool canReconnectPort(MidiSession *midiSession);
	PortNamingPolicy getPortNamingPolicy();
	bool createPort(int portIx, const QString &portName, MidiSession *midiSession);
	void deletePort(MidiSession *midiSession);
	void reconnectPort(int newPortIx, const QString &newPortName, MidiSession *midiSession);
	QString getNewPortNameHint(QStringList &knownPortNames);
};

#endif
