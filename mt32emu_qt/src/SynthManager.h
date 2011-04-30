#ifndef SYNTH_MANAGER_H
#define SYNTH_MANAGER_H

#include <ctime>
#include <QtCore>
#include <mt32emu/mt32emu.h>

#include "QSynth.h"

class MidiDriver;
class AudioDriver;

enum SynthManagerState {
	SynthManagerState_CLOSED,
	SynthManagerState_OPENING,
	SynthManagerState_OPEN,
	SynthManagerState_CLOSING
};

class SynthManager : public QObject {
	Q_OBJECT
private:
	SynthManagerState state;

	MidiDriver *midiDriver;

	int audioDeviceIndex;
	unsigned int sampleRate;

	bool midiNanosOffsetValid;
	// The number to add to a midiNanos value to get an audioNanos value. This will include a latency offset.
	qint64 midiNanosOffset;

	SynthTimestamp midiNanosToAudioNanos(qint64 midiNanos);
	void setState(SynthManagerState newState);

public:
	// FIXME: Shouldn't be public
	AudioDriver *audioDriver;
	QSynth qSynth;

	SynthManager();
	~SynthManager();
	bool open();
	bool close();
	bool pushMIDIShortMessage(MT32Emu::Bit32u msg, qint64 midiNanos);
	bool pushMIDISysex(MT32Emu::Bit8u *sysex, unsigned int sysexLen, qint64 midiNanos);

	// Quick hack:
	void startMIDI();
	void stopMIDI();

private slots:
	void handleQSynthState(SynthState synthState);

public slots:
	void setAudioDeviceIndex(int newAudioDeviceIndex);

signals:
	void stateChanged(SynthManagerState state);
};

#endif
