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
	MasterClockNanos offset;
	// While resyncing, we move offset towards 0 by offsetShift steps
	MasterClockNanos offsetShift;
	// Start is the moment when we start time measurement
	// (the first call to sync() after either init or reset)
	MasterClockNanos masterStart, externalStart;
	// Time interval for measuring the current drift
	MasterClockNanos periodicResetNanos;
	// The part of the external clock's offset to compensate when computing new drift
	double periodicDampFactor;
	// The maximum acceptable external clock's offset which can be absorbed by buffers (should be <= MIDI latency)
	MasterClockNanos emergencyResetThresholdNanos;
	// For offset shifting algorithm, the highest value of the external clock's offset to be shifted down to 0
	MasterClockNanos highJitterThresholdNanos;
	// For offset shifting algorithm, the lowest value of the external clock's offset to be shifted up to 0
	MasterClockNanos lowJitterThresholdNanos;
	// For offset shifting algorithm, the part of the external clock's offset to use as a single shift step
	double shiftFactor;

public:
	ClockSync(double initDrift = 1.0);

	double getDrift();
	MasterClockNanos sync(MasterClockNanos externalNanos);
	void reset();
	void setThresholds(MasterClockNanos useEmergencyResetThresholdNanos, MasterClockNanos useHighJitterThresholdNanos,
		MasterClockNanos useLowJitterThresholdNanos);
	void setParams(MasterClockNanos usePeriodicResetNanos, double usePeriodicDampFactor,
		MasterClockNanos useEmergencyResetThresholdNanos, MasterClockNanos useHighJitterThresholdNanos,
		MasterClockNanos useLowJitterThresholdNanos, double useShiftFactor);
};

#endif
