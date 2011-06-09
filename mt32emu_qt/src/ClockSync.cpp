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
static const qint64 HIGH_JITTER_THRESHOLD_NANOS = 100 * MasterClock::NANOS_PER_MILLISECOND;
static const qint64 LOW_JITTER_THRESHOLD_NANOS = -10 * MasterClock::NANOS_PER_MILLISECOND;
static const double SHIFT_FACTOR = 0.1;

ClockSync::ClockSync() : offsetValid(false) {
	drift = 1.0;	// we may store old drift values to make further syncs smarter
}

double ClockSync::getDrift() {
	return drift;
}

MasterClockNanos ClockSync::sync(qint64 externalNow) {
	// FIXME: Correct for clock drift.
	// FIXME: Only emergencies are handled at the moment - need to use a proper sync algorithm.

	MasterClockNanos masterNow = MasterClock::getClockNanos();
	if (externalNow == 0) {
		// Special value meaning "no timestamp, play immediately"
		return masterNow;
	}
	if (!offsetValid) {
		masterStart = masterNow;
		externalStart = externalNow;
		offset = 0;
		offsetShift =	0;
		qDebug() << "Sync:" << externalNow << masterNow << offset;
		offsetValid = true;
		return masterNow;
	}
	qint64 masterElapsed = masterNow - masterStart;
	qint64 externalElapsed = externalNow - externalStart;
	qint64 offsetNow = masterElapsed - drift * externalElapsed;
	if(qAbs(offsetNow + offset) > EMERGENCY_RESET_THRESHOLD_NANOS) {
			qDebug() << "Emergency reset:" << externalNow << masterNow << offset << offsetNow;
			offsetValid = false;
			return masterNow;
	}
	if (((offsetNow + offset) < LOW_JITTER_THRESHOLD_NANOS) ||
		 ((offsetNow + offset) > HIGH_JITTER_THRESHOLD_NANOS)) {
		qDebug() << "Latency resync:" << externalNow << masterNow << offset << offsetNow;
		drift = (double)masterElapsed / externalElapsed;
		// start moving offset towards 0 by steps of SHIFT_FACTOR * offset
		offset = offsetNow + offset;
		offsetShift =	(qint64)(SHIFT_FACTOR * offset);
		qDebug() << "Offset, shift:" << offset << offsetShift;
	}
	if (qAbs(offsetShift) > qAbs(offset)) {
		// resync's done
		qDebug() << "Latency resync's done";
		offset = 0;
		offsetShift = 0;
	}
	offset -= offsetShift;
	if (offset) qDebug() << "Offset, shift, masterNow, output:" << offset << offsetShift <<
		masterNow << qint64(masterStart + offset + drift * externalElapsed);
	return masterStart + offset + drift * externalElapsed;
}

void ClockSync::reset() {
	offsetValid = false;
}
