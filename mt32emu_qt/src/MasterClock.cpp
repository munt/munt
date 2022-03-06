/* Copyright (C) 2011-2022 Jerome Fisher, Sergey V. Mikayev
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

#if defined WITH_POSIX_CLOCK_NANOSLEEP

#if !(defined _GNU_SOURCE || defined _POSIX_C_SOURCE && (_POSIX_C_SOURCE - 0) >= 200112L)
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include <time.h>
#include <cerrno>

#endif

#include <QtGlobal>
#include <QDebug>

#include "MasterClock.h"

#if defined WITH_POSIX_CLOCK_NANOSLEEP

static qint64 timespecToNanos(const timespec &ts) {
	return ts.tv_sec * (qint64)MasterClock::NANOS_PER_SECOND + ts.tv_nsec;
}

static void nanosToTimespec(timespec &ts, qint64 nanos) {
	ts.tv_sec = nanos / MasterClock::NANOS_PER_SECOND;
	ts.tv_nsec = nanos % MasterClock::NANOS_PER_SECOND;
}

void MasterClock::sleepForNanos(MasterClockNanos nanos) {
	timespec ts;
	nanosToTimespec(ts, nanos);
	if (clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL) != 0) {
		qDebug() << "MasterClock: clock_nanosleep failed:" << errno;
	}
}

void MasterClock::sleepUntilClockNanos(MasterClockNanos clockNanos) {
	timespec ts;
	nanosToTimespec(ts, clockNanos);
	if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL) != 0) {
		qDebug() << "MasterClock: clock_nanosleep failed:" << errno;
	}
}

MasterClockNanos MasterClock::getClockNanos() {
	timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 0;
	// NOTE: I would use CLOCK_MONOTONIC_RAW, but several things (e.g. clock_getres()) are broken with that clock on my system.
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
		qDebug() << "MasterClock: Monotonic clock is broken, returning 0";
		return 0;
	}
	return timespecToNanos(ts);
}

void MasterClock::init() {
	timespec ts;
	if (clock_getres(CLOCK_MONOTONIC, &ts) != 0) {
		qDebug() << "MasterClock: Monotonic clock is broken:" << errno;
		return;
	}
	qDebug() << "MasterClock: Using POSIX monotonic clock. Found clock resolution:" << timespecToNanos(ts) << "nanos.";
}

void MasterClock::cleanup() {}

#elif defined WITH_WINMMTIMER

#include <Windows.h>

void MasterClock::sleepForNanos(MasterClockNanos nanos) {
	Sleep(DWORD(qMax(1LL, nanos / NANOS_PER_MILLISECOND)));
}

void MasterClock::sleepUntilClockNanos(MasterClockNanos clockNanos) {
	sleepForNanos(clockNanos - getClockNanos());
}

static bool hrTimerAvailable;

static LARGE_INTEGER freq = {{0, 0}};
static double mult;

static LARGE_INTEGER startTime = {{0, 0}};
static LARGE_INTEGER counter = {{0, 0}};

static DWORD mmTimerResolution = 0;

MasterClockNanos MasterClock::getClockNanos() {
	if (hrTimerAvailable) {
		QueryPerformanceCounter(&counter);
		return MasterClockNanos((counter.QuadPart - startTime.QuadPart) * mult);
	} else {
		DWORD currentTime = timeGetTime();
		LARGE_INTEGER counterSnapshot = counter;
		if (currentTime < counterSnapshot.LowPart) counterSnapshot.HighPart++;
		counterSnapshot.LowPart = currentTime;
		counter = counterSnapshot;
		return MasterClockNanos(counterSnapshot.QuadPart - startTime.QuadPart) * NANOS_PER_MILLISECOND;
	}
}

void MasterClock::init() {
	TIMECAPS tc;
	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
		qDebug() << "MasterClock: Unable to get multimedia timer capabilities.";
		qDebug() << "MasterClock: Trying to set 1 ms multimedia timer resolution.";
		mmTimerResolution = 1;
	} else {
		qDebug() << "MasterClock: Found minimum supported multimedia timer resolution:" << tc.wPeriodMin << "ms.";
		qDebug() << "MasterClock: Setting multimedia timer resolution to" << tc.wPeriodMin << "ms.";
		mmTimerResolution = tc.wPeriodMin;
	}
	if (timeBeginPeriod(mmTimerResolution) != TIMERR_NOERROR) {
		qDebug() << "MasterClock: Unable to set multimedia timer resolution. Using defaults.";;
		mmTimerResolution = 0;
	}
	if (QueryPerformanceFrequency(&freq)) {
		hrTimerAvailable = true;
		qDebug() << "MasterClock: High resolution timer initialized. Frequency:" << freq.QuadPart * 1e-6 << "MHz";
		mult = (double)NANOS_PER_SECOND / freq.QuadPart;
		QueryPerformanceCounter(&startTime);
	} else {
		hrTimerAvailable = false;
		qDebug() << "MasterClock: High resolution timer unavailable on the system. Falling back to multimedia timer.";
		startTime.QuadPart = timeGetTime();
	}
}

void MasterClock::cleanup() {
	if (mmTimerResolution != 0) {
		qDebug() << "MasterClock: Restoring default multimedia timer resolution";
		timeEndPeriod(mmTimerResolution);
	}
}

#else // defined WITH_POSIX_CLOCK_NANOSLEEP || defined WITH_WINMMTIMER

#include <QThread>

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))

// This class exists solely to expose usleep() publicly.
// Why it's protected in QThread is a bit of a mystery...
class QtSucks : public QThread {
public:
	static void usleep(ulong usecs) {
		QThread::usleep(usecs);
	}
};

void MasterClock::sleepForNanos(MasterClockNanos nanos) {
	if (nanos <= 0) {
		return;
	}
	QtSucks::usleep(nanos / NANOS_PER_MICROSECOND);
}

#else // (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))

void MasterClock::sleepForNanos(MasterClockNanos nanos) {
	if (nanos <= 0) {
		return;
	}
	QThread::usleep(nanos / NANOS_PER_MICROSECOND);
}

#endif // (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))

void MasterClock::sleepUntilClockNanos(MasterClockNanos clockNanos) {
	sleepForNanos(clockNanos - getClockNanos());
}

#if (QT_VERSION < QT_VERSION_CHECK(4, 7, 0))

#include <QDateTime>

static QDateTime startTime;

MasterClockNanos MasterClock::getClockNanos() {
	static MasterClockNanos MSECS_PER_DAY = 86400000LL;

	QDateTime systemTime = QDateTime::currentDateTime().toUTC();
	MasterClockNanos elapsedMillis = startTime.daysTo(systemTime) * MSECS_PER_DAY + startTime.time().msecsTo(systemTime.time());
	return MasterClockNanos(elapsedMillis * NANOS_PER_MILLISECOND);
}

void MasterClock::init() {
	startTime = QDateTime::currentDateTime().toUTC();
	qDebug() << "MasterClock: Using system time source";
}

void MasterClock::cleanup() {}

#else // (QT_VERSION < QT_VERSION_CHECK(4, 7, 0))

#include <QElapsedTimer>

static QElapsedTimer elapsedTimer;

MasterClockNanos MasterClock::getClockNanos() {
#if (QT_VERSION < QT_VERSION_CHECK(4, 8, 0))
	return (MasterClockNanos)elapsedTimer.elapsed() * NANOS_PER_MILLISECOND;
#else
	return (MasterClockNanos)elapsedTimer.nsecsElapsed();
#endif
}

void MasterClock::init() {
	elapsedTimer.start();
	static const char *clockTypes[] = { "SystemTime", "MonotonicClock", "TickCounter", "MachAbsoluteTime", "PerformanceCounter" };
	qDebug() << "MasterClock: Initialised QElapsedTimer, clockType:" << clockTypes[elapsedTimer.clockType()];
}

void MasterClock::cleanup() {
	qDebug() << "MasterClock: Invalidating QElapsedTimer";
	elapsedTimer.invalidate();
}

#endif // (QT_VERSION < QT_VERSION_CHECK(4, 7, 0))

#endif // defined WITH_POSIX_CLOCK_NANOSLEEP || defined WITH_WINMMTIMER
