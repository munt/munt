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

#include "TestDriver.h"

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <QtCore>

#include <time.h>

#include "../SynthRoute.h"

static const qint64 TEST1_EVENT_INTERVAL_NANOS = 8000000; // 256 samples;

static const qint64 NANOS_PER_SECOND = 1000000000;

class QtSucks : public QThread {
public:
	static void usleep(int usecs) {
		QThread::usleep(usecs);
	}
};

#if _POSIX_C_SOURCE >= 199309L

static qint64 timespecToNanos(const timespec &ts) {
	return ts.tv_sec * (qint64)NANOS_PER_SECOND + ts.tv_nsec;
}

static void nanosToTimespec(timespec &ts, qint64 nanos) {
	ts.tv_sec = nanos / NANOS_PER_SECOND;
	ts.tv_nsec = nanos % NANOS_PER_SECOND;
}

static qint64 getCurrentNanos() {
	timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 0;
	// NOTE: I would use CLOCK_MONOTONIC_RAW, but clock_nanosleep() and clock_getres() are broken with that clock on my system.
	// nanosleep() works with it, however.
	if (clock_getres(CLOCK_MONOTONIC, &ts) == 0) {
		qDebug() << "Timer resolution: " << ts.tv_sec << "s " << ts.tv_nsec << " us";
	}
	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
		qDebug() << "Monotonic clock is broken, returning 0";
		return 0;
	}
	return timespecToNanos(ts);
}

static bool sleepUntilNanos(qint64 targetNanos) {
	timespec ts;
	nanosToTimespec(ts, targetNanos);
	if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL) != 0) {
		qDebug() << errno;
		return false;
	}
	return true;
}

#else

static qint64 getCurrentNanos() {
	return (qint64)clock() * NANOS_PER_SECOND / CLOCKS_PER_SEC;
}

static bool sleepUntilNanos(qint64 targetNanos) {
	qint64 deltaMicros = (targetNanos - getCurrentNanos()) / 1000;
	if (deltaMicros <= 0) {
		return true;
	}
	QtSucks::usleep(deltaMicros);
	return true;
}

#endif

TestProcessor::TestProcessor(TestMidiDriver *useTestMidiDriver) : testMidiDriver(useTestMidiDriver), stopProcessing(false) {
}

void TestProcessor::stop() {
	stopProcessing = true;
}

void TestProcessor::processSeqEvents() {
	Master *master = testMidiDriver->getMaster();
	MidiSession *session1 = master->createMidiSession(testMidiDriver, "Test 1");
	MidiSession *session2 = master->createMidiSession(testMidiDriver, "Test 2");
	qint64 currentNanos = getCurrentNanos();
	qDebug() << currentNanos;
	bool alt = false;
	while(!stopProcessing) {
		session1->getSynthRoute()->pushMIDIShortMessage(0, currentNanos);
		// Test 2 sends an event at the same time as every second Test 1 event
		if(alt)
			session2->getSynthRoute()->pushMIDIShortMessage(0, currentNanos);
		alt = !alt;
		currentNanos += TEST1_EVENT_INTERVAL_NANOS;
		if (!sleepUntilNanos(currentNanos)) {
			break;
		}
	}
	qDebug() << "Test processor finished";
	master->deleteMidiSession(session1);
	master->deleteMidiSession(session2);
	emit finished();
}


TestMidiDriver::TestMidiDriver(Master *useMaster) : MidiDriver(useMaster), processor(NULL) {
	setName("Test Driver");
}

void TestMidiDriver::start() {
	processor = new TestProcessor(this);
	processor->moveToThread(&processorThread);
	// Yes, seriously. The QThread object's default thread is *this* thread,
	// We move it to the thread that it represents so that the finished()->quit() connection
	// will happen asynchronously and avoid a deadlock in the destructor.
	processorThread.moveToThread(&processorThread);

	// Start the processor once the thread has started
	processor->connect(&processorThread, SIGNAL(started()), SLOT(processSeqEvents()));
	// Stop the thread once the processor has finished
	processorThread.connect(processor, SIGNAL(finished()), SLOT(quit()));

	processorThread.start();
}

void TestMidiDriver::stop() {
	if (processor != NULL) {
		if (processorThread.isRunning()) {
			processor->stop();
			processorThread.wait();
		}
		delete processor;
		processor = NULL;
	}
}

TestMidiDriver::~TestMidiDriver() {
	stop();
}
