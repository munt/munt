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

void SMFProcessor::sendAllNotesOff(SynthRoute *synthRoute) {
	MasterClockNanos nanosNow = MasterClock::getClockNanos();
	for (int i = 0; i < 16; i++) {
		quint32 msg = (0xB0 | i) | 0x7F00;
		synthRoute->pushMIDIShortMessage(msg, nanosNow);
	}
}

SMFProcessor::SMFProcessor(SMFDriver *useSMFDriver) : driver(useSMFDriver) {
}

void SMFProcessor::start(QString useFileName) {
	stop();
	stopProcessing = false;
	bpmUpdated = false;
	driver->seekPosition = -1;
	driver->fastForwardingFactor = 0;
	fileName = useFileName;
	if (!parser.parse(fileName)) {
		qDebug() << "SMFDriver: Error parsing MIDI file:" << fileName;
		QMessageBox::warning(NULL, "Error", "Error encountered while loading MIDI file");
		emit driver->playbackFinished();
		return;
	}
	QThread::start();
}

void SMFProcessor::stop() {
	stopProcessing = true;
	wait();
}

void SMFProcessor::setBPM(quint32 newBPM) {
	midiTick = parser.getMidiTick(MidiParser::MICROSECONDS_PER_MINUTE / newBPM);
	bpmUpdated = true;
}

void SMFProcessor::run() {
	MidiSession *session = driver->createMidiSession(QFileInfo(fileName).fileName());
	SynthRoute *synthRoute = session->getSynthRoute();
	const MidiEventList &midiEvents = parser.getMIDIEvents();
	midiTick = parser.getMidiTick();
	quint32 totalSeconds = estimateRemainingTime(midiEvents, 0);
	MasterClockNanos startNanos = MasterClock::getClockNanos();
	MasterClockNanos currentNanos = startNanos;
	for (int i = 0; i < midiEvents.count(); i++) {
		const MidiEvent &e = midiEvents.at(i);
		currentNanos += e.getTimestamp() * midiTick;
		while (!stopProcessing && synthRoute->getState() == SynthRouteState_OPEN) {
			if (bpmUpdated) {
				bpmUpdated = false;
				totalSeconds = (currentNanos - startNanos) / MasterClock::NANOS_PER_SECOND + estimateRemainingTime(midiEvents, i + 1);
			}
			if (driver->seekPosition > -1) {
				SMFProcessor::sendAllNotesOff(synthRoute);
				MasterClockNanos seekNanos = totalSeconds * driver->seekPosition * MasterClock::NANOS_PER_MILLISECOND;
				MasterClockNanos eventNanos = currentNanos - e.getTimestamp() * midiTick - startNanos;
				if (seekNanos < eventNanos) {
					i = 0;
					eventNanos = 0;
					midiTick = parser.getMidiTick();
					emit driver->tempoUpdated(0);
				}
				i = seek(synthRoute, midiEvents, i, seekNanos, eventNanos) - 1;
				currentNanos = MasterClock::getClockNanos();
				startNanos = currentNanos - seekNanos;
				break;
			}
			emit driver->playbackTimeChanged(MasterClock::getClockNanos() - startNanos, totalSeconds);
			MasterClockNanos delay = currentNanos - MasterClock::getClockNanos();
			if (driver->fastForwardingFactor > 1) {
				MasterClockNanos timeShift = delay - (delay / driver->fastForwardingFactor);
				delay -= timeShift;
				currentNanos -= timeShift;
				startNanos -= timeShift;
			}
			if (delay < MasterClock::NANOS_PER_MILLISECOND) break;
			usleep(((delay < MAX_SLEEP_TIME ? delay : MAX_SLEEP_TIME) - MasterClock::NANOS_PER_MILLISECOND) / MasterClock::NANOS_PER_MICROSECOND);
		}
		if (stopProcessing || synthRoute->getState() != SynthRouteState_OPEN) break;
		if (driver->seekPosition > -1) {
			driver->seekPosition = -1;
			continue;
		}
		switch (e.getType()) {
			case SHORT_MESSAGE:
				synthRoute->pushMIDIShortMessage(e.getShortMessage(), currentNanos);
				break;
			case SYSEX:
				synthRoute->pushMIDISysex(e.getSysexData(), e.getSysexLen(), currentNanos);
				break;
			case SET_TEMPO: {
				uint tempo = e.getShortMessage();
				midiTick = parser.getMidiTick(tempo);
				emit driver->tempoUpdated(MidiParser::MICROSECONDS_PER_MINUTE / tempo);
				break;
			}
			default:
				break;
		}
	}
	SMFProcessor::sendAllNotesOff(synthRoute);
	emit driver->playbackTimeChanged(0, 0);
	qDebug() << "SMFDriver: processor thread stopped";
	driver->deleteMidiSession(session);
	if (!stopProcessing) emit driver->playbackFinished();
}

quint32 SMFProcessor::estimateRemainingTime(const MidiEventList &midiEvents, int currentEventIx) {
	MasterClockNanos tick = midiTick;
	MasterClockNanos totalNanos = 0;
	for (int i = currentEventIx; i < midiEvents.count(); i++) {
		const MidiEvent &e = midiEvents.at(i);
		totalNanos += e.getTimestamp() * tick;
		if (e.getType() == SET_TEMPO) tick = parser.getMidiTick(e.getShortMessage());
	}
	return quint32(totalNanos / MasterClock::NANOS_PER_SECOND);
}

int SMFProcessor::seek(SynthRoute *synthRoute, const MidiEventList &midiEvents, int currentEventIx, MasterClockNanos seekNanos, MasterClockNanos currentEventNanos) {
	MasterClockNanos nanosNow = MasterClock::getClockNanos();
	while (!stopProcessing && currentEventNanos < seekNanos && currentEventIx < midiEvents.size()) {
		const MidiEvent &e = midiEvents.at(currentEventIx);
		while (!stopProcessing && synthRoute->getState() == SynthRouteState_OPEN) {
			bool res = true;
			switch (e.getType()) {
				case SHORT_MESSAGE: {
						quint32 msg = e.getShortMessage();
						if ((msg & 0xE0) != 0x80) res = synthRoute->pushMIDIShortMessage(msg, nanosNow);
						break;
					}
				case SYSEX:
					res = synthRoute->pushMIDISysex(e.getSysexData(), e.getSysexLen(), nanosNow);
					break;
				case SET_TEMPO: {
					uint tempo = e.getShortMessage();
					midiTick = parser.getMidiTick(tempo);
					emit driver->tempoUpdated(MidiParser::MICROSECONDS_PER_MINUTE / tempo);
					break;
				}
				default:
					break;
			}
			if (res) break;
			qDebug() << "SMFProcessor: MIDI buffer became full while seeking, taking a nap";
			usleep(MAX_SLEEP_TIME / MasterClock::NANOS_PER_MICROSECOND);
			nanosNow = MasterClock::getClockNanos();
		}
		currentEventIx++;
		currentEventNanos += e.getTimestamp() * midiTick;
	}
	return currentEventIx;
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

void SMFDriver::start(QString fileName) {
	if (!fileName.isEmpty()) {
		processor.start(fileName);
	}
}

void SMFDriver::stop() {
	processor.stop();
}

void SMFDriver::setBPM(quint32 newBPM) {
	processor.setBPM(newBPM);
}

void SMFDriver::setFastForwardingFactor(uint useFastForwardingFactor) {
	fastForwardingFactor = useFastForwardingFactor;
}

void SMFDriver::jump(int newPosition) {
	seekPosition = newPosition;
}

SMFDriver::~SMFDriver() {
	stop();
}
