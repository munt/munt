#ifndef ALSA_MIDI_DRIVER_H
#define ALSA_MIDI_DRIVER_H

#include <QObject>
#include <QThread>

#include <alsa/asoundlib.h>

#include "MidiDriver.h"

class SynthRoute;
class ALSAMidiDriver;
class MidiSession;

class ALSAProcessor : public QObject {
	Q_OBJECT
public:
	ALSAProcessor(ALSAMidiDriver *useALSAMidiDriver, snd_seq_t *init_seq);

	void stop();

public slots:
	void processSeqEvents();

private:
	ALSAMidiDriver *alsaMidiDriver;
	snd_seq_t *seq;
	volatile bool stopProcessing;

	bool processSeqEvent(snd_seq_event_t *seq_event, SynthRoute *synthRoute);

signals:
	void finished();
};

class ALSAMidiDriver : public MidiDriver {
	Q_OBJECT
public:
	ALSAMidiDriver(Master *useMaster);
	~ALSAMidiDriver();
	void start();
	void stop();

private:
	ALSAProcessor *processor;
	QThread processorThread;
};

#endif


