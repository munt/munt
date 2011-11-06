#ifndef SYNTH_ROUTE_H
#define SYNTH_ROUTE_H

#include <ctime>
#include <QtCore>
#include <mt32emu/mt32emu.h>

#include "QSynth.h"

class MidiSession;
class AudioStream;
class AudioDevice;

enum SynthRouteState {
	SynthRouteState_CLOSED,
	SynthRouteState_OPENING,
	SynthRouteState_OPEN,
	SynthRouteState_CLOSING
};

class SynthRoute : public QObject {
	Q_OBJECT
private:
	SynthRouteState state;

	QSynth qSynth;

	QList<MidiSession *> midiSessions;

	const AudioDevice *audioDevice;
	AudioStream *audioStream; // NULL until a stream is created

	void setState(SynthRouteState newState);

public:
	SynthRoute(QObject *parent = NULL);
	~SynthRoute();
	bool open();
	bool close();
	bool pushMIDIShortMessage(MT32Emu::Bit32u msg, qint64 midiNanos);
	bool pushMIDISysex(MT32Emu::Bit8u *sysex, unsigned int sysexLen, qint64 midiNanos);
	void setDACInputMode(MT32Emu::DACInputMode emuDACInputMode);
	void addMidiSession(MidiSession *midiSession);
	void removeMidiSession(MidiSession *midiSession);
	SynthRouteState getState() const;
	void setAudioDevice(const AudioDevice *newAudioDevice);

private slots:
	void handleQSynthState(SynthState synthState);

signals:
	void stateChanged(SynthRouteState state);
	void midiSessionAdded(MidiSession *midiSession);
	void midiSessionRemoved(MidiSession *midiSession);
};

#endif
