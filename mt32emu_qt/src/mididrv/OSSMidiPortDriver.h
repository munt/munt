#ifndef OSSMIDIPORT_DRIVER_H
#define OSSMIDIPORT_DRIVER_H

#include "../MidiSession.h"
#include "MidiDriver.h"

class OSSMidiPortDriver : public MidiDriver {
private:
	struct OSSMidiPortData {
		QString midiPortName;
		bool sequencerMode;
		volatile bool pendingClose;
	};

	static void *processingThread(void *userData);

	QList<OSSMidiPortData *> sessions;

	void startSession(const QString midiPortName, bool sequencerMode);
	void stopSession(OSSMidiPortData *data);

public:
	OSSMidiPortDriver(Master *useMaster);
	void start();
	void stop();
	~OSSMidiPortDriver() {}
};

#endif
