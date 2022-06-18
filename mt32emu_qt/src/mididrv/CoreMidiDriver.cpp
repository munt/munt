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

#include <QtGlobal>

#include "CoreMidiDriver.h"
#include "../MasterClock.h"

static CoreMidiDriver *driver;

void CoreMidiDriver::readProc(const MIDIPacketList *packetList, void *readProcRefCon, void *) {
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

void CoreMidiDriver::midiNotifyProc(MIDINotification const *message, void *) {
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
	name = "CoreMidi";
	driver = this;
	qDebug() << "Core MIDI Driver started";
}

void CoreMidiDriver::start() {
	nextSessionID = 0;
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
	nextSessionID++;
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

bool CoreMidiDriver::canReconnectPort(MidiSession *midiSession) {
	return canDeletePort(midiSession);
}

MidiDriver::PortNamingPolicy CoreMidiDriver::getPortNamingPolicy() {
	return PortNamingPolicy_UNIQUE;
}

bool CoreMidiDriver::createPort(int, const QString &portName, MidiSession *midiSession) {
	createDestination(midiSession, portName);
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

void CoreMidiDriver::reconnectPort(int, const QString &newPortName, MidiSession *midiSession) {
	for (int i = 0; i < sessions.size(); i++) {
		if (sessions[i]->sessionID == newPortName) return;
	}
	for (int i = 0; i < sessions.size(); i++) {
		if (sessions.at(i)->midiSession == midiSession) {
			disposeDestination(sessions.at(i));
			midiSession->getSynthRoute()->setMidiSessionName(midiSession, newPortName);
			createDestination(midiSession, newPortName);
			return;
		}
	}
}

QString CoreMidiDriver::getNewPortNameHint(QStringList &knownPortNames) {
	for (int i = 0; i < sessions.size(); i++) {
		knownPortNames << sessions[i]->sessionID;
	}
	return "Mt32EmuPort" + QString::number(driver->nextSessionID);
}
