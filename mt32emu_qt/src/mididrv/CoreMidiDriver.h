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
	unsigned int sessionID;

	static void readProc(const MIDIPacketList *packetList, void *readProcRefCon, void *srcConnRefCon);
	static void midiNotifyProc(MIDINotification const *message, void *refCon);

	void createDestination(MidiSession *midiSession, QString sessionID);
	void disposeDestination(CoreMidiSession *data);

public:
	CoreMidiDriver(Master *useMaster);
	~CoreMidiDriver() {}
	void start();
	void stop();
	virtual bool canCreatePort();
	virtual bool canDeletePort(MidiSession *midiSession);
	virtual bool canSetPortProperties(MidiSession *midiSession);
	virtual bool createPort(MidiPropertiesDialog *mpd, MidiSession *midiSession);
	virtual void deletePort(MidiSession *);
	virtual bool setPortProperties(MidiPropertiesDialog *mpd, MidiSession *midiSession);
	virtual QString getNewPortName(MidiPropertiesDialog *mpd);
};

#endif
