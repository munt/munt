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

#include "ALSADriver.h"

#include <QtCore>

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <math.h>

#include <alsa/version.h>
#include <alsa/asoundlib.h>

#include "../MasterClock.h"
#include "../MidiSession.h"
#include "../SynthRoute.h"

ALSAProcessor::ALSAProcessor(ALSAMidiDriver *useALSAMidiDriver, snd_seq_t *useSeq) : alsaMidiDriver(useALSAMidiDriver), seq(useSeq) {
	stopProcessing = false;
}

void ALSAProcessor::stop() {
	stopProcessing = true;
}

void ALSAProcessor::processSeqEvents() {
	// FIXME: For now we just use a single MIDI session for all connections to our port
	MidiSession *midiSession = NULL;
	int pollFDCount;
	struct pollfd *pollFDs;

	pollFDCount = snd_seq_poll_descriptors_count(seq, POLLIN);
	pollFDs = (struct pollfd *)malloc(pollFDCount * sizeof(struct pollfd));
	snd_seq_poll_descriptors(seq, pollFDs, pollFDCount, POLLIN);
	for (;;) {
		// 200ms poll() timeout means that stopProcessing may take approximately this long to be noticed.
		// FIXME: Polling with a timeout is lame.
		int pollEventCount = poll(pollFDs, pollFDCount, 200);
		if (pollEventCount < 0) {
			qDebug() << "poll() returned " << pollEventCount << ", errno=" << errno;
			break;
		}
		if (stopProcessing) {
			break;
		}
		if (pollEventCount == 0) {
			continue;
		}
		unsigned short revents = 0;
		int err = snd_seq_poll_descriptors_revents(seq, pollFDs, pollFDCount, &revents);
		if (err < 0) {
			qDebug() << "snd_seq_poll_descriptors_revents() returned " << err;
			break;
		}
		if ((revents & (POLLERR | POLLNVAL)) != 0) {
			qDebug() << "snd_seq_poll_descriptors_revents() gave revents " << revents;
			break;
		}
		if ((revents & POLLIN) == 0) {
			continue;
		}
		snd_seq_event_t *seq_event = NULL;
		do {
			int status = snd_seq_event_input(seq, &seq_event);
			if (status < 0) {
				switch (status) {
				case -EAGAIN:
				case -ENOSPC:
					continue;
				}
				qDebug() << "Status: " << status;
				break;
			}
			if(midiSession == NULL) {
				midiSession = alsaMidiDriver->getMaster()->createMidiSession(alsaMidiDriver, "Combined ALSA Session");
			}
			if (processSeqEvent(seq_event, midiSession->getSynthRoute())) {
				break;
			}
		} while (!stopProcessing && snd_seq_event_input_pending(seq, 1));
	}
	free(pollFDs);
	snd_seq_close(seq);
	qDebug() << "ALSA MIDI processing loop finished";
	if(midiSession != NULL) {
		alsaMidiDriver->getMaster()->deleteMidiSession(midiSession);
	}
	emit finished();
}

bool ALSAProcessor::processSeqEvent(snd_seq_event_t *seq_event, SynthRoute *synthRoute) {
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
	case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
		// FIXME: Would be nice to have subscribers shown in the GUI
		break;

	case SND_SEQ_EVENT_CLIENT_START:
		printf("Client start\n");
		break;
	case SND_SEQ_EVENT_CLIENT_CHANGE:
		printf("Client change\n");
		break;
	case SND_SEQ_EVENT_CLIENT_EXIT:
		printf("Client exit\n");
		return true;

	case SND_SEQ_EVENT_NOTE:
		// FIXME: Implement these
		break;

	default:
		break;
	}
	return false;
}

static int alsa_setup_midi(snd_seq_t *&seq_handle)
{
	int seqPort;

	/* open sequencer interface for input */
	if (snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
		fprintf(stderr, "Error opening ALSA sequencer.\n");
		return -1;
	}

	snd_seq_set_client_name(seq_handle, "Munt MT-32");
	seqPort = snd_seq_create_simple_port(seq_handle, "Standard",
						SND_SEQ_PORT_CAP_SUBS_WRITE |
						SND_SEQ_PORT_CAP_WRITE,
						SND_SEQ_PORT_TYPE_MIDI_MT32 |
						SND_SEQ_PORT_TYPE_SYNTH);
	if (seqPort < 0) {
		fprintf(stderr, "Error creating sequencer port.\n");
		return -1;
	}

	printf("MT-32 emulator ALSA address is %d:0\n", snd_seq_client_id(seq_handle));

	return seqPort;
}

ALSAMidiDriver::ALSAMidiDriver(Master *useMaster) : MidiDriver(useMaster) {
	processor = NULL;
}

ALSAMidiDriver::~ALSAMidiDriver() {
	stop();
}

void ALSAMidiDriver::start() {
	snd_seq_t *snd_seq;
	if (alsa_setup_midi(snd_seq) >= 0) {
		processor = new ALSAProcessor(this, snd_seq);
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
}

void ALSAMidiDriver::stop() {
	if (processor != NULL) {
		if (processorThread.isRunning()) {
			processor->stop();
			processorThread.wait();
		}
		delete processor;
		processor = NULL;
	}
}
