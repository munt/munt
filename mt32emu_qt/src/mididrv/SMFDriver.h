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

private:
	MidiParser parser;
	SMFDriver *driver;
	MasterClockNanos midiTick;
	QString fileName;

	void run();
	quint32 estimateRemainingTime(const QMidiEventList &midiEvents, int currentEventIx);
	void seek(SynthRoute *synthRoute, const QMidiEventList &midiEvents, int &currentEventIx, MasterClockNanos &currentEventNanos, const MasterClockNanos seekNanos);
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
	void pause(bool paused);
	void setBPM(quint32 newBPM);
	void setFastForwardingFactor(uint useFastForwardingFactor);
	void jump(int newPosition);

private:
	SMFProcessor processor;
	volatile bool stopProcessing;
	volatile bool pauseProcessing;
	QAtomicInt bpmUpdate;
	volatile uint fastForwardingFactor;
	QAtomicInt seekPosition;

signals:
	void playbackFinished();
	void playbackTimeChanged(quint64 currentNanos, quint32 totalSeconds);
	void tempoUpdated(quint32 newTempo);
};

#endif
