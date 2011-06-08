/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QDebug>

#include "ClockSync.h"
#include "MasterClock.h"

// This value defines when the difference between our current idea of the offset is so bad that we just start
// using the new offset immediately.
// This can be regularly hit around stream start time by badly-behaved drivers that return bogus timestamps
// for a while when first starting.
static const qint64 EMERGENCY_RESET_THRESHOLD_NANOS = 500 * MasterClock::NANOS_PER_MILLISECOND;
static const qint64 JITTER_THRESHOLD_NANOS = 100 * MasterClock::NANOS_PER_MILLISECOND;
static const double LATENCY_FACTOR = 0.1;
static const qint64 SYNC_TIME_NANOS = 3000 * MasterClock::NANOS_PER_MILLISECOND;

ClockSync::ClockSync() : offsetValid(false) {
	drift = 1.0;	// we may store old drift values to make further syncs smarter
}

double ClockSync::getDrift() {
	return drift;
}

MasterClockNanos ClockSync::sync(qint64 externalNanos) {
	// FIXME: Correct for clock drift.
	// FIXME: Only emergencies are handled at the moment - need to use a proper sync algorithm.

	MasterClockNanos masterClockNow = MasterClock::getClockNanos();
	if (externalNanos == 0) {
		// Special value meaning "no timestamp, play immediately"
		return masterClockNow;
	}
	if (!offsetValid) {
		refNanosStart = masterClockNow;
		externalNanosStart = externalNanos;
		offset = 0;
		qDebug() << "Sync:" << externalNanos << masterClockNow << offset;
		offsetValid = true;
		synced = false;
		return masterClockNow;
	}
	qint64 nanosFromStart = masterClockNow - refNanosStart;
	qint64 externalNanosFromStart = externalNanos - externalNanosStart;
	qint64 offsetNow = nanosFromStart - drift * externalNanosFromStart;
	if (!synced) {
		if (nanosFromStart < SYNC_TIME_NANOS) {
			return masterClockNow - offsetNow;
		} else {
			// Special value meaning "sync immediately"
			offset = 0;
			offsetNow = -1;
			synced = true;
		}
	}
	if(qAbs(offsetNow - offset) > EMERGENCY_RESET_THRESHOLD_NANOS) {
			qDebug() << "Emergency reset:" << externalNanos << masterClockNow << offset << offsetNow;
			offsetValid = false;
			return masterClockNow;
	}
	if ((offsetNow < offset) || ((offsetNow - offset) > JITTER_THRESHOLD_NANOS)) {
		qDebug() << "Latency resync:" << externalNanos << masterClockNow << offset << offsetNow;
		drift = (double)nanosFromStart / externalNanosFromStart;
		offset = -LATENCY_FACTOR * JITTER_THRESHOLD_NANOS;
	}
	return refNanosStart + offset + drift * externalNanosFromStart;
}

void ClockSync::reset() {
	offsetValid = false;
}
