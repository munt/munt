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

#include "Master.h"
#include "MidiSession.h"
#include "audiodrv/AudioDriver.h"

Master *Master::INSTANCE = NULL;

void Master::init() {
	if (INSTANCE == NULL) {
		INSTANCE = new Master();
		INSTANCE->moveToThread(QCoreApplication::instance()->thread());
		INSTANCE->romDir = "./";
		qRegisterMetaType<MidiDriver *>("MidiDriver*");
		qRegisterMetaType<MidiSession *>("MidiSession*");
		qRegisterMetaType<MidiSession **>("MidiSession**");
		qRegisterMetaType<AudioDevice *>("AudioDevice*");
		qRegisterMetaType<const AudioDevice *>("const AudioDevice*");
		qRegisterMetaType<AudioDevice **>("AudioDevice**");
		qRegisterMetaType<QList<AudioDevice *> >("const QList<AudioDevice *>");
		qRegisterMetaType<const AudioDriver *>("const AudioDriver*");
	}
}

void Master::deinit() {
	delete INSTANCE;
	INSTANCE = NULL;
}

Master *Master::getInstance() {
	return INSTANCE;
}

Master::Master() {
}

const QList<AudioDevice *> Master::getAudioDevices() const {
	return audioDevices;
}

QDir Master::getROMDir() {
	return romDir;
}

void Master::setROMDir(QDir newROMDir) {
	romDir = newROMDir;
}

void Master::reallyCreateMidiSession(MidiSession **returnVal, MidiDriver *midiDriver, QString name) {
	// For now, we always create a new SynthRoute for each session
	// In future this will be configurable
	SynthRoute *synthRoute = new SynthRoute(this);
	MidiSession *midiSession = new MidiSession(this, midiDriver, name, synthRoute);
	synthRoute->addMidiSession(midiSession);
	synthRoutes.append(synthRoute);

	// FIXME: find device index using stored preferences
	synthRoute->setAudioDevice(audioDevices.at(0));
	synthRoute->open();
	*returnVal = midiSession;
	emit synthRouteAdded(synthRoute);
}

void Master::reallyDeleteMidiSession(MidiSession *midiSession) {
	SynthRoute *synthRoute = midiSession->getSynthRoute();
	synthRoute->removeMidiSession(midiSession);
	if (!synthRoute->isPinned()) {
		synthRoutes.removeOne(synthRoute);
		emit synthRouteRemoved(synthRoute);
		synthRoute->close();
		delete synthRoute;
	}
	delete midiSession;
}

void Master::reallyAddAudioDevice(AudioDevice *audioDevice) {
	audioDevices.append(audioDevice);
	emit audioDeviceAdded(audioDevice);
}

void Master::reallyRemoveAudioDevice(AudioDevice *audioDevice) {
	audioDevices.removeOne(audioDevice);
	emit audioDeviceRemoved(audioDevice);
	delete audioDevice;
}

void Master::reallyFindAudioDevice(AudioDevice **audioDeviceFound, const AudioDevice *audioDevice) {
	QListIterator<AudioDevice *> audioDevicesIt(audioDevices);
	while (audioDevicesIt.hasNext()) {
		AudioDevice *currentAudioDevice = audioDevicesIt.next();
		if ((currentAudioDevice->driver == audioDevice->driver)
				&& (currentAudioDevice->name == audioDevice->name)) {
			*audioDeviceFound = currentAudioDevice;
			return;
		}
	}
	*audioDeviceFound = NULL;
}

void Master::reallyRemoveStaleAudioDevices(const AudioDriver *driver, const QList<AudioDevice *> foundAudioDevices) {
	QListIterator<AudioDevice *> audioDevicesIt(audioDevices);
	while (audioDevicesIt.hasNext()) {
		AudioDevice *currentAudioDevice = audioDevicesIt.next();
		if (driver != currentAudioDevice->driver) {
			continue;
		}
		bool foundDevice = false;
		QListIterator<AudioDevice *> foundAudioDevicesIt(foundAudioDevices);
		while (foundAudioDevicesIt.hasNext()) {
			AudioDevice *currentFoundAudioDevice = foundAudioDevicesIt.next();
			if ((currentAudioDevice->driver == currentFoundAudioDevice->driver)
					&& (currentAudioDevice->name == currentFoundAudioDevice->name)) {
				foundDevice = true;
				break;
			}
		}
		if (!foundDevice) {
			reallyRemoveAudioDevice(currentAudioDevice);
		}
	}
}

MidiSession *Master::createMidiSession(MidiDriver *midiDriver, QString name) {
	MidiSession *midiSession;
	if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
		reallyCreateMidiSession(&midiSession, midiDriver, name);
	} else {
		QMetaObject::invokeMethod(this, "reallyCreateMidiSession", Qt::BlockingQueuedConnection,
								  Q_ARG(MidiSession **, &midiSession),
								  Q_ARG(MidiDriver *, midiDriver),
								  Q_ARG(QString, name));
	}
	return midiSession;
}

void Master::deleteMidiSession(MidiSession *midiSession) {
	if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
		reallyDeleteMidiSession(midiSession);
	} else {
		QMetaObject::invokeMethod(this, "reallyDeleteMidiSession", Qt::QueuedConnection,
								  Q_ARG(MidiSession *, midiSession));
	}
}

void Master::addAudioDevice(AudioDevice *audioDevice) {
	if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
		reallyAddAudioDevice(audioDevice);
	} else {
		QMetaObject::invokeMethod(this, "reallyAddAudioDevice", Qt::QueuedConnection,
								  Q_ARG(AudioDevice *, audioDevice));
	}
}

void Master::removeAudioDevice(AudioDevice *audioDevice) {
	if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
		reallyRemoveAudioDevice(audioDevice);
	} else {
		QMetaObject::invokeMethod(this, "reallyRemoveAudioDevice", Qt::QueuedConnection,
								  Q_ARG(AudioDevice *, audioDevice));
	}
}

AudioDevice *Master::findAudioDevice(const AudioDevice *audioDevice) {
	AudioDevice *audioDeviceFound;
	if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
		reallyFindAudioDevice(&audioDeviceFound, audioDevice);
	} else {
		QMetaObject::invokeMethod(this, "reallyFindAudioDevice", Qt::BlockingQueuedConnection,
								  Q_ARG(AudioDevice **, &audioDeviceFound),
								  Q_ARG(const AudioDevice *, audioDevice));
	}
	return audioDeviceFound;
}

void Master::removeStaleAudioDevices(const AudioDriver *driver, const QList<AudioDevice *> foundAudioDevices) {
	if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
		reallyRemoveStaleAudioDevices(driver, foundAudioDevices);
	} else {
		QMetaObject::invokeMethod(this, "reallyRemoveStaleAudioDevices", Qt::BlockingQueuedConnection,
								  Q_ARG(const AudioDriver *, driver),
								  Q_ARG(const QList<AudioDevice *>, foundAudioDevices));
	}
}
