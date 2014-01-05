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

#include <QtGlobal>
#include <QDebug>

#include "MasterClock.h"

const MasterClock MasterClock::instance;

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

MasterClock::MasterClock() {}

MasterClock::~MasterClock() {}

#elif defined WITH_WINMMTIMER

#include <Windows.h>

void MasterClock::sleepForNanos(qint64 nanos) {
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
		return (MasterClockNanos)((counter.QuadPart - startTime.QuadPart) * mult);
	} else {
		DWORD currentTime = timeGetTime();
		if (currentTime < counter.LowPart) counter.HighPart++;
		counter.LowPart = currentTime;
		return (MasterClockNanos)(counter.QuadPart - startTime.QuadPart) * NANOS_PER_MILLISECOND;
	}
}

MasterClock::MasterClock() {
	TIMECAPS tc;
	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
		qDebug() << "Unable to get multimedia timer capabilities.";
		qDebug() << "Trying to set 1 ms multimedia timer resolution.";
		mmTimerResolution = 1;
	} else {
		qDebug() << "Found minimum supported multimedia timer resolution:" << tc.wPeriodMin << "ms.";
		qDebug() << "Setting multimedia timer resolution to" << tc.wPeriodMin << "ms.";
		mmTimerResolution = tc.wPeriodMin;
	}
	if (timeBeginPeriod(mmTimerResolution) != TIMERR_NOERROR) {
		qDebug() << "Unable to set multimedia timer resolution. Using defaults.";;
		mmTimerResolution = 0;
	}
	if (QueryPerformanceFrequency(&freq)) {
		hrTimerAvailable = true;
		qDebug() << "High resolution timer initialized. Frequency:" << freq.QuadPart * 1e-6 << "MHz";
		mult = (double)NANOS_PER_SECOND / freq.QuadPart;
		QueryPerformanceCounter(&startTime);
	} else {
		hrTimerAvailable = false;
		qDebug() << "High resolution timer unavailable on the system. Falling back to multimedia timer.";
		startTime.QuadPart = timeGetTime();
	}
}

MasterClock::~MasterClock() {
	if (mmTimerResolution != 0) {
		qDebug() << "Restoring default multimedia timer resolution.";;
		timeEndPeriod(mmTimerResolution);
	}
}

#else

#include <QThread>
#include <QElapsedTimer>

// This class exists solely to expose usleep() publicly.
// Why it's protected in QThread is a bit of a mystery...
class QtSucks : public QThread {
public:
	static void usleep(ulong usecs) {
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

static QElapsedTimer elapsedTimer;

MasterClockNanos MasterClock::getClockNanos() {
#if (QT_VERSION < QT_VERSION_CHECK(4, 8, 0))
	return (MasterClockNanos)elapsedTimer.elapsed() * NANOS_PER_MILLISECOND;
#else
	return (MasterClockNanos)elapsedTimer.nsecsElapsed();
#endif
}

MasterClock::MasterClock() {
	elapsedTimer.start();
}

MasterClock::~MasterClock() {}

#endif
