/* Copyright (C) 2011 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
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
		qRegisterMetaType<MidiDriver *>("MidiDriver*");
		qRegisterMetaType<MidiSession *>("MidiSession*");
		qRegisterMetaType<MidiSession **>("MidiSession**");
		qRegisterMetaType<AudioDevice *>("AudioDevice*");
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

void Master::reallyCreateMidiSession(MidiSession **returnVal, MidiDriver *midiDriver, QString name) {
	// For now, we always create a new SynthRoute for each session
	// In future this will be configurable
	SynthRoute *synthRoute = new SynthRoute(this);
	MidiSession *midiSession = new MidiSession(this, midiDriver, name, synthRoute);
	synthRoute->addMidiSession(midiSession);
	synthRoutes.append(synthRoute);
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
