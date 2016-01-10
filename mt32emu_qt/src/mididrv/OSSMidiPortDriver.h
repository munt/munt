#ifndef OSSMIDIPORT_DRIVER_H
#define OSSMIDIPORT_DRIVER_H

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
	virtual bool canCreatePort();
	virtual bool canDeletePort(MidiSession *midiSession);
	virtual bool canSetPortProperties(MidiSession *midiSession);
	virtual bool createPort(MidiPropertiesDialog *mpd, MidiSession *midiSession);
	virtual void deletePort(MidiSession *);
	virtual bool setPortProperties(MidiPropertiesDialog *mpd, MidiSession *midiSession);
	virtual QString getNewPortName(MidiPropertiesDialog *mpd);
};

#endif
