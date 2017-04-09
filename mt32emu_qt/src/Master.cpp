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

#include <QSystemTrayIcon>
#include <QDropEvent>
#include <QMessageBox>

#include "Master.h"
#include "MasterClock.h"
#include "MidiSession.h"

#ifdef WITH_WINMM_AUDIO_DRIVER
#include "audiodrv/WinMMAudioDriver.h"
#endif
#ifdef WITH_COREAUDIO_DRIVER
#include "audiodrv/CoreAudioDriver.h"
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

static const int ACTUAL_SETTINGS_VERSION = 2;

static Master *instance = NULL;

static void migrateSettings(QSettings &settings, const int fromVersion) {
	qDebug() << "Migrating settings from version" << fromVersion << "to version" << ACTUAL_SETTINGS_VERSION;
	switch (fromVersion) {
	case 1:
		AudioDriver::migrateAudioSettingsFromVersion1();
		settings.setValue("Master/settingsVersion", ACTUAL_SETTINGS_VERSION);
		break;
	default:
		qDebug() << "Migration failed";
		QMessageBox::warning(NULL, "Unsupported settings version",
			"Unable to load application settings of unsupported version " + QString().setNum(fromVersion) + ".\n"
			"Please, check the settings!");
		break;
	}
}

Master::Master() {
	if (instance != NULL) {
		qFatal("Master already instantiated!");
		// Do nothing if ignored
		return;
	}
	instance = this;
	maxSessions = 0;

	moveToThread(QCoreApplication::instance()->thread());

	MasterClock::init();

	settings = new QSettings("muntemu.org", "Munt mt32emu-qt");
	int settingsVersion = settings->value("Master/settingsVersion", 1).toInt();
	if (settingsVersion != ACTUAL_SETTINGS_VERSION) {
		migrateSettings(*settings, settingsVersion);
	}

	synthProfileName = settings->value("Master/defaultSynthProfile", "default").toString();

	trayIcon = NULL;
	defaultAudioDriverId = settings->value("Master/DefaultAudioDriver").toString();
	defaultAudioDeviceName = settings->value("Master/DefaultAudioDevice").toString();

	initAudioDrivers();
	initMidiDrivers();
	lastAudioDeviceScan = -4 * MasterClock::NANOS_PER_SECOND;
	getAudioDevices();
	pinnedSynthRoute = NULL;
	audioFileWriterSynth = NULL;

	qRegisterMetaType<MidiDriver *>("MidiDriver*");
	qRegisterMetaType<MidiSession *>("MidiSession*");
	qRegisterMetaType<MidiSession **>("MidiSession**");
	qRegisterMetaType<SynthState>("SynthState");
}

Master::~Master() {
	qDebug() << "Shutting down Master...";

	delete settings;

	if (midiDriver != NULL) {
		midiDriver->stop();
		delete midiDriver;
		midiDriver = NULL;
	}

	QMutableListIterator<SynthRoute *> synthRouteIt(synthRoutes);
	while (synthRouteIt.hasNext()) {
		delete synthRouteIt.next();
		synthRouteIt.remove();
	}

	QMutableListIterator<const AudioDevice *> audioDeviceIt(audioDevices);
	while (audioDeviceIt.hasNext()) {
		delete audioDeviceIt.next();
		audioDeviceIt.remove();
	}

	QMutableListIterator<AudioDriver *> audioDriverIt(audioDrivers);
	while (audioDriverIt.hasNext()) {
		delete audioDriverIt.next();
		audioDriverIt.remove();
	}

	MasterClock::cleanup();
}

void Master::initAudioDrivers() {
#ifdef WITH_WINMM_AUDIO_DRIVER
	audioDrivers.append(new WinMMAudioDriver(this));
#endif
#ifdef WITH_COREAUDIO_DRIVER
	audioDrivers.append(new CoreAudioDriver(this));
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
	return instance;
}

void Master::showCommandLineHelp() {
	QString appName = QFileInfo(QCoreApplication::arguments().at(0)).fileName();
	QMessageBox::information(NULL, "Information",
		"Command line format:\n"
		"	" + appName + " [option...] [<command> file...]\n"
		"\n"
		"Options:\n"
		"-profile <profile name>\n"
		"	override default synth profile with specified profile\n"
		"	during this run only.\n"
		"-max_sessions <number of sessions>\n"
		"	exit after this number of MIDI sessions are finished.\n"
		"\n"
		"Commands:\n"
		"play <SMF file...>\n"
		"	enqueue specified standard MIDI files into the internal\n"
		"	MIDI player for playback and start playing.\n"
		"convert <output file> <SMF file...>\n"
		"	convert specified standard MIDI files to a WAV/RAW wave\n"
		"	output file and exit.\n"
	);
}

void Master::processCommandLine(QStringList args) {
	int argIx = 1;
	QString command = args.at(argIx++);
	while (command.startsWith('-')) {
		if (QString::compare(command, "-profile", Qt::CaseInsensitive) == 0) {
			if (args.count() > argIx) {
				QString profile = args.at(argIx++);
				if (enumSynthProfiles().contains(profile, Qt::CaseInsensitive)) {
					synthProfileName = profile;
				} else {
					QMessageBox::warning(NULL, "Error", "The profile name specified in command line is invalid.\nOption \"-profile\" ignored.");
				}
			} else {
				QMessageBox::warning(NULL, "Error", "The profile name must be specified in command line with \"-profile\" option.");
				showCommandLineHelp();
			}
		} else if (QString::compare(command, "-max_sessions", Qt::CaseInsensitive) == 0) {
			if (args.count() > argIx) {
				maxSessions = args.at(argIx++).toUInt();
				if (maxSessions == 0) QMessageBox::warning(NULL, "Error", "The maximum number of sessions specified in command line is invalid.\nOption \"-max_sessions\" ignored.");
			} else {
				QMessageBox::warning(NULL, "Error", "The maximum number of sessions must be specified in command line with \"-max_sessions\" option.");
				showCommandLineHelp();
			}
		} else {
			QMessageBox::warning(NULL, "Error", "Illegal command line option " + command + " specified.");
			showCommandLineHelp();
		}
		if (args.count() == argIx) return;
		command = args.at(argIx++);
	}
	if (args.count() == argIx) {
		QMessageBox::warning(NULL, "Error", "The file list must be specified in command line with a command.");
		showCommandLineHelp();
		return;
	}
	QStringList files = args.mid(argIx);
	if (QString::compare(command, "play", Qt::CaseInsensitive) == 0) {
		emit playMidiFiles(files);
	} else if (QString::compare(command, "convert", Qt::CaseInsensitive) == 0) {
		if (args.count() > (argIx + 1)) {
			emit convertMidiFiles(files);
		} else {
			QMessageBox::warning(NULL, "Error", "The file list must be specified in command line with " + command + " command.");
			showCommandLineHelp();
		}
	} else {
		QMessageBox::warning(NULL, "Error", "Illegal command " + command + " specified in command line.");
		showCommandLineHelp();
	}
}

void Master::setDefaultAudioDevice(QString driverId, QString name) {
	defaultAudioDriverId = driverId;
	defaultAudioDeviceName = name;
	settings->setValue("Master/DefaultAudioDriver", driverId);
	settings->setValue("Master/DefaultAudioDevice", name);
}

const QList<const AudioDevice *> Master::getAudioDevices() {
	MasterClockNanos nanosNow = MasterClock::getClockNanos();
	if ((nanosNow - lastAudioDeviceScan) > 3 * MasterClock::NANOS_PER_SECOND) {
		lastAudioDeviceScan = nanosNow;
		qDebug() << "Scanning audio devices ...";
		audioDevices.clear();
		QListIterator<AudioDriver *> audioDriverIt(audioDrivers);
		while(audioDriverIt.hasNext()) {
			AudioDriver *audioDriver = audioDriverIt.next();
			audioDevices.append(audioDriver->createDeviceList());
		}
	}
	return audioDevices;
}

const AudioDevice *Master::findAudioDevice(QString driverId, QString name) const {
	QListIterator<const AudioDevice *> audioDeviceIt(audioDevices);
	while(audioDeviceIt.hasNext()) {
		const AudioDevice *audioDevice = audioDeviceIt.next();
		if (driverId == audioDevice->driver.id && name == audioDevice->name) {
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

	// FIXME: Probably there need to be a proper ROMImage cache with allocation counters
	foreach (SynthRoute *synthRoute, synthRoutes) {
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
	SynthProfile synthProfile;
	if (audioFileWriterSynth != NULL) {
		audioFileWriterSynth->getSynthProfile(synthProfile);
		controlROMInUse = controlROMInUse || (synthProfile.controlROMImage == controlROMImage);
		pcmROMInUse = pcmROMInUse || (synthProfile.pcmROMImage == pcmROMImage);
	}
	foreach (SynthRoute *synthRoute, synthRoutes) {
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

QString Master::getDefaultROMSearchPath() {
	QString defaultPath;
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	if (env.contains("USERPROFILE")) {
		defaultPath = env.value("USERPROFILE");
	} else if (env.contains("HOME")) {
		defaultPath = env.value("HOME");
	} else {
		defaultPath = ".";
	}
	return defaultPath + "/roms/";
}

void Master::loadSynthProfile(SynthProfile &synthProfile, QString name) {
	static bool romNotSetReported = false;
	if (name.isEmpty()) name = synthProfileName;
	settings->beginGroup("Profiles/" + name);

	QString romPath = settings->value("romDir", "").toString();
	synthProfile.romDir.setPath(romPath.isEmpty() ? getDefaultROMSearchPath(): romPath);
	synthProfile.controlROMFileName = settings->value("controlROM", "MT32_CONTROL.ROM").toString();
	synthProfile.pcmROMFileName = settings->value("pcmROM", "MT32_PCM.ROM").toString();
	synthProfile.emuDACInputMode = (MT32Emu::DACInputMode)settings->value("emuDACInputMode", MT32Emu::DACInputMode_NICE).toInt();
	synthProfile.midiDelayMode = (MT32Emu::MIDIDelayMode)settings->value("midiDelayMode", MT32Emu::MIDIDelayMode_DELAY_SHORT_MESSAGES_ONLY).toInt();
	synthProfile.analogOutputMode = (MT32Emu::AnalogOutputMode)settings->value("analogOutputMode", MT32Emu::AnalogOutputMode_ACCURATE).toInt();
	synthProfile.rendererType = (MT32Emu::RendererType)settings->value("rendererType", MT32Emu::RendererType_BIT16S).toInt();
	synthProfile.reverbCompatibilityMode = (ReverbCompatibilityMode)settings->value("reverbCompatibilityMode", ReverbCompatibilityMode_DEFAULT).toInt();
	synthProfile.reverbEnabled = settings->value("reverbEnabled", true).toBool();
	synthProfile.reverbOverridden = settings->value("reverbOverridden", false).toBool();
	synthProfile.reverbMode = settings->value("reverbMode", 0).toInt();
	synthProfile.reverbTime = settings->value("reverbTime", 5).toInt();
	synthProfile.reverbLevel = settings->value("reverbLevel", 3).toInt();
	synthProfile.outputGain = settings->value("outputGain", 1.0f).toFloat();
	synthProfile.reverbOutputGain = settings->value("reverbOutputGain", 1.0f).toFloat();
	synthProfile.reversedStereoEnabled = settings->value("reversedStereoEnabled", false).toBool();
	synthProfile.engageChannel1OnOpen = settings->value("engageChannel1OnOpen", false).toBool();
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
	settings->setValue("midiDelayMode", synthProfile.midiDelayMode);
	settings->setValue("analogOutputMode", synthProfile.analogOutputMode);
	settings->setValue("rendererType", synthProfile.rendererType);
	settings->setValue("reverbCompatibilityMode", synthProfile.reverbCompatibilityMode);
	settings->setValue("reverbEnabled", synthProfile.reverbEnabled);
	settings->setValue("reverbOverridden", synthProfile.reverbOverridden);
	settings->setValue("reverbMode", synthProfile.reverbMode);
	settings->setValue("reverbTime", synthProfile.reverbTime);
	settings->setValue("reverbLevel", synthProfile.reverbLevel);
	settings->setValue("outputGain", QString().setNum(synthProfile.outputGain));
	settings->setValue("reverbOutputGain", QString().setNum(synthProfile.reverbOutputGain));
	settings->setValue("reversedStereoEnabled", synthProfile.reversedStereoEnabled);
	settings->setValue("engageChannel1OnOpen", synthProfile.engageChannel1OnOpen);
	settings->endGroup();
}

bool Master::isPinned(const SynthRoute *synthRoute) const {
	return synthRoute == pinnedSynthRoute;
}

void Master::setPinned(SynthRoute *synthRoute) {
	if (pinnedSynthRoute == synthRoute) return;
	settings->setValue("Master/startPinnedSynthRoute", synthRoute != NULL);
	pinnedSynthRoute = synthRoute;
	emit synthRoutePinned();
}

void Master::startPinnedSynthRoute() {
	if (settings->value("Master/startPinnedSynthRoute", false).toBool())
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
	Master::trayIcon = trayIcon;
}

void Master::showBalloon(const QString &title, const QString &text) {
	if (trayIcon != NULL) {
		trayIcon->showMessage(title, text);
	}
}

void Master::updateMainWindowTitleContribution(const QString &titleContribution) {
	QString title("Munt: MT-32 Emulator");
	if (!titleContribution.isEmpty()) title += " - " + titleContribution;
	emit mainWindowTitleUpdated(title);
}

void Master::createMidiSession(MidiSession **returnVal, MidiDriver *midiDriver, QString name) {
	SynthRoute *synthRoute = startSynthRoute();
	MidiSession *midiSession = new MidiSession(this, midiDriver, name, synthRoute);
	synthRoute->addMidiSession(midiSession);
	*returnVal = midiSession;
}

void Master::deleteMidiSession(MidiSession *midiSession) {
	if ((maxSessions > 0) && (--maxSessions == 0)) {
		qDebug() << "Exitting due to maximum number of sessions finished";
		maxSessionsFinished();
	}
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

// A quick hack to prevent ROMImages used in SMF converter from being freed
// when closing another synth which uses the same ROMImages
void Master::setAudioFileWriterSynth(const QSynth *qSynth) {
	audioFileWriterSynth = qSynth;
}

void Master::isSupportedDropEvent(QDropEvent *e) {
	if (!e->mimeData()->hasUrls()) {
		e->ignore();
		return;
	}
	if ((e->possibleActions() & Qt::CopyAction) == 0) {
		e->ignore();
		return;
	}
	if (e->proposedAction() != Qt::CopyAction) {
		e->setDropAction(Qt::CopyAction);
	}
	e->accept();
}
