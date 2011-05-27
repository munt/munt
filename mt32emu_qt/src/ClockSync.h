#ifndef CLOCK_SYNC_H
#define CLOCK_SYNC_H

#include <QtGlobal>

#include "MasterClock.h"

class ClockSync {
private:
	bool offsetValid;
	// Multiplier for externalNanos
	double skew;
	// Offset to be added to externalNanos to get refNanos;
	qint64 offset;
	qint64 refNanosStart;
	qint64 externalNanosStart;

public:
	ClockSync();

	double getSkew();
	MasterClockNanos sync(qint64 externalNanos);
	void reset();
};

#endif
