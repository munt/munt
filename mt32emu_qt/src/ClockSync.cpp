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

ClockSync::ClockSync() : performResetOnNextSync(true) {
	periodicResetNanos = 2 * MasterClock::NANOS_PER_SECOND;
	emergencyResetThresholdNanos = 200 * MasterClock::NANOS_PER_MILLISECOND;
}

void ClockSync::scheduleReset() {
	performResetOnNextSync = true;
}

double ClockSync::getDrift() {
	return drift;
}

void ClockSync::setParams(MasterClockNanos useEmergencyResetThresholdNanos, MasterClockNanos usePeriodicResetNanos) {
	periodicResetNanos = usePeriodicResetNanos;
	emergencyResetThresholdNanos = useEmergencyResetThresholdNanos;
}

MasterClockNanos ClockSync::sync(MasterClockNanos masterNow, MasterClockNanos externalNow) {
	if (performResetOnNextSync) {
		masterStart = masterNow;
		externalStart = externalNow;
		baseOffset = 0;
		offsetSum = 0;
		syncCount = 0;
		drift = rdrift = 1;
		qDebug() << "ClockSync: init:" << externalNow << masterNow << baseOffset << drift;
		performResetOnNextSync = false;
		return masterNow;
	}
	MasterClockNanos masterElapsed = masterNow - masterStart;
	MasterClockNanos externalElapsed = externalNow - externalStart;
	MasterClockNanos offsetNow = rdrift * externalElapsed - masterElapsed;
	offsetSum += offsetNow;
	syncCount++;
	if (qAbs(offsetNow) > emergencyResetThresholdNanos) {
		qDebug() << "ClockSync: Synchronisation lost:" << externalNow << masterNow << baseOffset << drift << "reset scheduled";
		scheduleReset();
	}
	if (masterElapsed > periodicResetNanos) {
		double offsetAverage = offsetSum / (double)syncCount;
		drift = (externalElapsed + 0.5 * (baseOffset + offsetAverage)) / (double)masterElapsed;
		rdrift = 1 / drift;

		masterStart = masterNow;
		externalStart = externalNow;
		// Ensure the clock won't jump
		baseOffset += offsetNow;
		offsetSum = 0;
		syncCount = 0;
		offsetAverage = 0;

		qDebug() << "ClockSync: baseOffset:" << 1e-6 * baseOffset << "drift:" << drift;
		return masterNow + baseOffset;
	}
	return masterStart + baseOffset + rdrift * externalElapsed;
}
