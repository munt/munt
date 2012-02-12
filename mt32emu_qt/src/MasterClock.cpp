/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
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

#include <QtGlobal>
#include <QDebug>

#include "MasterClock.h"

#if _POSIX_C_SOURCE >= 199309L

#include <cerrno>
#include <time.h>

static qint64 timespecToNanos(const timespec &ts) {
	return ts.tv_sec * (qint64)MasterClock::NANOS_PER_SECOND + ts.tv_nsec;
}

static void nanosToTimespec(timespec &ts, qint64 nanos) {
	ts.tv_sec = nanos / MasterClock::NANOS_PER_SECOND;
	ts.tv_nsec = nanos % MasterClock::NANOS_PER_SECOND;
}

void MasterClock::sleepForNanos(qint64 nanos) {
	timespec ts;
	nanosToTimespec(ts, nanos);
	if (clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL) != 0) {
		qDebug() << errno;
	}
}

void MasterClock::sleepUntilClockNanos(MasterClockNanos clockNanos) {
	timespec ts;
	nanosToTimespec(ts, clockNanos);
	if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL) != 0) {
		qDebug() << errno;
	}
}

MasterClockNanos MasterClock::getClockNanos() {
	timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 0;
	// NOTE: I would use CLOCK_MONOTONIC_RAW, but several things (e.g. clock_getres()) are broken with that clock on my system.
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
		qDebug() << "Monotonic clock is broken, returning 0";
		return 0;
	}
	return timespecToNanos(ts);
}

void MasterClock::init() {
}

void MasterClock::deinit() {
}

#else

#include <QThread>

// This class exists solely to expose usleep() publicly.
// Why it's protected in QThread is a bit of a mystery...
class QtSucks : public QThread {
public:
	static void usleep(int usecs) {
		QThread::usleep(usecs);
	}
};

void MasterClock::sleepForNanos(qint64 nanos) {
	if (nanos <= 0) {
		return;
	}
	QtSucks::usleep(nanos / NANOS_PER_MICROSECOND);
}

void MasterClock::sleepUntilClockNanos(MasterClockNanos clockNanos) {
	sleepForNanos(clockNanos - getClockNanos());
}

#ifdef WITH_WINMMTIMER

#include <Windows.h>

static DWORD startTime = 0;

MasterClockNanos MasterClock::getClockNanos() {
	// FIXME: Deal with wrapping
	return (MasterClockNanos)(timeGetTime() - startTime) * NANOS_PER_MILLISECOND;
}

void MasterClock::init() {
	startTime = timeGetTime();
}

void MasterClock::deinit() {
}

#else

#include <QElapsedTimer>

static QElapsedTimer *elapsedTimer = NULL;

MasterClockNanos MasterClock::getClockNanos() {
	return (MasterClockNanos)elapsedTimer->elapsed() * NANOS_PER_MILLISECOND;
}

void MasterClock::init() {
	elapsedTimer = new QElapsedTimer();
	elapsedTimer->start();
}

void MasterClock::deinit() {
	delete elapsedTimer;
	elapsedTimer = NULL;
}

#endif
#endif
