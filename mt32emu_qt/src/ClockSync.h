#ifndef CLOCK_SYNC_H
#define CLOCK_SYNC_H

#include <QtGlobal>

#include "MasterClock.h"

class ClockSync {
private:
	bool performResetOnNextSync;

	// External to master clock rate
	double drift;

	// The average difference between the external clock and the master clock
	MasterClockNanos baseOffset;

	// Variables below intended to find average clock offset to reduce jitter and make drift smoother
	MasterClockNanos offsetSum;
	uint syncCount;

	// Start is the moment when we start time measurement
	// (the first call to sync() after either init or reset)
	MasterClockNanos masterStart, externalStart;

	// Time interval for measuring the current drift
	MasterClockNanos periodicResetNanos;

	// When the clock offset exceeds this threshold, a reset is scheduled upon the next sync
	// The reset isn't performed immediately to avoid syncing on a random fluctuation
	MasterClockNanos emergencyResetThresholdNanos;

public:
	ClockSync();

	void scheduleReset();
	MasterClockNanos sync(MasterClockNanos masterNow, MasterClockNanos externalNanos);
	double getDrift();

	void setParams(MasterClockNanos useEmergencyResetThresholdNanos, MasterClockNanos usePeriodicResetNanos);
};

#endif
