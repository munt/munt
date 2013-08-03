#ifndef SMF_DRIVER_H
#define SMF_DRIVER_H

#include <QThread>

#include "MidiDriver.h"
#include "../Master.h"
#include "../MidiParser.h"
#include "../MasterClock.h"

class SMFDriver;

class SMFProcessor : public QThread {
	Q_OBJECT

public:
	SMFProcessor(SMFDriver *useSMFDriver);
	void start(QString fileName);
	void stop();
	void setBPM(quint32 newBPM);

protected:
	void run();

private:
	MidiParser parser;
	SMFDriver *driver;
	volatile bool stopProcessing;
	volatile MasterClockNanos midiTick;
	volatile bool bpmUpdated;
	QString fileName;

	static void sendChannelsReset(SynthRoute *synthRoute);
	quint32 estimateRemainingTime(const QMidiEventList &midiEvents, int currentEventIx);
	int seek(SynthRoute *synthRoute, const QMidiEventList &midiEvents, int currentEventIx, MasterClockNanos seekNanos, MasterClockNanos currentEventNanos);
};

class SMFDriver : public MidiDriver {
	Q_OBJECT
	friend class SMFProcessor;

public:
	SMFDriver(Master *master);
	~SMFDriver();
	void start();
	void start(QString fileName);
	void stop();
	void setBPM(quint32 newBPM);
	void setFastForwardingFactor(uint useFastForwardingFactor);
	void jump(int newPosition);

private:
	SMFProcessor processor;
	volatile uint fastForwardingFactor;
	volatile int seekPosition;

signals:
	void playbackFinished();
	void playbackTimeChanged(quint64 currentNanos, quint32 totalSeconds);
	void tempoUpdated(quint32 newTempo);
};

#endif
