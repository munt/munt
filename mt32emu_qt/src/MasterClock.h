#ifndef MASTER_CLOCK_H
#define MASTER_CLOCK_H

#include <QtGlobal>

typedef qint64 MasterClockNanos;

class MasterClock {
private:
	static const MasterClock instance;

	MasterClock();
	~MasterClock();

public:
	static const qint64 NANOS_PER_SECOND = 1000000000;
	static const qint64 NANOS_PER_MILLISECOND = 1000000;
	static const qint64 NANOS_PER_MICROSECOND = 1000;
	static const qint64 MICROS_PER_SECOND = 1000000;
	static const qint64 MICROS_PER_MILLISECOND = 1000;
	static const qint64 MILLIS_PER_SECOND = 1000;
	static void sleepForNanos(MasterClockNanos nanos);
	static void sleepUntilClockNanos(MasterClockNanos clockNanos);
	static MasterClockNanos getClockNanos();
};

#endif
