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

#include "../MasterClock.h"
#include "CoreMidiDriver.h"

void readProc(const MIDIPacketList *packetList, void *readProcRefCon, void *srcConnRefCon) {
Q_UNUSED(srcConnRefCon)

	MidiSession *midiSession = (MidiSession *)readProcRefCon;
	SynthRoute *synthRoute = midiSession->getSynthRoute();
	MIDIPacket const *packet = &packetList->packet[0];
	UInt32 numPackets = packetList->numPackets;
	while (numPackets > 0) {
		UInt32 packetLen = packet->length;
		while (packetLen > 0) {
			Byte const *b = packet->data;
			if (*b == 240) {
				if (packet->data[packetLen - 1] != 247) {
					qDebug() << "Fragmented sysex len:" << packetLen;
					break;
				}
				synthRoute->pushMIDISysex((unsigned char *)b, packetLen, MasterClock::getClockNanos());
				break;
			}
			UInt32 message = 0;
			message = *((UInt32 *)b);
			synthRoute->pushMIDIShortMessage(message, MasterClock::getClockNanos());
			while (--packetLen > 0) {
				if (*(++b) & 0x80) break;
			}
		}
		packet = MIDIPacketNext(packet);
		numPackets--;
	}
}

void CoreMidiDriver::midiNotifyProc(MIDINotification const *message, void *refCon) {
	if (message->messageID == kMIDIMsgObjectAdded) {
		driver->createDestination();
		return;
	}
	if (message->messageID != kMIDIMsgObjectRemoved) return;
	MIDIObjectAddRemoveNotification *removeMessage = (MIDIObjectAddRemoveNotification *)message;
	CoreMidiDriver *driver = (CoreMidiDriver *)refCon;
	if (driver->outDest != removeMessage->child) return;
	qDebug() << "Core MIDI destination disposed";

	// FIXME: no idea if this is really needed
	MIDIEndpointDispose(driver->outDest);

	driver->deleteMidiSession(driver->midiSessions.takeFirst());
	driver->createDestination();
}

CoreMidiDriver::CoreMidiDriver(Master *useMaster) : MidiDriver(useMaster) {
	master = useMaster;
	name = "CoreMidi";
	qDebug() << "Core MIDI Driver started";
}

void CoreMidiDriver::start() {
	MIDIClientCreate(CFSTR("Mt32Emu"), midiNotifyProc, this, &client);
}

void CoreMidiDriver::createDestination() {
	MidiSession *midiSession = createMidiSession("CoreMIDI session #" + QString().setNum(MIDIGetNumberOfDestinations()));
	MIDIDestinationCreate(client, CFSTR("Mt32EmuPort"), readProc, midiSession, &outDest);
	qDebug() << "CoreMIDI Destination created";
	qDebug() << "Number of Destinations:" << MIDIGetNumberOfDestinations();
}

void CoreMidiDriver::stop() {
}
