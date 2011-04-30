#ifndef TEST_MIDI_DRIVER_H
#define TEST_MIDI_DRIVER_H

#include <QThread>

#include "MidiDriver.h"

class SynthManager;

class TestProcessor : public QObject {
	Q_OBJECT
public:
	TestProcessor(SynthManager *useSynthManager);

	void stop();

public slots:
	void processSeqEvents();

private:
	SynthManager *synthManager;
	volatile bool stopProcessing;

signals:
	void finished();
};

class TestMidiDriver : public MidiDriver {
	Q_OBJECT
public:
	TestMidiDriver(SynthManager *synthManager);
	~TestMidiDriver();

private:
	TestProcessor *processor;
	QThread processorThread;
};

#endif


