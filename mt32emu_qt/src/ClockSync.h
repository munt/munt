#ifndef CLOCK_SYNC_H
#define CLOCK_SYNC_H

#include <QtGlobal>

#include "MasterClock.h"

class ClockSync {
private:
	bool offsetValid;
	// Multiplier for externalElapsed
	double drift;
	// offset is the difference between the master clock and the scaled external clock
	qint64 offset;
	// Start is the moment when we start time measurement
	// (the first call to sync() after either init or reset)
	qint64 masterStart;
	qint64 externalStart;

public:
	ClockSync(double initDrift = 1.0);

	double getDrift();
	MasterClockNanos sync(qint64 externalNanos);
	void reset();
};

#endif
