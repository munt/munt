/* Copyright (C) 2011-2017 Jerome Fisher, Sergey V. Mikayev
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

#include "CoreMidiDriver.h"
#include "../MasterClock.h"
#include "../MidiPropertiesDialog.h"

static CoreMidiDriver *driver;

void CoreMidiDriver::readProc(const MIDIPacketList *packetList, void *readProcRefCon, void *srcConnRefCon) {
Q_UNUSED(srcConnRefCon)

	CoreMidiSession *data = (CoreMidiSession *)readProcRefCon;
	if (data->midiSession == NULL) {
		data->midiSession = driver->createMidiSession(data->sessionID);
	}
	QMidiStreamParser &qMidiStreamParser = *data->midiSession->getQMidiStreamParser();
	qMidiStreamParser.setTimestamp(MasterClock::getClockNanos());
	MIDIPacket const *packet = &packetList->packet[0];
	UInt32 numPackets = packetList->numPackets;
	while (numPackets > 0) {
		UInt32 packetLen = packet->length;
		if (packetLen > 0) {
			qMidiStreamParser.parseStream(packet->data, packetLen);
		}
		packet = MIDIPacketNext(packet);
		numPackets--;
	}
}

void CoreMidiDriver::midiNotifyProc(MIDINotification const *message, void *refCon) {
Q_UNUSED(refCon)
	if (message->messageID != kMIDIMsgObjectRemoved) return;
	MIDIObjectAddRemoveNotification *removeMessage = (MIDIObjectAddRemoveNotification *)message;
	for (int i = 0; i < driver->sessions.size(); i++) {
		CoreMidiSession *data = driver->sessions[i];
		if (data->outDest != removeMessage->child) continue;
		MIDIEndpointDispose(data->outDest);
		MIDIDestinationCreate(driver->client, CFStringCreateWithCString(NULL, data->sessionID.toUtf8().constData(), kCFStringEncodingUTF8), readProc, data, &data->outDest);
		qDebug() << "Core MIDI destination recreated. ID:" << data->sessionID;
	}
}

CoreMidiDriver::CoreMidiDriver(Master *useMaster) : MidiDriver(useMaster) {
	master = useMaster;
	name = "CoreMidi";
	driver = this;
	qDebug() << "Core MIDI Driver started";
}

void CoreMidiDriver::start() {
	sessionID = 1;
	MIDIClientCreate(CFSTR("Mt32Emu"), midiNotifyProc, this, &client);
	createDestination(NULL, "Mt32EmuPort");
}

void CoreMidiDriver::stop() {
	for (int i = sessions.size() - 1; i >= 0; i--) {
		MidiSession *midiSession = sessions[i]->midiSession;
		disposeDestination(sessions[i]);
		if (midiSession != NULL) deleteMidiSession(midiSession);
	}
}

void CoreMidiDriver::createDestination(MidiSession *midiSession, QString sessionID) {
	CoreMidiSession *data = new CoreMidiSession;
	data->midiSession = midiSession;
	data->sessionID = sessionID;
	sessions.append(data);
	MIDIDestinationCreate(client, CFStringCreateWithCString(NULL, data->sessionID.toUtf8().constData(), kCFStringEncodingUTF8), readProc, data, &data->outDest);
	qDebug() << "Core MIDI Destination created. ID:" << data->sessionID;
}

void CoreMidiDriver::disposeDestination(CoreMidiSession *data) {
	MIDIEndpointDispose(data->outDest);
	qDebug() << "Core MIDI destination disposed. ID:" << data->sessionID;
	sessions.removeOne(data);
	delete data;
}

bool CoreMidiDriver::canCreatePort() {
	return true;
}

bool CoreMidiDriver::canDeletePort(MidiSession *midiSession) {
	for (int i = 0; i < sessions.size(); i++) {
		if (sessions[i]->midiSession == midiSession) return true;
	}
	return false;
}

bool CoreMidiDriver::canSetPortProperties(MidiSession *midiSession) {
	return canDeletePort(midiSession);
}

bool CoreMidiDriver::createPort(MidiPropertiesDialog *mpd, MidiSession *midiSession) {
	createDestination(midiSession, mpd->getMidiPortName());
	if (midiSession != NULL) midiSessions.append(midiSession);
	return true;
}

void CoreMidiDriver::deletePort(MidiSession *midiSession) {
	for (int i = 0; i < sessions.size(); i++) {
		if (sessions[i]->midiSession == midiSession) {
			disposeDestination(sessions[i]);
			midiSessions.removeOne(midiSession);
			break;
		}
	}
}

bool CoreMidiDriver::setPortProperties(MidiPropertiesDialog *mpd, MidiSession *midiSession) {
	int sessionIx = -1;
	QString sessionID = "Mt32EmuPort";
	if (midiSession != NULL) {
		for (int i = 0; i < sessions.size(); i++) {
			if (sessions[i]->midiSession == midiSession) {
				sessionIx = i;
				sessionID = sessions[i]->sessionID;
				break;
			}
		}
	} else {
		sessionID = "Mt32EmuPort" + QString().setNum(driver->sessionID++);
	}
	QList<QString> sessionIDs;
	for (int i = 0; i < sessions.size(); i++) {
		sessionIDs.append(sessions[i]->sessionID);
	}
	mpd->setMidiPortListEnabled(false);
	mpd->setMidiList(sessionIDs, -1);
	mpd->setMidiPortName(sessionID);
	if (mpd->exec() != QDialog::Accepted) return false;
	if (sessionIx == -1 || sessionID == mpd->getMidiPortName()) return true;
	disposeDestination(sessions[sessionIx]);
	midiSession->getSynthRoute()->setMidiSessionName(midiSession, mpd->getMidiPortName());
	createDestination(midiSession, mpd->getMidiPortName());
	return true;
}

QString CoreMidiDriver::getNewPortName(MidiPropertiesDialog *mpd) {
	if (setPortProperties(mpd, NULL)) {
		return mpd->getMidiPortName();
	}
	return "";
}
