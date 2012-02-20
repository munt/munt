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
#include <pthread.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#include "../MasterClock.h"
#include "OSSMidiPortDriver.h"

static const QString defaultMidiPortName = "/dev/midi";
static const QString sequencerName = "/dev/sequencer";

static OSSMidiPortDriver *driver;

void* OSSMidiPortDriver::processingThread(void *userData) {
	static const int BUFFER_SIZE = 1024;
	static const int SYSEX_BUFFER_SIZE = 1024;
	static const int SEQ_MIDIPUTC = 5;
	unsigned char buffer[4 * BUFFER_SIZE];
	unsigned char messageBuffer[BUFFER_SIZE];
	unsigned char sysexBuffer[SYSEX_BUFFER_SIZE];
	int sysexLength = 0;
	int fd = -1;
	pollfd pfd;

	OSSMidiPortData *data = (OSSMidiPortData *)userData;
	MidiSession *midiSession = driver->createMidiSession(data->midiPortName);
	SynthRoute *synthRoute = midiSession->getSynthRoute();
	qDebug() << "OSSMidiPortDriver: Processing thread started. Port: " << data->midiPortName;

	while (!data->pendingClose) {
		if (fd == -1) {
			fd = open(data->midiPortName.toAscii().constData(), O_RDONLY | O_NONBLOCK);
			if (fd == -1) {
				qDebug() << "OSSMidiPortDriver: Can't open MIDI port provided:" << data->midiPortName << ", errno:" << errno;
				break;
			}
		}
		pfd.fd = fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		int pollRes = poll(&pfd, 1, 100);
		if (pollRes < 0) {
			qDebug() << "OSSMidiPortDriver: Poll() returned error:" << data->midiPortName << "Reopening ...";
			close(fd);
			fd = -1;
			continue;
		}
		if (pollRes == 0) continue;
		int len = read(fd, buffer, BUFFER_SIZE);
		if (len <= 0) {
			if (len == 0) {
				qDebug() << "OSSMidiPortDriver: Closed MIDI port:" << data->midiPortName << "Reopening ...";
			} else {
				qDebug() << "OSSMidiPortDriver: Error reading from MIDI port:" << data->midiPortName << ", errno:" << errno << "Reopening ...";
				close(fd);
			}
			fd = -1;
			continue;
		}
		unsigned char *msg = buffer;
		int messageLength = len;
		if (data->sequencerMode) {
			messageLength = 0;
			unsigned char *buf = buffer;
			msg = messageBuffer;
			while (len >= 4) {
				len -= 4;
				if (*buf != SEQ_MIDIPUTC) {
					buf += 4;
					continue;
				}
				*(msg++) = *(++buf);
				buf += 3;
				messageLength++;
			}
			msg = messageBuffer;
		}
		if ((*msg & 0x80) == 0 && sysexLength == 0) qDebug() << "OSSMidiPortDriver: Desync in midi stream";
		while (messageLength > 0) {
			// Seek for status byte
			if (sysexLength == 0) {
				if ((*msg & 0x80) == 0) {
					msg++;
					messageLength--;
					continue;
				}
				if (*msg == 0xF0) {
					sysexBuffer[sysexLength++] = *(msg++);
					messageLength--;
				}
			}
			if (sysexLength != 0) {
				while ((messageLength > 0) && ((*msg & 0x80) == 0) && sysexLength < SYSEX_BUFFER_SIZE) {
					sysexBuffer[sysexLength++] = *(msg++);
					messageLength--;
				}
				if (messageLength == 0) continue;
				if (sysexLength > SYSEX_BUFFER_SIZE - 1) qDebug() << "OSSMidiPortDriver: Sysex buffer overrun";
				if (*msg != 0xF7) {
					qDebug() << "OSSMidiPortDriver: Desync in sysex, ending:" << *msg;
					sysexLength = 0;
					continue;
				}
				sysexBuffer[sysexLength++] = *(msg++);
				messageLength--;
				synthRoute->pushMIDISysex(sysexBuffer, sysexLength, MasterClock::getClockNanos());
				sysexLength = 0;
				continue;
			}
			synthRoute->pushMIDIShortMessage(*((unsigned int *)msg), MasterClock::getClockNanos());
			msg++;
			messageLength--;
		}
	}
	driver->deleteMidiSession(midiSession);
	data->pendingClose = !data->pendingClose;
	qDebug() << "OSSMidiPortDriver: Processing thread stopped. Port: " << data->midiPortName;
	return NULL;
}

OSSMidiPortDriver::OSSMidiPortDriver(Master *useMaster) : MidiDriver(useMaster) {
	master = useMaster;
	name = "OSSMidiPort";
	driver = this;
	qDebug() << "OSS MIDI Port Driver started";
}

void OSSMidiPortDriver::start() {
	startSession(defaultMidiPortName, false);
	startSession(sequencerName, true);
}

void OSSMidiPortDriver::stop() {
	for (int i = sessions.size() - 1; i >= 0; i--) {
		stopSession(sessions[i]);
	}
	qDebug() << "OSS MIDI Port Driver stopped";
}

void OSSMidiPortDriver::startSession(const QString midiPortName, bool sequencerMode) {
	OSSMidiPortData *data = new OSSMidiPortData;
	data->midiPortName = midiPortName;
	data->sequencerMode = sequencerMode;
	data->pendingClose = false;
	pthread_t threadID;
	int error = pthread_create(&threadID, NULL, processingThread, data);
	if (error) {
		qDebug() << "OSSMidiPortDriver: Processing Thread creation failed:" << error;
		delete data;
		return;
	}
	sessions.append(data);
}

void OSSMidiPortDriver::stopSession(OSSMidiPortData *data) {
	if (!data->pendingClose) {
		data->pendingClose = true;
		qDebug() << "OSSMidiPortDriver: Stopping processing thread for Port: " << data->midiPortName << "...";
		while (data->pendingClose) {
			sleep(1);
		}
	}
	sessions.removeOne(data);
	delete data;
}
