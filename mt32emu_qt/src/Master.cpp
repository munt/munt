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
#ifdef WITH_ALSA_AUDIO_DRIVER
#include "audiodrv/AlsaAudioDriver.h"
#endif
#ifdef WITH_OSS_AUDIO_DRIVER
#include "audiodrv/OSSAudioDriver.h"
#endif
#ifdef WITH_PULSE_AUDIO_DRIVER
#include "audiodrv/PulseAudioDriver.h"
#endif
#ifdef WITH_PORT_AUDIO_DRIVER
#include "audiodrv/PortAudioDriver.h"
#endif
#ifdef WITH_QT_AUDIO_DRIVER
#include "audiodrv/QtAudioDriver.h"
#endif

#include "audiodrv/AudioFileWriterDriver.h"

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

	MasterClock::init();

	INSTANCE->settings = new QSettings("muntemu.org", "Munt mt32emu-qt");
	INSTANCE->synthProfileName = INSTANCE->settings->value("Master/defaultSynthProfile", "default").toString();

	INSTANCE->trayIcon = NULL;
	INSTANCE->defaultAudioDriverId = INSTANCE->settings->value("Master/DefaultAudioDriver").toString();
	INSTANCE->defaultAudioDeviceName = INSTANCE->settings->value("Master/DefaultAudioDevice").toString();

	INSTANCE->initAudioDrivers();
	INSTANCE->initMidiDrivers();
	INSTANCE->lastAudioDeviceScan = -4 * MasterClock::NANOS_PER_SECOND;
	INSTANCE->getAudioDevices();
	INSTANCE->pinnedSynthRoute = NULL;
	INSTANCE->running = true;

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
		delete INSTANCE->midiDriver;
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

	MasterClock::deinit();

	INSTANCE = NULL;
}

void Master::initAudioDrivers() {
#ifdef WITH_WINMM_AUDIO_DRIVER
	audioDrivers.append(new WinMMAudioDriver(this));
#endif
#ifdef WITH_ALSA_AUDIO_DRIVER
	audioDrivers.append(new AlsaAudioDriver(this));
#endif
#ifdef WITH_OSS_AUDIO_DRIVER
	audioDrivers.append(new OSSAudioDriver(this));
#endif
#ifdef WITH_PULSE_AUDIO_DRIVER
	audioDrivers.append(new PulseAudioDriver(this));
#endif
#ifdef WITH_PORT_AUDIO_DRIVER
	audioDrivers.append(new PortAudioDriver(this));
#endif
#ifdef WITH_QT_AUDIO_DRIVER
	audioDrivers.append(new QtAudioDriver(this));
#endif
	audioDrivers.append(new AudioFileWriterDriver(this));
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

bool Master::isRunning() {
	return running;
}

void Master::shutDown() {
	running = false;
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

QString Master::getDefaultSynthProfileName() {
	return synthProfileName;
}

bool Master::setDefaultSynthProfileName(QString name) {
	const QStringList profiles = enumSynthProfiles();
	if (profiles.contains(name, Qt::CaseInsensitive)) {
		synthProfileName = name;
		settings->setValue("Master/defaultSynthProfile", synthProfileName);
		return true;
	}
	return false;
}

const QStringList Master::enumSynthProfiles() const {
	settings->beginGroup("Profiles");
	const QStringList profiles = settings->childGroups();
	settings->endGroup();
	return profiles;
}

const QString Master::getROMPathName(const QDir &romDir, QString romFileName) const {
	QString pathName = QDir::toNativeSeparators(romDir.absolutePath());
	return pathName + QDir::separator() + romFileName;
}

void Master::makeROMImages(SynthProfile &synthProfile) {
	freeROMImages(synthProfile.controlROMImage, synthProfile.pcmROMImage);

	foreach (SynthRoute *synthRoute, INSTANCE->synthRoutes) {
		SynthProfile profile;
		synthRoute->getSynthProfile(profile);
		if (synthProfile.romDir != profile.romDir) continue;
		if (synthProfile.controlROMFileName == profile.controlROMFileName) synthProfile.controlROMImage = profile.controlROMImage;
		if (synthProfile.pcmROMFileName == profile.pcmROMFileName) synthProfile.pcmROMImage = profile.pcmROMImage;
	}

	MT32Emu::FileStream *file;
	if (synthProfile.controlROMImage == NULL) {
		file = new MT32Emu::FileStream();
		if (file->open(getROMPathName(synthProfile.romDir, synthProfile.controlROMFileName).toUtf8())) {
			synthProfile.controlROMImage = MT32Emu::ROMImage::makeROMImage(file);
			if (synthProfile.controlROMImage->getROMInfo() == NULL) {
				freeROMImages(synthProfile.controlROMImage, synthProfile.pcmROMImage);
				return;
			}
		}
	}
	if (synthProfile.pcmROMImage == NULL) {
		file = new MT32Emu::FileStream();
		if (file->open(getROMPathName(synthProfile.romDir, synthProfile.pcmROMFileName).toUtf8())) {
			synthProfile.pcmROMImage = MT32Emu::ROMImage::makeROMImage(file);
			if (synthProfile.pcmROMImage->getROMInfo() == NULL) {
				freeROMImages(synthProfile.controlROMImage, synthProfile.pcmROMImage);
			}
		}
	}
}

void Master::freeROMImages(const MT32Emu::ROMImage* &controlROMImage, const MT32Emu::ROMImage* &pcmROMImage) {
	if (controlROMImage == NULL && pcmROMImage == NULL) return;
	bool controlROMInUse = false;
	bool pcmROMInUse = false;
	foreach (SynthRoute *synthRoute, INSTANCE->synthRoutes) {
		SynthProfile synthProfile;
		synthRoute->getSynthProfile(synthProfile);
		controlROMInUse = controlROMInUse || (synthProfile.controlROMImage == controlROMImage);
		pcmROMInUse = pcmROMInUse || (synthProfile.pcmROMImage == pcmROMImage);
	}
	if (!controlROMInUse && controlROMImage != NULL) {
		delete controlROMImage->getFile();
		MT32Emu::ROMImage::freeROMImage(controlROMImage);
		controlROMImage = NULL;
	}
	if (!pcmROMInUse && pcmROMImage != NULL) {
		delete pcmROMImage->getFile();
		MT32Emu::ROMImage::freeROMImage(pcmROMImage);
		pcmROMImage = NULL;
	}
}

void Master::loadSynthProfile(SynthProfile &synthProfile, QString name) {
	static bool romNotSetReported = false;
	if (name.isEmpty()) name = synthProfileName;
	settings->beginGroup("Profiles/" + name);
	synthProfile.romDir.setPath(settings->value("romDir", "./").toString());
	synthProfile.controlROMFileName = settings->value("controlROM").toString();
	synthProfile.pcmROMFileName = settings->value("pcmROM").toString();
	synthProfile.emuDACInputMode = (MT32Emu::DACInputMode)settings->value("emuDACInputMode", 0).toInt();
	synthProfile.reverbEnabled = settings->value("reverbEnabled", true).toBool();
	synthProfile.reverbOverridden = settings->value("reverbOverridden", false).toBool();
	synthProfile.reverbMode = settings->value("reverbMode", 0).toInt();
	synthProfile.reverbTime = settings->value("reverbTime", 5).toInt();
	synthProfile.reverbLevel = settings->value("reverbLevel", 3).toInt();
	synthProfile.outputGain = settings->value("outputGain", 1.0f).toFloat();
	synthProfile.reverbOutputGain = settings->value("reverbOutputGain", 1.0f).toFloat();
	settings->endGroup();

	makeROMImages(synthProfile);
	if (romNotSetReported || name != synthProfileName) return;
	if (synthProfile.controlROMImage == NULL || synthProfile.pcmROMImage == NULL) {
		romNotSetReported = true;
		emit romsNotSet();
		loadSynthProfile(synthProfile, name);
		romNotSetReported = false;
	}
}

void Master::storeSynthProfile(const SynthProfile &synthProfile, QString name) const {
	if (name.isEmpty()) name = synthProfileName;
	settings->beginGroup("Profiles/" + name);
	settings->setValue("romDir", synthProfile.romDir.absolutePath());
	settings->setValue("controlROM", synthProfile.controlROMFileName);
	settings->setValue("pcmROM", synthProfile.pcmROMFileName);
	settings->setValue("emuDACInputMode", synthProfile.emuDACInputMode);
	settings->setValue("reverbEnabled", synthProfile.reverbEnabled);
	settings->setValue("reverbOverridden", synthProfile.reverbOverridden);
	settings->setValue("reverbMode", synthProfile.reverbMode);
	settings->setValue("reverbTime", synthProfile.reverbTime);
	settings->setValue("reverbLevel", synthProfile.reverbLevel);
	settings->setValue("outputGain", QString().setNum(synthProfile.outputGain));
	settings->setValue("reverbOutputGain", QString().setNum(synthProfile.reverbOutputGain));
	settings->endGroup();
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

bool Master::canCreateMidiPort() {
	return midiDriver->canCreatePort();
}

bool Master::canDeleteMidiPort(MidiSession *midiSession) {
	return midiDriver->canDeletePort(midiSession);
}

bool Master::canSetMidiPortProperties(MidiSession *midiSession) {
	return midiDriver->canSetPortProperties(midiSession);
}

void Master::createMidiPort(MidiPropertiesDialog *mpd, SynthRoute *synthRoute) {
	QString portName = midiDriver->getNewPortName(mpd);
	if (portName.isEmpty()) return;
	if (synthRoute == NULL) synthRoute = startSynthRoute();
	MidiSession *midiSession = new MidiSession(this, midiDriver, portName, synthRoute);
	if (midiDriver->createPort(mpd, midiSession)) {
		synthRoute->addMidiSession(midiSession);
	} else {
		deleteMidiSession(midiSession);
	}
}

void Master::deleteMidiPort(MidiSession *midiSession) {
	midiDriver->deletePort(midiSession);
	deleteMidiSession(midiSession);
}

void Master::setMidiPortProperties(MidiPropertiesDialog *mpd, MidiSession *midiSession) {
	midiDriver->setPortProperties(mpd, midiSession);
}
