#ifndef TEST_MIDI_DRIVER_H
#define TEST_MIDI_DRIVER_H

#include <QThread>

#include "MidiDriver.h"
#include "../Master.h"

class TestMidiDriver;

class TestProcessor : public QThread {
	Q_OBJECT
public:
	TestProcessor(TestMidiDriver *useTestMidiDriver);
	void start();
	void stop();

protected:
	void run();

private:
	TestMidiDriver *testMidiDriver;
	volatile bool stopProcessing;
};

class TestMidiDriver : public MidiDriver {
	Q_OBJECT
	friend class TestProcessor;
public:
	TestMidiDriver(Master *master);
	~TestMidiDriver();
	void start();
	void stop();
private:
	TestProcessor processor;
};

#endif


