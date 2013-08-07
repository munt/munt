#ifndef SYNTH_ROUTE_H
#define SYNTH_ROUTE_H

#include <ctime>
#include <QtCore>
#include <mt32emu/mt32emu.h>

#include "QSynth.h"
#include "MasterClock.h"
#include "MidiRecorder.h"

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
	MidiRecorder recorder;

	const AudioDevice *audioDevice;
	AudioStream *audioStream; // NULL until a stream is created

	void setState(SynthRouteState newState);

public:
	SynthRoute(QObject *parent = NULL);
	~SynthRoute();
	bool open();
	bool close();
	bool reset();

	const QString getPatchName(int partNum) const;
	const MT32Emu::Partial *getPartial(int partialNum) const;
	const MT32Emu::Poly *getFirstActivePolyOnPart(unsigned int partNum) const;
	unsigned int getPartialCount() const;

	bool pushMIDIShortMessage(MT32Emu::Bit32u msg, MasterClockNanos midiNanos);
	bool pushMIDISysex(MT32Emu::Bit8u *sysex, unsigned int sysexLen, MasterClockNanos midiNanos);
	void setMasterVolume(int masterVolume);
	void setOutputGain(float outputGain);
	void setReverbOutputGain(float reverbOutputGain);
	void setReverbEnabled(bool reverbEnabled);
	void setReverbOverridden(bool reverbOverridden);
	void setReverbSettings(int reverbMode, int reverbTime, int reverbLevel);
	void setDACInputMode(MT32Emu::DACInputMode emuDACInputMode);

	void addMidiSession(MidiSession *midiSession);
	void removeMidiSession(MidiSession *midiSession);
	void setMidiSessionName(MidiSession *midiSession, QString name);
	bool hasMIDISessions() const;
	SynthRouteState getState() const;
	void setAudioDevice(const AudioDevice *newAudioDevice);
	MidiRecorder *getMidiRecorder();
	void getSynthProfile(SynthProfile &synthProfile) const;
	void setSynthProfile(const SynthProfile &synthProfile, QString useSynthProfileName);
	bool connectSynth(const char *signal, const QObject *receiver, const char *slot) const;
	bool connectReportHandler(const char *signal, const QObject *receiver, const char *slot) const;

private slots:
	void handleQSynthState(SynthState synthState);

signals:
	void stateChanged(SynthRouteState state);
	void midiSessionAdded(MidiSession *midiSession);
	void midiSessionRemoved(MidiSession *midiSession);
	void midiSessionNameChanged(MidiSession *midiSession);
};

#endif
