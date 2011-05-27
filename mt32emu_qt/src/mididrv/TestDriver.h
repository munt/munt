#ifndef TEST_MIDI_DRIVER_H
#define TEST_MIDI_DRIVER_H

#include <QThread>

#include "MidiDriver.h"
#include "../Master.h"

class SynthRoute;

class TestMidiDriver;

class TestProcessor : public QObject {
	Q_OBJECT
public:
	TestProcessor(TestMidiDriver *useTestMidiDriver);

	void stop();

public slots:
	void processSeqEvents();

private:
	TestMidiDriver *testMidiDriver;
	volatile bool stopProcessing;

signals:
	void finished();
};

class TestMidiDriver : public MidiDriver {
	friend class TestProcessor;
	Q_OBJECT
public:
	TestMidiDriver(Master *master);
	~TestMidiDriver();
	void start();
	void stop();
private:
	TestProcessor *processor;
	QThread processorThread;
};

#endif


