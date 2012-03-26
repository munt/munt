/* Copyright (C) 2011, 2012 Jerome Fisher, Sergey V. Mikayev
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

#include "SMFDriver.h"

#include <QtCore>
#include <QFileDialog>
#include <QMessageBox>

#include "../MasterClock.h"
#include "../MidiSession.h"
#include "../MidiParser.h"

SMFProcessor::SMFProcessor(SMFDriver *useSMFDriver) : driver(useSMFDriver), stopProcessing(false) {
}

void SMFProcessor::start(QString useFileName) {
	stopProcessing = false;
	fileName = useFileName;
	QThread::start();
}

void SMFProcessor::stop() {
	stopProcessing = true;
	wait();
}

void SMFProcessor::run() {
	MidiSession *session = driver->createMidiSession(QFileInfo(fileName).fileName());
	SynthRoute *synthRoute = session->getSynthRoute();
	MidiParser parser(fileName);
	if (!parser.parse()) {
		qDebug() << "SMFDriver: Error parsing MIDI file:" << fileName;
		QMessageBox::critical(NULL, "Error", "Error encountered while loading MIDI file");
		return;
	}
	int division = parser.getDivision();
	uint bpm = 120;
	MasterClockNanos midiTick;
	if (division & 0x8000) {
		// SMPTE timebase
		uint framesPerSecond = -division >> 8;
		uint subframesPerFrame = division & 0xFF;
		midiTick = MasterClock::NANOS_PER_SECOND / (framesPerSecond * subframesPerFrame);
	} else {
		// PPQN
		midiTick = (60 * MasterClock::NANOS_PER_SECOND) / (division * bpm);
	}
	QVector<MidiEvent> midiEvents = parser.getMIDIEvents();
	MasterClockNanos currentNanos = MasterClock::getClockNanos();
	for (int i = 0; i < midiEvents.count(); i++) {
		if (stopProcessing) break;
		const MidiEvent &e = midiEvents.at(i);
		currentNanos += e.getTimestamp() * midiTick;
		if (e.getType() == SHORT_MESSAGE) {
			synthRoute->pushMIDIShortMessage(e.getShortMessage(), currentNanos);
		} else {
			synthRoute->pushMIDISysex(e.getSysexData(), e.getSysexLen(), currentNanos);
		}
		MasterClock::sleepUntilClockNanos(currentNanos);
	}
	qDebug() << "SMFDriver: processor thread stopped";
	driver->deleteMidiSession(session);
}

SMFDriver::SMFDriver(Master *useMaster) : MidiDriver(useMaster), processor(this) {
	name = "Standard MIDI File Driver";
}

void SMFDriver::start() {
	QString fileName = QFileDialog::getOpenFileName(NULL, NULL, NULL, "*.mid *.smf;;*.mid;;*.smf;;*.*");
	if (!fileName.isEmpty()) {
		processor.start(fileName);
	}
}

void SMFDriver::stop() {
	processor.stop();
}

SMFDriver::~SMFDriver() {
	stop();
}
