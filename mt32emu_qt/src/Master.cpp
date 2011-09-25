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

#ifdef WITH_WINMM_AUDIO_DRIVER
#include "audiodrv/WinMMAudioDriver.h"
#endif
#ifdef WITH_QT_AUDIO_DRIVER
#include "audiodrv/QtAudioDriver.h"
#endif
#ifdef WITH_ALSA_AUDIO_DRIVER
#include "audiodrv/AlsaAudioDriver.h"
#endif
#ifdef WITH_PULSE_AUDIO_DRIVER
#include "audiodrv/PulseAudioDriver.h"
#endif
#ifdef WITH_PORT_AUDIO_DRIVER
#include "audiodrv/PortAudioDriver.h"
#endif

#ifdef WITH_WIN32_MIDI_DRIVER
#include "mididrv/Win32Driver.h"
#elif defined(WITH_ALSA_MIDI_DRIVER)
#include "mididrv/ALSADriver.h"
#endif

Master *Master::INSTANCE = NULL;

Master::Master() {
}

void Master::init() {
	static Master master;

	if (INSTANCE == NULL) {
		INSTANCE = &master;
		INSTANCE->moveToThread(QCoreApplication::instance()->thread());
		INSTANCE->initAudioDrivers();
		INSTANCE->initMidiDrivers();
		INSTANCE->getAudioDevices();

		INSTANCE->settings = new QSettings("muntemu.org", "Munt mt32emu-qt");
		INSTANCE->romDir = INSTANCE->settings->value("Master/romDir", "./").toString();

		qRegisterMetaType<MidiDriver *>("MidiDriver*");
		qRegisterMetaType<MidiSession *>("MidiSession*");
		qRegisterMetaType<MidiSession **>("MidiSession**");
		qRegisterMetaType<SynthState>("SynthState");
	}
}

void Master::deinit() {
	delete INSTANCE->settings;
	if (INSTANCE->midiDriver != NULL) {
		INSTANCE->midiDriver->stop();
		INSTANCE->midiDriver = NULL;
	}
	QMutableListIterator<AudioDriver *> audioDriverIt(INSTANCE->audioDrivers);
	while(audioDriverIt.hasNext()) {
		AudioDriver *audioDriver = audioDriverIt.next();
		delete audioDriver;
		audioDriverIt.remove();
	}
	INSTANCE = NULL;
}

void Master::initAudioDrivers() {
#ifdef WITH_WINMM_AUDIO_DRIVER
	audioDrivers.append(new WinMMAudioDriver(this));
#endif
#ifdef WITH_QT_AUDIO_DRIVER
	audioDrivers.append(new QtAudioDriver(this));
#endif
#ifdef WITH_ALSA_AUDIO_DRIVER
	audioDrivers.append(new AlsaAudioDriver(this));
#endif
#ifdef WITH_PULSE_AUDIO_DRIVER
	audioDrivers.append(new PulseAudioDriver(this));
#endif
#ifdef WITH_PORT_AUDIO_DRIVER
	audioDrivers.append(new PortAudioDriver(this));
#endif
}

void Master::initMidiDrivers() {
#ifdef WITH_WIN32_MIDI_DRIVER
	midiDriver = new Win32MidiDriver(this);
#elif defined(WITH_ALSA_MIDI_DRIVER)
	midiDriver = new ALSAMidiDriver(this);
#else
	midiDriver = NULL;
#endif
	if (midiDriver != NULL) {
		midiDriver->start();
	}
}

Master *Master::getInstance() {
	return INSTANCE;
}

const QList<AudioDevice *> Master::getAudioDevices() {
	audioDevices.clear();
	QListIterator<AudioDriver *> audioDriverIt(INSTANCE->audioDrivers);
	while(audioDriverIt.hasNext()) {
		AudioDriver *audioDriver = audioDriverIt.next();
		audioDevices.append(audioDriver->getDeviceList());
	}
	return audioDevices;
}

QSettings *Master::getSettings() {
	return settings;
}

QDir Master::getROMDir() {
	return romDir;
}

void Master::setROMDir(QDir newROMDir) {
	romDir = newROMDir;
	settings->setValue("Master/romDir", romDir.absolutePath());
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
