#ifndef COREMIDI_DRIVER_H
#define COREMIDI_DRIVER_H

#include <CoreMIDI/MIDIServices.h>

#include "../MidiSession.h"
#include "MidiDriver.h"

class CoreMidiDriver : public MidiDriver {
private:
	struct CoreMidiSession {
		QString sessionID;
		MIDIEndpointRef outDest;
		MidiSession *midiSession;
	};

	MIDIClientRef client;
	QList<CoreMidiSession *> sessions;
	uint nextSessionID;

	static void readProc(const MIDIPacketList *packetList, void *readProcRefCon, void *srcConnRefCon);
	static void midiNotifyProc(MIDINotification const *message, void *refCon);

	void createDestination(MidiSession *midiSession, QString sessionID);
	void disposeDestination(CoreMidiSession *data);

public:
	CoreMidiDriver(Master *useMaster);
	~CoreMidiDriver() {}
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
