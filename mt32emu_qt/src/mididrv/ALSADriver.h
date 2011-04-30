#ifndef ALSA_MIDI_DRIVER_H
#define ALSA_MIDI_DRIVER_H

#include <QObject>
#include <QThread>

#include <alsa/asoundlib.h>

#include "MidiDriver.h"

class SynthManager;

class ALSAProcessor : public QObject {
	Q_OBJECT
public:
	ALSAProcessor(SynthManager *useSynthManager, snd_seq_t *init_seq);

	void stop();

public slots:
	void processSeqEvents();

private:
	SynthManager *synthManager;
	snd_seq_t *seq;
	volatile bool stopProcessing;

	bool processSeqEvent(snd_seq_event_t *seq_event);

signals:
	void finished();
};

class ALSAMidiDriver : public MidiDriver {
	Q_OBJECT
public:
	ALSAMidiDriver(SynthManager *useSynthManager);
	~ALSAMidiDriver();

private:
	ALSAProcessor *processor;
	QThread processorThread;
};

#endif


