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

#include "QSynth.h"

Foo::Foo(QSynth *init_synth, snd_seq_t *init_seq) : synth(init_synth), seq(init_seq) {
	stopProcessing = false;
}

void Foo::stop() {
	stopProcessing = true;
}

void Foo::processSeqEvents() {
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
		if (processSeqEvent(seq_event)) {
			break;
		}
		qDebug() << "Processed event";
	}
	free(pollFDs);
	snd_seq_close(seq);
	qDebug() << "ALSA MIDI processing loop finished";
	emit finished();
}

bool Foo::processSeqEvent(snd_seq_event_t *seq_event) {
	MT32Emu::Bit32u msg = 0;
	switch(seq_event->type) {
	case SND_SEQ_EVENT_NOTEON:
		msg = 0x90;
		msg |= seq_event->data.note.channel;
		msg |= seq_event->data.note.note << 8;
		msg |= seq_event->data.note.velocity << 16;
		synth->pushMIDIShortMessage(msg);
		break;

	case SND_SEQ_EVENT_NOTEOFF:
		msg = 0x80;
		msg |= seq_event->data.note.channel;
		msg |= seq_event->data.note.note << 8;
		msg |= seq_event->data.note.velocity << 16;
		synth->pushMIDIShortMessage(msg);
		break;

	case SND_SEQ_EVENT_CONTROLLER:
		msg = 0xB0;
		msg |= seq_event->data.control.channel;
		msg |= seq_event->data.control.param << 8;
		msg |= seq_event->data.control.value << 16;
		synth->pushMIDIShortMessage(msg);
		break;

	case SND_SEQ_EVENT_PGMCHANGE:
		msg = 0xC0;
		msg |= seq_event->data.control.channel;
		msg |= seq_event->data.control.value << 8;
		synth->pushMIDIShortMessage(msg);
		break;

	case SND_SEQ_EVENT_SYSEX:
		synth->pushMIDISysex((MT32Emu::Bit8u *)seq_event->data.ext.ptr, seq_event->data.ext.len);
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
	case SND_SEQ_EVENT_PITCHBEND:
	case SND_SEQ_EVENT_CONTROL14:
	case SND_SEQ_EVENT_NONREGPARAM:
	case SND_SEQ_EVENT_REGPARAM:
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

ALSADriver::ALSADriver(QSynth *synth)
{
	processor = NULL;
	snd_seq_t *snd_seq;
	if (alsa_setup_midi(snd_seq) >= 0) {
		processor = new Foo(synth, snd_seq);
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

ALSADriver::~ALSADriver() {
	if (processor != NULL) {
		if (processorThread.isRunning()) {
			processor->stop();
			processorThread.wait();
		}
		delete processor;
	}
}
