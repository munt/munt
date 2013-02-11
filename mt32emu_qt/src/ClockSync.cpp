/* Copyright (C) 2011, 2012, 2013 Jerome Fisher, Sergey V. Mikayev
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

ClockSync::ClockSync(double initDrift) : offsetValid(false), drift(initDrift) {
	periodicResetNanos = 5 * MasterClock::NANOS_PER_SECOND;
	periodicDampFactor = 0.1;
	emergencyResetThresholdNanos = 500 * MasterClock::NANOS_PER_MILLISECOND;
	highJitterThresholdNanos = 100 * MasterClock::NANOS_PER_MILLISECOND;
	lowJitterThresholdNanos = -100 * MasterClock::NANOS_PER_MILLISECOND;
	shiftFactor = 0.1;
}

double ClockSync::getDrift() {
	return drift;
}

void ClockSync::setThresholds(MasterClockNanos useEmergencyResetThresholdNanos,
			MasterClockNanos useHighJitterThresholdNanos, MasterClockNanos useLowJitterThresholdNanos) {
	emergencyResetThresholdNanos = useEmergencyResetThresholdNanos;
	highJitterThresholdNanos = useHighJitterThresholdNanos;
	lowJitterThresholdNanos = useLowJitterThresholdNanos;
}

void ClockSync::setParams(MasterClockNanos usePeriodicResetNanos, double usePeriodicDampFactor,
			MasterClockNanos useEmergencyResetThresholdNanos, MasterClockNanos useHighJitterThresholdNanos,
			MasterClockNanos useLowJitterThresholdNanos, double useShiftFactor) {
	periodicResetNanos = usePeriodicResetNanos;
	periodicDampFactor = usePeriodicDampFactor;
	emergencyResetThresholdNanos = useEmergencyResetThresholdNanos;
	highJitterThresholdNanos = useHighJitterThresholdNanos;
	lowJitterThresholdNanos = useLowJitterThresholdNanos;
	shiftFactor = useShiftFactor;
}

MasterClockNanos ClockSync::sync(MasterClockNanos externalNow) {
	MasterClockNanos masterNow = MasterClock::getClockNanos();
	if (externalNow == 0) {
		// Special value meaning "no timestamp, play immediately"
		return masterNow;
	}
	if (!offsetValid) {
		masterStart = masterNow;
		externalStart = externalNow;
		offset = 0;
		offsetShift = 0;
		qDebug() << "ClockSync: init:" << externalNow << masterNow << offset << drift;
		offsetValid = true;
		return masterNow;
	}
	MasterClockNanos masterElapsed = masterNow - masterStart;
	MasterClockNanos externalElapsed = externalNow - externalStart;
	MasterClockNanos offsetNow = masterElapsed - drift * externalElapsed;
	if (masterElapsed > periodicResetNanos) {
		masterStart = masterNow;
		externalStart = externalNow;
		offset -= offsetNow;
		offsetShift = 0;	// we don't want here to shift
		// we rather add a compensation for the offset we have now to the new drift value
		drift = (masterElapsed - offset * periodicDampFactor) / externalElapsed;
		qDebug() << "ClockSync: offset:" << 1e-6 * offset << "drift:" << drift;
		return masterNow + offset;
	}
	if(qAbs(offsetNow - offset) > emergencyResetThresholdNanos) {
		qDebug() << "ClockSync: emergency reset:" << externalNow << masterNow << offset << offsetNow;
		masterStart = masterNow;
		externalStart = externalNow;
		offset = 0;
		offsetShift = 0;
		drift = 1.0;
		return masterNow;
	}
	if (((offsetNow - offset) < lowJitterThresholdNanos) ||
		((offsetNow - offset) > highJitterThresholdNanos)) {
		qDebug() << "ClockSync: Latency resync offset diff:" << 1e-6 * (offsetNow - offset) << "drift:" << drift;
		// start moving offset towards 0 by steps of shiftFactor * offset
		offsetShift = MasterClockNanos(shiftFactor * (offset - offsetNow));
	}
	if (qAbs(offsetShift) > qAbs(offset - offsetNow)) {
		// resync's done
		masterStart = masterNow;
		externalStart = externalNow;
		offset = 0;
		offsetShift = 0;
		drift = 1.0;
		return masterNow;
	}
	offset -= offsetShift;
	if (offsetShift != 0) {
		qDebug() << "ClockSync: offset:" << 1e-6 * offset << "shift:" << 1e-6 * offsetShift;
	}
	return masterStart + offset + drift * externalElapsed;
}

void ClockSync::reset() {
	offsetValid = false;
}
