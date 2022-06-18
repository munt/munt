#ifndef OSSMIDIPORT_DRIVER_H
#define OSSMIDIPORT_DRIVER_H

#include <pthread.h>

#include "../MidiSession.h"
#include "MidiDriver.h"

class OSSMidiPortDriver : public MidiDriver {
private:
	struct OSSMidiPortData {
		MidiSession *midiSession;
		QString midiPortName;
		bool sequencerMode;
		pthread_t processingThreadID;
		volatile bool stopProcessing;
	};

	static void *processingThread(void *userData);
	static void enumPorts(QStringList &midiPortNames);

	QList<OSSMidiPortData *> sessions;

	bool startSession(MidiSession *midiSession, const QString midiPortName, bool sequencerMode);
	void stopSession(OSSMidiPortData *data);

public:
	OSSMidiPortDriver(Master *useMaster);
	~OSSMidiPortDriver() {}
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
