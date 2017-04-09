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
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#include "OSSMidiPortDriver.h"
#include "../MasterClock.h"
#include "../MidiPropertiesDialog.h"

static const QString devDirName = "/dev/";
static const QString defaultMidiPortName = "midi";
static const QString sequencerName = "sequencer";
static const QStringList midiPortTemplates = QStringList() << defaultMidiPortName << "midi?" << "midi??" << "midi?.?" << "umidi?.?" << sequencerName;

static OSSMidiPortDriver *driver;

void* OSSMidiPortDriver::processingThread(void *userData) {
	static const int BUFFER_SIZE = 1024;
	static const int SEQ_MIDIPUTC = 5;
	unsigned char buffer[4 * BUFFER_SIZE];
	unsigned char messageBuffer[BUFFER_SIZE];
	int fd = -1;
	pollfd pfd;

	OSSMidiPortData *data = (OSSMidiPortData *)userData;
	if (data->midiSession == NULL) data->midiSession = driver->createMidiSession(data->midiPortName);
	QMidiStreamParser &qMidiStreamParser = *data->midiSession->getQMidiStreamParser();
	qDebug() << "OSSMidiPortDriver: Processing thread started. Port: " << data->midiPortName;

	while (!data->stopProcessing) {
		if (fd == -1) {
			fd = open(data->midiPortName.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
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
		qMidiStreamParser.setTimestamp(MasterClock::getClockNanos());
		qMidiStreamParser.parseStream(msg, messageLength);
	}
	qDebug() << "OSSMidiPortDriver: Processing thread stopped. Port: " << data->midiPortName;
	if (!data->stopProcessing) driver->deleteMidiSession(data->midiSession);
	data->stopProcessing = false;
	data->processingThreadID = 0;
	driver->sessions.removeOne(data);
	delete data;
	return NULL;
}

OSSMidiPortDriver::OSSMidiPortDriver(Master *useMaster) : MidiDriver(useMaster) {
	master = useMaster;
	name = "OSSMidiPort";
	driver = this;
	qDebug() << "OSS MIDI Port Driver started";
}

void OSSMidiPortDriver::start() {
	QStringList midiInPortNames;
	OSSMidiPortDriver::enumPorts(midiInPortNames);

	if (midiInPortNames.indexOf(sequencerName) >= 0) startSession(NULL, sequencerName, true);
	for (int i = 0; i < midiInPortNames.size(); i++) {
		if (midiInPortNames[i].left(4) == defaultMidiPortName) {
			startSession(NULL, midiInPortNames[i], false);
			break;
		}
	}
}

void OSSMidiPortDriver::stop() {
	for (int i = sessions.size() - 1; i >= 0; i--) {
		MidiSession *midiSession = sessions[i]->midiSession;
		stopSession(sessions[i]);
		if (midiSession != NULL) deleteMidiSession(midiSession);
	}
	qDebug() << "OSS MIDI Port Driver stopped";
}

bool OSSMidiPortDriver::startSession(MidiSession *midiSession, const QString midiPortName, bool sequencerMode) {
	OSSMidiPortData *data = new OSSMidiPortData;
	data->midiSession = midiSession;
	if (midiPortName.contains("/")) {
		data->midiPortName = midiPortName;
	} else {
		data->midiPortName = devDirName + midiPortName;
	}
	data->sequencerMode = sequencerMode;
	data->stopProcessing = false;
	int error = pthread_create(&data->processingThreadID, NULL, processingThread, data);
	if (error != 0) {
		qDebug() << "OSSMidiPortDriver: Processing Thread creation failed:" << error;
		delete data;
		return false;
	}
	sessions.append(data);
	return true;
}

void OSSMidiPortDriver::stopSession(OSSMidiPortData *data) {
	if (data->processingThreadID == 0) return;
	qDebug() << "OSSMidiPortDriver: Stopping processing thread for Port: " << data->midiPortName << "...";
	data->stopProcessing = true;
	pthread_join(data->processingThreadID, NULL);
}

bool OSSMidiPortDriver::canCreatePort() {
	return true;
}

bool OSSMidiPortDriver::canDeletePort(MidiSession *midiSession) {
	for (int i = 0; i < sessions.size(); i++) {
		if (sessions[i]->midiSession == midiSession) {
			if (sessions[i]->sequencerMode) return false;
			return true;
		}
	}
	return false;
}

bool OSSMidiPortDriver::canSetPortProperties(MidiSession *midiSession) {
	return canDeletePort(midiSession);
}

bool OSSMidiPortDriver::createPort(MidiPropertiesDialog *mpd, MidiSession *midiSession) {
	if (startSession(midiSession, mpd->getMidiPortName(), false)) {
		if (midiSession != NULL) midiSessions.append(midiSession);
		return true;
	}
	return false;
}

void OSSMidiPortDriver::deletePort(MidiSession *midiSession) {
	for (int i = 0; i < sessions.size(); i++) {
		if (sessions[i]->midiSession == midiSession) {
			stopSession(sessions[i]);
			midiSessions.removeOne(midiSession);
			break;
		}
	}
}

bool OSSMidiPortDriver::setPortProperties(MidiPropertiesDialog *mpd, MidiSession *midiSession) {
	int sessionIx = -1;
	QString midiPortName = "";
	if (midiSession != NULL) {
		for (int i = 0; i < sessions.size(); i++) {
			if (sessions[i]->midiSession == midiSession) {
				sessionIx = i;
				midiPortName = sessions[i]->midiPortName;
				break;
			}
		}
	}
	QStringList midiInPortNames;
	OSSMidiPortDriver::enumPorts(midiInPortNames);
	midiInPortNames.removeOne(sequencerName);
	mpd->setMidiList(midiInPortNames, -1);
	mpd->setMidiPortName(midiPortName);
	if (mpd->exec() != QDialog::Accepted) return false;
	if (sessionIx == -1 || midiPortName == mpd->getMidiPortName()) return true;
	stopSession(sessions[sessionIx]);
	midiSession->getSynthRoute()->setMidiSessionName(midiSession, mpd->getMidiPortName());
	return startSession(midiSession, mpd->getMidiPortName(), false);
}

QString OSSMidiPortDriver::getNewPortName(MidiPropertiesDialog *mpd) {
	if (setPortProperties(mpd, NULL)) {
		return mpd->getMidiPortName();
	}
	return "";
}

void OSSMidiPortDriver::enumPorts(QStringList &midiPortNames) {
	midiPortNames.append(QDir(devDirName).entryList(midiPortTemplates, QDir::System));
}
