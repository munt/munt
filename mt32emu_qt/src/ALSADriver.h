#ifndef ALSADRIVER_H
#define ALSADRIVER_H

#include <alsa/asoundlib.h>

#include "QSynth.h"

class Foo : public QObject {
	Q_OBJECT
public:
	Foo(QSynth *init_synth, snd_seq_t *init_seq);

	void stop();

public slots:
	void processSeqEvents();

private:
	QSynth *synth;
	snd_seq_t *seq;
	volatile bool stopProcessing;

	bool processSeqEvent(snd_seq_event_t *seq_event);

signals:
	void finished();
};

class ALSADriver
{
public:
	ALSADriver(QSynth *synth);
	~ALSADriver();

private:
	Foo *processor;
	QThread processorThread;
};

#endif


