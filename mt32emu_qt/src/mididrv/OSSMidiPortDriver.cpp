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
#include <errno.h>

#include "../MasterClock.h"
#include "OSSMidiPortDriver.h"

static char defaultMidiPortName[] = "/dev/midi";
static char sequencerName[] = "/dev/sequencer";

void* OSSMidiPortDriver::processingThread(void *userData) {
	static const int BUFFER_SIZE = 1024;
	static const int SYSEX_BUFFER_SIZE = 1024;
	static const int SEQ_MIDIPUTC = 5;
	unsigned char buffer[4 * BUFFER_SIZE];
	unsigned char messageBuffer[BUFFER_SIZE];
	unsigned char sysexBuffer[SYSEX_BUFFER_SIZE];
	int sysexLength = 0;

	OSSMidiPortDriver *driver = (OSSMidiPortDriver *)userData;
	SynthRoute *synthRoute = driver->midiSession->getSynthRoute();
	qDebug() << "OSSMidiPortDriver: Processing thread started";

	while (!driver->pendingClose) {
		int len = read(driver->fd, buffer, BUFFER_SIZE);
		if (len <= 0) {
			if (len == 0) {
				qDebug() << "OSSMidiPortDriver: Closed MIDI port:" << driver->midiPortName << "Reopening ...";
			} else {
				qDebug() << "OSSMidiPortDriver: Error reading from MIDI port:" << driver->midiPortName << ", errno:" << errno << "Reopening ...";
				close(driver->fd);
			}
			if (driver->openPort()) continue;
			break;
		}
		unsigned char *msg = buffer;
		int messageLength = len;
		if (!driver->useOSSMidiPort) {
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
	driver->pendingClose = !driver->pendingClose;
	qDebug() << "OSSMidiPortDriver: Processing thread stopped";
	return NULL;
}

OSSMidiPortDriver::OSSMidiPortDriver(Master *useMaster) : MidiDriver(useMaster) {
	master = useMaster;
	name = "OSSMidiPort";
	midiSession = NULL;
	qDebug() << "OSS MIDI Port Driver started";
}

bool OSSMidiPortDriver::openPort() {
	useOSSMidiPort = true;
	midiPortName = defaultMidiPortName;
	fd = open(midiPortName, O_RDONLY);
	if (fd == -1) {
		qDebug() << "OSSMidiPortDriver: Can't open MIDI port provided:" << midiPortName << ", errno:" << errno << "Falling back to OSS3 /dev/sequencer";
		useOSSMidiPort = false;
		midiPortName = sequencerName;
		fd = open(midiPortName, O_RDONLY);
		if (fd == -1) {
			qDebug() << "OSSMidiPortDriver: Can't open" << midiPortName << "either. Driver stopped.";
			return false;
		}
	}
	return true;
}

void OSSMidiPortDriver::start() {
	if (!openPort()) return;
	midiSession = createMidiSession("Combined OSS MIDI session");
	pendingClose = false;
	pthread_t threadID;
	int error = pthread_create(&threadID, NULL, processingThread, this);
	if (error) {
		qDebug() << "OSSMidiPortDriver: Processing Thread creation failed:" << error;
	}
}

void OSSMidiPortDriver::stop() {
	if (midiSession == NULL) return;
	if (!pendingClose) {
		pendingClose = true;
		qDebug() << "OSSMidiPortDriver: Stopping processing thread...";
		while (pendingClose) {
			sleep(1);
		}
	}
	qDebug() << "OSSMidiPortDriver: Stopped";
	deleteMidiSession(midiSession);
	midiSession = NULL;
}
