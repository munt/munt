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
	QList<unsigned int> clients;

	bool processSeqEvent(snd_seq_event_t *seq_event, SynthRoute *synthRoute);
	unsigned int getSourceAddr(snd_seq_event_t *seq_event);
	QString getClientName(unsigned int clientAddr);
	MidiSession *findMidiSessionForClient(unsigned int clientAddr);

signals:
	void finished();
};

class ALSAMidiDriver : public MidiDriver {
	Q_OBJECT
	friend class ALSAProcessor;
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
