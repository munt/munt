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

#include <QSystemTrayIcon>

#include "Master.h"
#include "MasterClock.h"
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
#ifdef WITH_OSS_AUDIO_DRIVER
#include "audiodrv/OSSAudioDriver.h"
#endif
#ifdef WITH_PORT_AUDIO_DRIVER
#include "audiodrv/PortAudioDriver.h"
#endif

#ifdef WITH_WIN32_MIDI_DRIVER
#include "mididrv/Win32Driver.h"
#elif defined(WITH_ALSA_MIDI_DRIVER)
#include "mididrv/ALSADriver.h"
#elif defined(WITH_COREMIDI_DRIVER)
#include "mididrv/CoreMidiDriver.h"
#else
#include "mididrv/OSSMidiPortDriver.h"
#endif

Master *Master::INSTANCE = NULL;

Master::Master() {
}

void Master::init() {
	static Master master;

	if (INSTANCE != NULL) return;
	INSTANCE = &master;
	INSTANCE->moveToThread(QCoreApplication::instance()->thread());
	INSTANCE->settings = new QSettings("muntemu.org", "Munt mt32emu-qt");
	INSTANCE->romDir = INSTANCE->settings->value("Master/romDir", "./").toString();
	INSTANCE->controlROMFileName = INSTANCE->settings->value("Master/ControlROM", "MT32_CONTROL.ROM").toString();
	INSTANCE->pcmROMFileName = INSTANCE->settings->value("Master/PCMROM", "MT32_PCM.ROM").toString();
	INSTANCE->makeROMImages();

	INSTANCE->trayIcon = NULL;
	INSTANCE->defaultAudioDriverId = INSTANCE->settings->value("Master/DefaultAudioDriver").toString();
	INSTANCE->defaultAudioDeviceName = INSTANCE->settings->value("Master/DefaultAudioDevice").toString();

	INSTANCE->initAudioDrivers();
	INSTANCE->initMidiDrivers();
	INSTANCE->lastAudioDeviceScan = -4 * MasterClock::NANOS_PER_SECOND;
	INSTANCE->getAudioDevices();
	INSTANCE->pinnedSynthRoute = NULL;

	qRegisterMetaType<MidiDriver *>("MidiDriver*");
	qRegisterMetaType<MidiSession *>("MidiSession*");
	qRegisterMetaType<MidiSession **>("MidiSession**");
	qRegisterMetaType<SynthState>("SynthState");
}

void Master::deinit() {
	if (INSTANCE->trayIcon != NULL) delete INSTANCE->trayIcon;
	delete INSTANCE->settings;

	if (INSTANCE->midiDriver != NULL) {
		INSTANCE->midiDriver->stop();
		INSTANCE->midiDriver = NULL;
	}

	QMutableListIterator<SynthRoute *> synthRouteIt(INSTANCE->synthRoutes);
	while(synthRouteIt.hasNext()) {
		delete synthRouteIt.next();
		synthRouteIt.remove();
	}

	QMutableListIterator<AudioDevice *> audioDeviceIt(INSTANCE->audioDevices);
	while(audioDeviceIt.hasNext()) {
		delete audioDeviceIt.next();
		audioDeviceIt.remove();
	}

	QMutableListIterator<AudioDriver *> audioDriverIt(INSTANCE->audioDrivers);
	while(audioDriverIt.hasNext()) {
		delete audioDriverIt.next();
		audioDriverIt.remove();
	}

	INSTANCE->freeROMImages();

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
#ifdef WITH_OSS_AUDIO_DRIVER
	audioDrivers.append(new OSSAudioDriver(this));
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
#elif defined(WITH_COREMIDI_DRIVER)
	midiDriver = new CoreMidiDriver(this);
#else
	midiDriver = new OSSMidiPortDriver(this);
#endif
}

void Master::startMidiProcessing() {
	if (midiDriver != NULL) {
		midiDriver->start();
	}
}

Master *Master::getInstance() {
	return INSTANCE;
}

void Master::setDefaultAudioDevice(QString driverId, QString name) {
	defaultAudioDriverId = driverId;
	defaultAudioDeviceName = name;
	settings->setValue("Master/DefaultAudioDriver", driverId);
	settings->setValue("Master/DefaultAudioDevice", name);
}

const QList<AudioDevice *> Master::getAudioDevices() {
	MasterClockNanos nanosNow = MasterClock::getClockNanos();
	if ((nanosNow - lastAudioDeviceScan) > 3 * MasterClock::NANOS_PER_SECOND) {
		lastAudioDeviceScan = nanosNow;
		qDebug() << "Scanning audio devices ...";
		audioDevices.clear();
		QListIterator<AudioDriver *> audioDriverIt(INSTANCE->audioDrivers);
		while(audioDriverIt.hasNext()) {
			AudioDriver *audioDriver = audioDriverIt.next();
			audioDevices.append(audioDriver->getDeviceList());
		}
	}
	return audioDevices;
}

const AudioDevice *Master::findAudioDevice(QString driverId, QString name) const {
	QListIterator<AudioDevice *> audioDeviceIt(audioDevices);
	while(audioDeviceIt.hasNext()) {
		AudioDevice *audioDevice = audioDeviceIt.next();
		if (driverId == audioDevice->driver->id && name == audioDevice->name) {
			return audioDevice;
		}
	}
	return audioDevices.first();
}

QSettings *Master::getSettings() const {
	return settings;
}

QDir Master::getROMDir() {
	return romDir;
}

void Master::setROMDir(QDir newROMDir) {
	romDir = newROMDir;
	settings->setValue("Master/romDir", romDir.absolutePath());
}

void Master::getROMFileNames(QString &controlROMFileName, QString &pcmROMFileName) {
	controlROMFileName = this->controlROMFileName;
	pcmROMFileName = this->pcmROMFileName;
}

void Master::setROMFileNames(QString useControlROMFileName, QString usePCMROMFileName) {
	controlROMFileName = useControlROMFileName;
	pcmROMFileName = usePCMROMFileName;
	settings->setValue("Master/ControlROM", useControlROMFileName);
	settings->setValue("Master/PCMROM", usePCMROMFileName);
	makeROMImages();
}

void Master::getROMImages(const MT32Emu::ROMImage* &controlROMImage, const MT32Emu::ROMImage* &pcmROMImage) {
	controlROMImage = this->controlROMImage;
	pcmROMImage = this->pcmROMImage;
}

const QString Master::getROMPathName(QString romFileName) const {
	QString pathName = QDir::toNativeSeparators(romDir.absolutePath());
	return pathName + QDir::separator() + romFileName;
}

void Master::makeROMImages() {
	MT32Emu::FileStream *file;

	freeROMImages();

	file = new MT32Emu::FileStream();
	if (file->open(getROMPathName(controlROMFileName).toUtf8())) controlROMImage = MT32Emu::ROMImage::makeROMImage(file);

	file = new MT32Emu::FileStream();
	if (file->open(getROMPathName(pcmROMFileName).toUtf8())) pcmROMImage = MT32Emu::ROMImage::makeROMImage(file);
}

void Master::freeROMImages() {
	if (controlROMImage != NULL) MT32Emu::ROMImage::freeROMImage(controlROMImage);
	if (pcmROMImage != NULL) MT32Emu::ROMImage::freeROMImage(pcmROMImage);
	controlROMImage = NULL;
	pcmROMImage = NULL;
}

bool Master::isPinned(const SynthRoute *synthRoute) const {
	return synthRoute == pinnedSynthRoute;
}

void Master::setPinned(SynthRoute *synthRoute) {
	if (pinnedSynthRoute == synthRoute) return;
	settings->setValue("Master/startPinnedSynthRoute", QString().setNum(synthRoute != NULL));
	pinnedSynthRoute = synthRoute;
	emit synthRoutePinned();
}

void Master::startPinnedSynthRoute() {
	if (settings->value("Master/startPinnedSynthRoute", "0").toBool())
		setPinned(startSynthRoute());
}

SynthRoute *Master::startSynthRoute() {
	SynthRoute *synthRoute = pinnedSynthRoute;
	if (synthRoute == NULL) {
		synthRoute = new SynthRoute(this);
		const AudioDevice *audioDevice = NULL;
		getAudioDevices();
		if (!audioDevices.isEmpty()) {
			audioDevice = findAudioDevice(defaultAudioDriverId, defaultAudioDeviceName);
			synthRoute->setAudioDevice(audioDevice);
			synthRoute->open();
			synthRoutes.append(synthRoute);
			emit synthRouteAdded(synthRoute, audioDevice);
		}
	}
	return synthRoute;
}

QSystemTrayIcon *Master::getTrayIcon() const {
	return trayIcon;
}

void Master::setTrayIcon(QSystemTrayIcon *trayIcon) {
	INSTANCE->trayIcon = trayIcon;
}

void Master::showBalloon(const QString &title, const QString &text) {
	trayIcon->showMessage(title, text);
}

void Master::createMidiSession(MidiSession **returnVal, MidiDriver *midiDriver, QString name) {
	SynthRoute *synthRoute = startSynthRoute();
	MidiSession *midiSession = new MidiSession(this, midiDriver, name, synthRoute);
	synthRoute->addMidiSession(midiSession);
	*returnVal = midiSession;
}

void Master::deleteMidiSession(MidiSession *midiSession) {
	SynthRoute *synthRoute = midiSession->getSynthRoute();
	synthRoute->removeMidiSession(midiSession);
	delete midiSession;
	if (synthRoute == pinnedSynthRoute || synthRoute->hasMIDISessions()) return;
	synthRoutes.removeOne(synthRoute);
	emit synthRouteRemoved(synthRoute);
	synthRoute->close();
	delete synthRoute;
}
