/* Copyright (C) 2011, 2012, 2013, 2014 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QDebug>

#include "ClockSync.h"
#include "MasterClock.h"

ClockSync::ClockSync() : performResetOnNextSync(true) {
	periodicResetNanos = 2 * MasterClock::NANOS_PER_SECOND;
	emergencyResetThresholdNanos = 200 * MasterClock::NANOS_PER_MILLISECOND;
}

void ClockSync::scheduleReset() {
	performResetOnNextSync = true;
}

double ClockSync::getDrift() {
	return 1.0 + drift;
}

void ClockSync::setParams(MasterClockNanos useEmergencyResetThresholdNanos, MasterClockNanos usePeriodicResetNanos) {
	periodicResetNanos = usePeriodicResetNanos;
	emergencyResetThresholdNanos = useEmergencyResetThresholdNanos;
}

MasterClockNanos ClockSync::sync(MasterClockNanos masterNow, MasterClockNanos externalNow) {
	if (performResetOnNextSync) {
		masterStart = masterNow;
		externalStart = externalNow;
		baseOffset = externalNow - masterNow;
		offsetSum = 0;
		syncCount = 0;
		drift = 0;
		qDebug() << "ClockSync: reset" << externalNow * 1e-6 << masterNow * 1e-6 << baseOffset * 1e-6 << getDrift();
		performResetOnNextSync = false;
		return masterNow;
	}
	MasterClockNanos masterElapsed = masterNow - masterStart;
	MasterClockNanos externalElapsed = externalNow - externalStart;

	// Use deltas in average offset calculation to avoid overflow
	MasterClockNanos deltaOffset = (externalNow - masterNow) - baseOffset;
	offsetSum += deltaOffset;
	++syncCount;
	MasterClockNanos currentOffset = baseOffset + MasterClockNanos(externalElapsed * drift);
	if (emergencyResetThresholdNanos < qAbs(deltaOffset)) {
		scheduleReset();
	}
	if (periodicResetNanos < masterElapsed) {
		MasterClockNanos newBaseOffset = baseOffset + offsetSum / syncCount;
		drift = (newBaseOffset - currentOffset) / (double)periodicResetNanos;
		masterStart = masterNow;
		externalStart = externalNow;
		baseOffset = currentOffset;
		offsetSum = 0;
		syncCount = 0;
#if 0
		qDebug() << "ClockSync: offset delta" << (newBaseOffset - baseOffset) * 1e-6 << "drift" << getDrift();
#endif
	}
	return externalNow - currentOffset;
}
