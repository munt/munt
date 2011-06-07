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
static const qint64 EMERGENCY_RESYNC_THRESHOLD_NANOS = 500 * MasterClock::NANOS_PER_MILLISECOND;

ClockSync::ClockSync() : offsetValid(false) {
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
	qint64 nanosFromStart = masterClockNow - refNanosStart;
	qint64 externalNanosFromStart = externalNanos - externalNanosStart;
	qint64 offsetNow = nanosFromStart - drift * externalNanosFromStart;
	if (!offsetValid) {
		refNanosStart = masterClockNow;
		externalNanosStart = externalNanos;
		externalNanosFromStart = 0.0;
		offset = 0.0;
		drift = 1.0;
		qDebug() << "Sync:" << externalNanos << masterClockNow << offset;
		offsetValid = true;
	} else if (offsetNow < offset) {
		qDebug() << "Latency resync:" << externalNanos << masterClockNow << offset << offsetNow;
		drift = (double)nanosFromStart / externalNanosFromStart;
		offset = nanosFromStart - drift * externalNanosFromStart;
	} else {
		if(qAbs(offsetNow - offset) > EMERGENCY_RESYNC_THRESHOLD_NANOS) {
			qDebug() << "Emergency resync:" << externalNanos << masterClockNow << offset << offsetNow;
			drift = (double)nanosFromStart / externalNanosFromStart;
			offset = nanosFromStart - drift * externalNanosFromStart;
		}
	}
	return refNanosStart + offset + drift * externalNanosFromStart;
}

void ClockSync::reset() {
	offsetValid = false;
}
