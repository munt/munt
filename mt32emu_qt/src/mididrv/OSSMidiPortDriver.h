#ifndef OSSMIDIPORT_DRIVER_H
#define OSSMIDIPORT_DRIVER_H

#include "../MidiSession.h"
#include "MidiDriver.h"

class OSSMidiPortDriver : public MidiDriver {
private:
	MidiSession *midiSession;
	int fd;
	volatile bool pendingClose;
	bool useOSSMidiPort;
	char *midiPortName;

	bool openPort();
	static void* processingThread(void *userData);

public:
	OSSMidiPortDriver(Master *useMaster);
	void start();
	void stop();
	~OSSMidiPortDriver() {}
};

#endif
