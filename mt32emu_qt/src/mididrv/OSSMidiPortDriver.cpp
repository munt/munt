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

static bool useOSSMidiPort = false;
static char midiPortName[] = "/dev/midi";

void* OSSMidiPortDriver::processingThread(void *userData) {
	static const int BUFFER_SIZE = 1024;
	static const int SEQ_MIDIPUTC = 5;
	unsigned char messageBuffer[BUFFER_SIZE];
	unsigned char buffer[4 * BUFFER_SIZE];

	OSSMidiPortDriver *driver = (OSSMidiPortDriver *)userData;
	SynthRoute *synthRoute = driver->midiSession->getSynthRoute();
	qDebug() << "OSSMidiPortDriver: Processing thread started";

	while (!driver->pendingClose) {
		int len = read(driver->fd, buffer, BUFFER_SIZE);
		if (len <= 0) {
			if (len == 0) {
				qDebug() << "OSSMidiPortDriver: Closed MIDI port:" << midiPortName << "Reopening ...";
			} else {
				qDebug() << "OSSMidiPortDriver: Error reading from MIDI port:" << midiPortName << ", errno:" << errno << "Reopening ...";
				close(driver->fd);
			}
			driver->fd = open(midiPortName, O_RDONLY);
			if (driver->fd == -1) {
				qDebug() << "OSSMidiPortDriver: Can't open MIDI port provided:" << midiPortName << ", errno:" << errno;
				break;
			}
			continue;
		}
		if (useOSSMidiPort) {
			if (buffer[0] == 240) {
				if (buffer[len - 1] != 247) {
					qDebug() << "Fragmented sysex, len:" << len;
					break;
				}
				synthRoute->pushMIDISysex(buffer, len, MasterClock::getClockNanos());
				continue;
			}
			synthRoute->pushMIDIShortMessage(*((unsigned int *)buffer), MasterClock::getClockNanos());
		} else {
			int messageLength = 0;
			unsigned char *buf = buffer;
			unsigned char *msg = messageBuffer;
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
			if (messageBuffer[0] == 240) {
				if (messageBuffer[messageLength - 1] != 247) {
					qDebug() << "Fragmented sysex, len:" << messageLength;
					break;
				}
				synthRoute->pushMIDISysex(messageBuffer, messageLength, MasterClock::getClockNanos());
				continue;
			}
			synthRoute->pushMIDIShortMessage(*((unsigned int *)messageBuffer), MasterClock::getClockNanos());
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

void OSSMidiPortDriver::start() {
	fd = open(midiPortName, O_RDONLY);
	if (fd == -1) {
		qDebug() << "OSSMidiPortDriver: Can't open MIDI port provided:" << midiPortName << ", errno:" << errno;
		return;
	}
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
