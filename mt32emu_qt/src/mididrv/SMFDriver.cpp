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

#include "../MidiSession.h"

static const MasterClockNanos MAX_SLEEP_TIME = 200 * MasterClock::NANOS_PER_MILLISECOND;
static const int DEFAULT_TEMPO = 60000000 / 120 /* bpm */;

SMFProcessor::SMFProcessor(SMFDriver *useSMFDriver) : driver(useSMFDriver), stopProcessing(false) {
}

void SMFProcessor::start(QString useFileName) {
	stopProcessing = false;
	fileName = useFileName;
	if (!parser.parse(fileName)) {
		qDebug() << "SMFDriver: Error parsing MIDI file:" << fileName;
		QMessageBox::critical(NULL, "Error", "Error encountered while loading MIDI file");
		return;
	}
	QThread::start();
}

void SMFProcessor::stop() {
	stopProcessing = true;
	wait();
}

void SMFProcessor::setTempo(uint newTempo) {
	int division = parser.getDivision();
	if (division & 0x8000) {
		// SMPTE timebase
		uint framesPerSecond = -division >> 8;
		uint subframesPerFrame = division & 0xFF;
		midiTick = MasterClock::NANOS_PER_SECOND / (framesPerSecond * subframesPerFrame);
	} else {
		// PPQN
		midiTick = newTempo * MasterClock::NANOS_PER_MICROSECOND / division;
	}
}

void SMFProcessor::run() {
	MidiSession *session = driver->createMidiSession(QFileInfo(fileName).fileName());
	SynthRoute *synthRoute = session->getSynthRoute();
	QVector<MidiEvent> midiEvents = parser.getMIDIEvents();
	setTempo(DEFAULT_TEMPO);
	MasterClockNanos currentNanos = MasterClock::getClockNanos();
	for (int i = 0; i < midiEvents.count(); i++) {
		const MidiEvent &e = midiEvents.at(i);
		currentNanos += e.getTimestamp() * midiTick;
		while (!stopProcessing) {
			MasterClockNanos delay = currentNanos - MasterClock::getClockNanos();
			if (delay < MasterClock::NANOS_PER_MILLISECOND) break;
			MasterClock::sleepForNanos((delay < MAX_SLEEP_TIME ? delay : MAX_SLEEP_TIME) - MasterClock::NANOS_PER_MILLISECOND);
		}
		if (stopProcessing) break;
		switch (e.getType()) {
			case SHORT_MESSAGE:
				synthRoute->pushMIDIShortMessage(e.getShortMessage(), currentNanos);
				break;
			case SYSEX:
				synthRoute->pushMIDISysex(e.getSysexData(), e.getSysexLen(), currentNanos);
				break;
			case SET_TEMPO:
				setTempo(e.getShortMessage());
				break;
		}
	}
	qDebug() << "SMFDriver: processor thread stopped";
	driver->deleteMidiSession(session);
}

SMFDriver::SMFDriver(Master *useMaster) : MidiDriver(useMaster), processor(this) {
	name = "Standard MIDI File Driver";
}

void SMFDriver::start() {
	static QString currentDir = NULL;
	QString fileName = QFileDialog::getOpenFileName(NULL, NULL, currentDir, "*.mid *.smf *.syx;;*.mid;;*.smf;;*.syx;;*.*");
	currentDir = QDir(fileName).absolutePath();
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
