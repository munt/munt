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

#include "ALSADriver.h"

#include <stdlib.h>
#include <poll.h>
#include <pthread.h>
#include <QtCore>

#include "../MasterClock.h"
#include "../MidiSession.h"
#include "../SynthRoute.h"
#include "../MidiPropertiesDialog.h"

void *ALSAMidiDriver::processingThread(void *userData) {
	ALSAMidiDriver *driver = (ALSAMidiDriver *)userData;
	driver->processSeqEvents();
	driver->processingThreadID = 0;
	return NULL;
}

void ALSAMidiDriver::processSeqEvents() {
	int pollFDCount;
	struct pollfd *pollFDs;

	pollFDCount = snd_seq_poll_descriptors_count(snd_seq, POLLIN);
	pollFDs = (struct pollfd *)malloc(pollFDCount * sizeof(struct pollfd));
	snd_seq_poll_descriptors(snd_seq, pollFDs, pollFDCount, POLLIN);
	for (;;) {
		// 200ms poll() timeout means that stopProcessing may take approximately this long to be noticed.
		// FIXME: Polling with a timeout is lame.
		int pollEventCount = poll(pollFDs, pollFDCount, 200);
		if (pollEventCount < 0) {
			qDebug() << "ALSAMidiDriver: poll() returned " << pollEventCount << ", errno=" << errno;
			break;
		}
		if (stopProcessing) {
			break;
		}
		if (pollEventCount == 0) {
			continue;
		}
		unsigned short revents = 0;
		int err = snd_seq_poll_descriptors_revents(snd_seq, pollFDs, pollFDCount, &revents);
		if (err < 0) {
			qDebug() << "ALSAMidiDriver: snd_seq_poll_descriptors_revents() returned " << err;
			break;
		}
		if ((revents & (POLLERR | POLLNVAL)) != 0) {
			qDebug() << "ALSAMidiDriver: snd_seq_poll_descriptors_revents() gave revents " << revents;
			break;
		}
		if ((revents & POLLIN) == 0) {
			continue;
		}
		snd_seq_event_t *seq_event = NULL;
		do {
			int status = snd_seq_event_input(snd_seq, &seq_event);
			if (status < 0) {
				switch (status) {
				case -EAGAIN:
				case -ENOSPC:
					continue;
				}
				qDebug() << "ALSAMidiDriver: Status: " << status;
				break;
			}
			unsigned int clientAddr = getSourceAddr(seq_event);
			MidiSession *midiSession = findMidiSessionForClient(clientAddr);
			if (midiSession == NULL) {
				QString appName = getClientName(clientAddr);
				midiSession = createMidiSession(appName);
				if (midiSession == NULL) {
					qDebug() << "ALSAMidiDriver: Can't create new MIDI Session. Exiting...";
					break;
				}
				clients.append(clientAddr);
				showBalloon("Connected application:", appName);
				qDebug() << "ALSAMidiDriver: Connected application" << appName;
			}
			if (processSeqEvent(seq_event, midiSession->getSynthRoute())) {
				break;
			}
		} while (!stopProcessing && snd_seq_event_input_pending(snd_seq, 1));
	}
	free(pollFDs);
	snd_seq_close(snd_seq);
	qDebug() << "ALSAMidiDriver: MIDI processing loop finished";

	QMutableListIterator<MidiSession *> midiSessionIt(midiSessions);
	while(midiSessionIt.hasNext()) {
		MidiSession *midiSession = midiSessionIt.next();
		if (midiSession != NULL) {
			deleteMidiSession(midiSession);
		}
	}
	clients.clear();
	stopProcessing = false;
}

unsigned int ALSAMidiDriver::getSourceAddr(snd_seq_event_t *seq_event) {
	snd_seq_addr addr;
	if (seq_event->type == SND_SEQ_EVENT_PORT_SUBSCRIBED || seq_event->type == SND_SEQ_EVENT_PORT_UNSUBSCRIBED)
		addr = seq_event->data.connect.sender;
	else
		addr = seq_event->source;
	return (addr.client << 8) | addr.port;
}

QString ALSAMidiDriver::getClientName(unsigned int clientAddr) {
	snd_seq_client_info_t *info;
	snd_seq_client_info_alloca(&info);
	if (snd_seq_get_any_client_info(snd_seq, clientAddr >> 8, info) != 0) return "Unknown ALSA session";
	return QString().setNum(clientAddr >> 8) + ":" + QString().setNum(clientAddr & 0xFF) + " " + snd_seq_client_info_get_name(info);
}

MidiSession *ALSAMidiDriver::findMidiSessionForClient(unsigned int clientAddr) {
	int i = clients.indexOf(clientAddr);
	if (i == -1) return NULL;
	return midiSessions.at(i);
}

bool ALSAMidiDriver::processSeqEvent(snd_seq_event_t *seq_event, SynthRoute *synthRoute) {
	MT32Emu::Bit32u msg = 0;
	switch(seq_event->type) {
	case SND_SEQ_EVENT_NOTEON:
		msg = 0x90;
		msg |= seq_event->data.note.channel;
		msg |= seq_event->data.note.note << 8;
		msg |= seq_event->data.note.velocity << 16;
		synthRoute->pushMIDIShortMessage(msg, MasterClock::getClockNanos());
		break;

	case SND_SEQ_EVENT_NOTEOFF:
		msg = 0x80;
		msg |= seq_event->data.note.channel;
		msg |= seq_event->data.note.note << 8;
		msg |= seq_event->data.note.velocity << 16;
		synthRoute->pushMIDIShortMessage(msg, MasterClock::getClockNanos());
		break;

	case SND_SEQ_EVENT_CONTROLLER:
		msg = 0xB0;
		msg |= seq_event->data.control.channel;
		msg |= seq_event->data.control.param << 8;
		msg |= seq_event->data.control.value << 16;
		synthRoute->pushMIDIShortMessage(msg, MasterClock::getClockNanos());
		break;

	case SND_SEQ_EVENT_CONTROL14:
		msg = 0xB0;
		msg |= seq_event->data.control.channel;
		msg |= 0x06 << 8;
		msg |= (seq_event->data.control.value >> 7) << 16;
		synthRoute->pushMIDIShortMessage(msg, MasterClock::getClockNanos());
		break;

	case SND_SEQ_EVENT_NONREGPARAM:
		msg = 0xB0;
		msg |= seq_event->data.control.channel;
		msg |= 0x63 << 8; // Since the real synths don't support NRPNs, it's OK to send just the MSB
		synthRoute->pushMIDIShortMessage(msg, MasterClock::getClockNanos());
		break;

	case SND_SEQ_EVENT_REGPARAM:
		msg = 0xB0;
		msg |= seq_event->data.control.channel;
		int rpn;
		rpn = seq_event->data.control.value;
		msg |= 0x64 << 8;
		msg |= (rpn & 0x7F) << 16;
		synthRoute->pushMIDIShortMessage(msg, MasterClock::getClockNanos());
		msg &= 0xFF;
		msg |= 0x65 << 8;
		msg |= ((rpn >> 7) & 0x7F) << 16;
		synthRoute->pushMIDIShortMessage(msg, MasterClock::getClockNanos());
		break;

	case SND_SEQ_EVENT_PGMCHANGE:
		msg = 0xC0;
		msg |= seq_event->data.control.channel;
		msg |= seq_event->data.control.value << 8;
		synthRoute->pushMIDIShortMessage(msg, MasterClock::getClockNanos());
		break;

	case SND_SEQ_EVENT_PITCHBEND:
		msg = 0xE0;
		msg |= seq_event->data.control.channel;
		int bend;
		bend = seq_event->data.control.value + 8192;
		msg |= (bend & 0x7F) << 8;
		msg |= ((bend >> 7) & 0x7F) << 16;
		synthRoute->pushMIDIShortMessage(msg, MasterClock::getClockNanos());
		break;

	case SND_SEQ_EVENT_SYSEX:
		synthRoute->pushMIDISysex((MT32Emu::Bit8u *)seq_event->data.ext.ptr, seq_event->data.ext.len, MasterClock::getClockNanos());
		break;

	case SND_SEQ_EVENT_PORT_SUBSCRIBED:
		// no-op
		break;
	case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
		unsigned int clientAddr;
		clientAddr = getSourceAddr(seq_event);
		MidiSession *midiSession;
		midiSession = findMidiSessionForClient(clientAddr);
		if (midiSession != NULL) deleteMidiSession(midiSession);
		clients.removeAll(clientAddr);
		break;

	case SND_SEQ_EVENT_CLIENT_START:
		qDebug() << "ALSAMidiDriver: Client start";
		break;
	case SND_SEQ_EVENT_CLIENT_CHANGE:
		qDebug() << "ALSAMidiDriver: Client change";
		break;
	case SND_SEQ_EVENT_CLIENT_EXIT:
		qDebug() << "ALSAMidiDriver: Client exit";
		return true;

	case SND_SEQ_EVENT_NOTE:
		// FIXME: Implement these
		break;

	default:
		break;
	}
	return false;
}

int ALSAMidiDriver::alsa_setup_midi() {
	int seqPort;

	/* open sequencer interface for input */
	if (snd_seq_open(&snd_seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
		qDebug() << "ALSAMidiDriver: Error opening ALSA sequencer";
		return -1;
	}

	snd_seq_set_client_name(snd_seq, "Munt MT-32");
	seqPort = snd_seq_create_simple_port(snd_seq, "Standard",
						SND_SEQ_PORT_CAP_SUBS_WRITE |
						SND_SEQ_PORT_CAP_WRITE,
						SND_SEQ_PORT_TYPE_MIDI_GENERIC |
						SND_SEQ_PORT_TYPE_MIDI_MT32 |
						SND_SEQ_PORT_TYPE_SYNTH);
	if (seqPort < 0) {
		qDebug() << "ALSAMidiDriver: Error creating sequencer port";
		return -1;
	}
	QString midiPortStr = QString().setNum(snd_seq_client_id(snd_seq)) + ":0";
	qDebug() << "MT-32 emulator ALSA address is:" << midiPortStr;
	emit mainWindowTitleContributionUpdated("ALSA MIDI Port " + midiPortStr);
	return seqPort;
}

ALSAMidiDriver::ALSAMidiDriver(Master *useMaster) : MidiDriver(useMaster), processingThreadID(0) {}

ALSAMidiDriver::~ALSAMidiDriver() {
	stop();
}

void ALSAMidiDriver::start() {
	connect(this, SIGNAL(mainWindowTitleContributionUpdated(const QString &)), master, SLOT(updateMainWindowTitleContribution(const QString &)));
	if (alsa_setup_midi() < 0) return;
	stopProcessing = false;
	int error = pthread_create(&processingThreadID, NULL, processingThread, this);
	if (error != 0) {
		processingThreadID = 0;
		qDebug() << "ALSAMidiDriver: Processing Thread creation failed:" << error;
		snd_seq_close(snd_seq);
	}
}

void ALSAMidiDriver::stop() {
	if (processingThreadID == 0) return;
	qDebug() << "ALSAMidiDriver: Stopping MIDI processing loop...";
	stopProcessing = true;
	pthread_join(processingThreadID, NULL);
	processingThreadID = 0;
}

bool ALSAMidiDriver::canSetPortProperties(MidiSession *) {
	return true;
}

bool ALSAMidiDriver::setPortProperties(MidiPropertiesDialog *mpd, MidiSession *) {
	mpd->setMidiPortListEnabled(false);
	mpd->setMidiPortNameEditorEnabled(false);
	mpd->setMidiPortName(QString().setNum(snd_seq_client_id(snd_seq)));
	mpd->exec();
	return false;
}
