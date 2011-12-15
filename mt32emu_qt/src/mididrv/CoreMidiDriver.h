#ifndef COREMIDI_DRIVER_H
#define COREMIDI_DRIVER_H

#include <CoreMIDI/MIDIServices.h>

#include "../MidiSession.h"
#include "MidiDriver.h"

class CoreMidiDriver : public MidiDriver {
private:
    MIDIClientRef client;
    MIDIEndpointRef outDest;

    static void midiNotifyProc(MIDINotification const *message, void *refCon);
    void createDestination();

public:
	CoreMidiDriver(Master *useMaster);
	void start();
	void stop();
    ~CoreMidiDriver() {}
};

#endif
