#ifndef JACK_MIDI_DRIVER_H
#define JACK_MIDI_DRIVER_H

#include <QThread>

#include "MidiDriver.h"

class Master;
class JACKClient;

class JACKMidiDriver : public MidiDriver {
	Q_OBJECT

public:
	static MasterClockNanos jackFrameTimeToMasterClockNanos(MasterClockNanos refNanos, quint64 eventJackTime, quint64 refJackTime);
	static bool pushMIDIMessage(MidiSession *midiSession, MasterClockNanos eventTimestamp, size_t midiBufferSize, uchar *midiBuffer);
	static bool playMIDIMessage(MidiSession *midiSession, quint64 eventTimestamp, size_t midiBufferSize, uchar *midiBuffer);

	JACKMidiDriver(Master *master);
	~JACKMidiDriver();
	void start();
	void stop();
	bool canDeletePort(MidiSession *midiSession);
	void deletePort(MidiSession *midiSession);
	bool createJACKPort(bool exclusive);

private:
	QList<JACKClient *> jackClients;
	QList<MidiSession *> exclusiveSessions;

private slots:
	void onJACKMidiPortDeleted(MidiSession *midiSession);
};

#endif
