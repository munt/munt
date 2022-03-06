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

#include "TestDriver.h"

#include <QtCore>

#include "../MasterClock.h"
#include "../MidiSession.h"

static const qint64 TEST1_EVENT_INTERVAL_NANOS = 8000000; // 256 samples;

TestProcessor::TestProcessor(TestMidiDriver *useTestMidiDriver) : testMidiDriver(useTestMidiDriver), stopProcessing(false) {
}

void TestProcessor::start() {
	stopProcessing = false;
	QThread::start(QThread::TimeCriticalPriority);
}

void TestProcessor::stop() {
	stopProcessing = true;
}

void TestProcessor::run() {
	MidiSession *session1 = testMidiDriver->createMidiSession("Test 1");
	MidiSession *session2 = NULL;//testMidiDriver->createMidiSession("Test 2");
	qint64 currentNanos = MasterClock::getClockNanos();
	qDebug() << currentNanos;
	bool alt = false;
	while (!stopProcessing) {
		session1->getSynthRoute()->pushMIDIShortMessage(*session1, 0, currentNanos);
		// Test 2 sends an event at the same time as every second Test 1 event
		if(alt && session2 != NULL)
			session2->getSynthRoute()->pushMIDIShortMessage(*session2, 0, currentNanos);
		alt = !alt;
		currentNanos += TEST1_EVENT_INTERVAL_NANOS;
		MasterClock::sleepUntilClockNanos(currentNanos);
	}
	qDebug() << "Test processor finished";
	testMidiDriver->deleteMidiSession(session1);
	if (session2 != NULL) {
		testMidiDriver->deleteMidiSession(session2);
	}
}


TestMidiDriver::TestMidiDriver(Master *useMaster) : MidiDriver(useMaster), processor(this) {
	name = "Test Driver";
}

void TestMidiDriver::start() {
	processor.start();
}

void TestMidiDriver::stop() {
	processor.stop();
	MidiDriver::waitForProcessingThread(processor, TEST1_EVENT_INTERVAL_NANOS);
}

TestMidiDriver::~TestMidiDriver() {
	stop();
}
