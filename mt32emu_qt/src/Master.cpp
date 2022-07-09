/* Copyright (C) 2011-2022 Jerome Fisher, Sergey V. Mikayev
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
#include "MidiPropertiesDialog.h"

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
#ifdef WITH_JACK_AUDIO_DRIVER
#include "audiodrv/JACKAudioDriver.h"
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

#ifdef WITH_JACK_MIDI_DRIVER
#include "mididrv/JACKMidiDriver.h"
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

#ifdef WITH_JACK_MIDI_DRIVER
	jackMidiDriver->stop();
	delete jackMidiDriver;
	jackMidiDriver = NULL;
#endif

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
#ifdef WITH_JACK_AUDIO_DRIVER
	audioDrivers.append(new JACKAudioDriver(this));
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

#ifdef WITH_JACK_MIDI_DRIVER
	jackMidiDriver = new JACKMidiDriver(this);
#endif
}

void Master::startMidiProcessing() {
	if (midiDriver != NULL) {
		midiDriver->start();
	}

#ifdef WITH_JACK_MIDI_DRIVER
	jackMidiDriver->start();
#endif
}

Master *Master::getInstance() {
	return instance;
}

void Master::showCommandLineHelp() {
	QString appName = QFileInfo(QCoreApplication::arguments().at(0)).fileName();
	QMessageBox::information(NULL, "Information",
		"<h3>Command line format:</h3>"
		"<pre><code>" + appName + " [option...] [&lt;command&gt; [parameters...]]</code></pre>"
		"<h3>Options:</h3>"
		"<p><code>-profile &lt;profile-name&gt;</code></p>"
		"<p>override default synth profile with specified profile during this run only.</p>"
		"<p><code>-max_sessions &lt;number of sessions&gt;</code></p>"
		"<p>exit after this number of MIDI sessions are finished.</p>"
#ifdef WITH_JACK_MIDI_DRIVER
		"<p><code>-jack_midi_clients &lt;number of MIDI ports&gt;</code></p>"
		"<p>create the specified number of JACK MIDI ports that may be connected to any synth.</p>"
		"<p><code>-jack_sync_clients &lt;number of synchronous clients&gt;</code></p>"
		"<p>create the specified number of synchronous JACK clients, each of which consists of a dedicated synth,"
		" a MIDI input port (exclusive) and a couple of audio output ports.</p>"
#endif
		"<h3>Commands:</h3>"
		"<p><code>play &lt;SMF file...&gt;</code></p>"
		"<p>enqueue specified standard MIDI files into the internal MIDI player for playback and start playing.</p>"
		"<p><code>convert &lt;output file&gt; &lt;SMF file...&gt;</code></p>"
		"<p>convert specified standard MIDI files to a WAV/RAW wave output file and exit.</p>"
		"<p><code>reset &lt;scope&gt;</code></p>"
		"<p>restore settings within the defined scope to their factory defaults. The scope parameter may be one of:</p>"
		"<ul>"
		"<li><code>all</code>   - all settings, including any configured synth profiles;</li>"
		"<li><code>no-profiles</code> - all settings, except configured synth profiles;</li>"
		"<li><code>profiles</code> - delete all configured synth profiles;</li>"
		"<li><code>audio</code> - reset the default audio device.</li>"
		"</ul>"
		"<p><code>connect_midi &lt;MIDI port name...&gt;</code></p>"
		"<p>attempts to create one or more MIDI ports with the specified name(s) using the system MIDI driver. On Windows,"
		" opens available MIDI input devices with names that contain (case-insensitively) one of the specified port names.</p>"
	);
}

bool Master::processCommandLine(QStringList args) {
	int argIx = 1;
	QString command = args.at(argIx++);
	while (command.startsWith('-')) {
		if (QString::compare(command, "-profile", Qt::CaseInsensitive) == 0) {
			handleCLIOptionProfile(args, argIx);
		} else if (QString::compare(command, "-max_sessions", Qt::CaseInsensitive) == 0) {
			handleCLIOptionMaxSessions(args, argIx);
#ifdef WITH_JACK_MIDI_DRIVER
		} else if (QString::compare(command, "-jack_midi_clients", Qt::CaseInsensitive) == 0) {
			handleCLIOptionJackMidiClients(args, argIx);
		} else if (QString::compare(command, "-jack_sync_clients", Qt::CaseInsensitive) == 0) {
			handleCLIOptionJackSyncClients(args, argIx);
#endif
		} else {
			QMessageBox::warning(NULL, "Error", "Illegal command line option " + command + " specified.");
			showCommandLineHelp();
		}
		if (args.count() == argIx) return true;
		command = args.at(argIx++);
	}
	if (QString::compare(command, "play", Qt::CaseInsensitive) == 0) {
		handleCLICommandPlay(args, argIx);
	} else if (QString::compare(command, "convert", Qt::CaseInsensitive) == 0) {
		handleCLICommandConvert(args, argIx);
	} else if (QString::compare(command, "reset", Qt::CaseInsensitive) == 0) {
		return handleCLICommandReset(args, argIx);
	} else if (QString::compare(command, "connect_midi", Qt::CaseInsensitive) == 0) {
		handleCLIConnectMidi(args, argIx);
	} else {
		QMessageBox::warning(NULL, "Error", "Illegal command " + command + " specified in command line.");
		showCommandLineHelp();
	}
	return true;
}

void Master::handleCLIOptionProfile(const QStringList &args, int &argIx) {
	if (args.count() == argIx) {
		QMessageBox::warning(NULL, "Error", "The profile name must be specified in command line with \"-profile\" option.");
		showCommandLineHelp();
		return;
	}
	QString profile = args.at(argIx++);
	if (enumSynthProfiles().contains(profile, Qt::CaseInsensitive)) {
		synthProfileName = profile;
	} else {
		QMessageBox::warning(NULL, "Error", "The profile name specified in command line is invalid.\nOption \"-profile\" ignored.");
	}
}

void Master::handleCLIOptionMaxSessions(const QStringList &args, int &argIx) {
	if (args.count() == argIx) {
		QMessageBox::warning(NULL, "Error", "The maximum number of sessions must be specified in command line\n"
			"with \"-max_sessions\" option.");
		showCommandLineHelp();
		return;
	}
	maxSessions = args.at(argIx++).toUInt();
	if (maxSessions == 0) QMessageBox::warning(NULL, "Error", "The maximum number of sessions specified in command line is invalid.\n"
		"Option \"-max_sessions\" ignored.");
}

#ifdef WITH_JACK_MIDI_DRIVER

void Master::handleCLIOptionJackMidiClients(const QStringList &args, int &argIx) {
	if (args.count() == argIx) {
		QMessageBox::warning(NULL, "Error", "The number of JACK MIDI clients must be specified in command line\n"
			"with \"-jack_midi_clients\" option.");
		showCommandLineHelp();
		return;
	}
	int ports = args.at(argIx++).toInt();
	if (ports <= 0) {
		QMessageBox::warning(NULL, "Error", "The number of JACK MIDI clients specified in command line is invalid.\n"
			"Option \"-jack_midi_clients\" ignored.");
		return;
	}
	if (ports > 99) {
		QMessageBox::warning(NULL, "Error", "The number of JACK MIDI clients specified in command line is too big.\n"
			"Option \"-jack_midi_clients\" ignored.");
		return;
	}
	startPinnedSynthRoute();
	for (int i = 0; i < ports; i++) createJACKMidiPort(false);
}

void Master::handleCLIOptionJackSyncClients(const QStringList &args, int &argIx) {
	if (args.count() == argIx) {
		QMessageBox::warning(NULL, "Error", "The number of JACK sync clients must be specified in command line\n"
			"with \"-jack_sync_clients\" option.");
		showCommandLineHelp();
		return;
	}
	int ports = args.at(argIx++).toInt();
	if (ports <= 0) {
		QMessageBox::warning(NULL, "Error", "The number of JACK sync clients specified in command line is invalid.\n"
			"Option \"-jack_sync_clients\" ignored.");
		return;
	}
	if (ports > 99) {
		QMessageBox::warning(NULL, "Error", "The number of JACK sync clients specified in command line is too big.\n"
			"Option \"-jack_sync_clients\" ignored.");
		return;
	}
	for (int i = 0; i < ports; i++) createJACKMidiPort(true);
}

#endif

void Master::handleCLICommandPlay(const QStringList &args, int &argIx) {
	if (args.count() == argIx) {
		QMessageBox::warning(NULL, "Error", "The file list must be specified in command line with play command.");
		showCommandLineHelp();
		return;
	}
	emit playMidiFiles(args.mid(argIx));
}

void Master::handleCLICommandConvert(const QStringList &args, int &argIx) {
	if (args.count() > (argIx + 1)) {
		emit convertMidiFiles(args.mid(argIx));
		return;
	}
	QMessageBox::warning(NULL, "Error", "The file list must be specified in command line with convert command.");
	showCommandLineHelp();
}

bool Master::handleCLICommandReset(const QStringList &args, int &argIx) {
	if (args.count() != (argIx + 1)) {
		QMessageBox::warning(NULL, "Error", "The settings scope must be specified in command line with reset command.");
		showCommandLineHelp();
		return true;
	}
	QString scope = args.at(argIx);
	if (QString::compare(scope, "all", Qt::CaseInsensitive) == 0) {
		settings->clear();
		qDebug() << "All settings reset";
	} else if (QString::compare(scope, "no-profiles", Qt::CaseInsensitive) == 0) {
		settings->remove("Master");
		settings->remove("Audio");
		settings->remove("FloatingDisplay");
		qDebug() << "All settings except synth profiles reset";
	} else if (QString::compare(scope, "profiles", Qt::CaseInsensitive) == 0) {
		settings->remove("Profiles");
		qDebug() << "Synth profiles reset";
	} else if (QString::compare(scope, "audio", Qt::CaseInsensitive) == 0) {
		settings->remove("Master/DefaultAudioDriver");
		settings->remove("Master/DefaultAudioDevice");
		settings->remove("Audio");
		qDebug() << "Audio devices settings reset";
	} else {
		QMessageBox::warning(NULL, "Error", "The settings scope specified in command line is invalid.\n"
			"Command reset ignored.");
		showCommandLineHelp();
		return true;
	}
	QMessageBox::information(NULL, "Information", "Requested settings reset completed.\n"
		"Please, restart the application.");
	return false;
}

void Master::handleCLIConnectMidi(const QStringList &args, int &argIx) {
	if (args.count() == argIx) {
		QMessageBox::warning(NULL, "Error", "The MIDI port list must be specified in command line with connect_midi command.");
		showCommandLineHelp();
		return;
	}
	if (!midiDriver->canCreatePort()) {
		QMessageBox::warning(NULL, "Error", "The MIDI driver does not support creation of MIDI ports.");
		return;
	}

	startPinnedSynthRoute();
	QStringList portNames = args.mid(argIx);
	portNames.removeDuplicates();

	if (MidiDriver::PortNamingPolicy_RESTRICTED == midiDriver->getPortNamingPolicy()) {
		QStringList knownPortNames;
		midiDriver->getNewPortNameHint(knownPortNames);
		for (int knownPortIndex = 0; knownPortIndex < knownPortNames.size(); knownPortIndex++) {
			const QString &knownPortName = knownPortNames.at(knownPortIndex);
			foreach (const QString portName, portNames) {
				if (knownPortName.contains(portName, Qt::CaseInsensitive)) {
					createMidiPort(knownPortIndex, knownPortName);
					break;
				}
			}
			if (!midiDriver->canCreatePort()) break;
		}
		return;
	}

	if (MidiDriver::PortNamingPolicy_UNIQUE == midiDriver->getPortNamingPolicy()) {
		QStringList knownPortNames;
		midiDriver->getNewPortNameHint(knownPortNames);
		foreach (const QString knownPortName, knownPortNames) {
			portNames.removeOne(knownPortName);
		}
	}

	foreach (const QString portName, portNames) {
		createMidiPort(-1, portName);
		if (!midiDriver->canCreatePort()) break;
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

const QByteArray Master::getROMPathNameLocal(const QDir &romDir, const QString romFileName) {
	return QDir::toNativeSeparators(romDir.absoluteFilePath(romFileName)).toLocal8Bit();
}

void Master::findROMImages(const SynthProfile &synthProfile, const MT32Emu::ROMImage *&controlROMImage, const MT32Emu::ROMImage *&pcmROMImage) const {
	const MT32Emu::ROMImage *synthControlROMImage = NULL;
	const MT32Emu::ROMImage *synthPCMROMImage = NULL;
	for (int synthRouteIx = -1; synthRouteIx < synthRoutes.size(); synthRouteIx++) {
		if (controlROMImage != NULL && pcmROMImage != NULL) return;
		SynthProfile profile;
		if (synthRouteIx == -1) {
			if (audioFileWriterSynth == NULL) continue;
			audioFileWriterSynth->getSynthProfile(profile);
			audioFileWriterSynth->getROMImages(synthControlROMImage, synthPCMROMImage);
		} else {
			SynthRoute *synthRoute = synthRoutes.at(synthRouteIx);
			synthRoute->getSynthProfile(profile);
			synthRoute->getROMImages(synthControlROMImage, synthPCMROMImage);
		}
		if (synthProfile.romDir != profile.romDir) continue;
		if (controlROMImage == NULL && synthProfile.controlROMFileName == profile.controlROMFileName && synthProfile.controlROMFileName2 == profile.controlROMFileName2) controlROMImage = synthControlROMImage;
		if (pcmROMImage == NULL && synthProfile.pcmROMFileName == profile.pcmROMFileName && synthProfile.pcmROMFileName2 == profile.pcmROMFileName2) pcmROMImage = synthPCMROMImage;
	}
}

void Master::freeROMImages(const MT32Emu::ROMImage *&controlROMImage, const MT32Emu::ROMImage *&pcmROMImage) const {
	if (controlROMImage == NULL && pcmROMImage == NULL) return;
	bool controlROMInUse = false;
	bool pcmROMInUse = false;
	const MT32Emu::ROMImage *synthControlROMImage = NULL;
	const MT32Emu::ROMImage *synthPCMROMImage = NULL;
	if (audioFileWriterSynth != NULL) {
		audioFileWriterSynth->getROMImages(synthControlROMImage, synthPCMROMImage);
		controlROMInUse = synthControlROMImage == controlROMImage;
		pcmROMInUse = synthPCMROMImage == pcmROMImage;
		if (controlROMInUse && pcmROMInUse) return;
	}
	foreach (SynthRoute *synthRoute, synthRoutes) {
		synthRoute->getROMImages(synthControlROMImage, synthPCMROMImage);
		controlROMInUse = controlROMInUse || (synthControlROMImage == controlROMImage);
		pcmROMInUse = pcmROMInUse || (synthPCMROMImage == pcmROMImage);
		if (controlROMInUse && pcmROMInUse) return;
	}
	if (!controlROMInUse && controlROMImage != NULL) {
		if (controlROMImage->isFileUserProvided()) delete controlROMImage->getFile();
		MT32Emu::ROMImage::freeROMImage(controlROMImage);
		controlROMImage = NULL;
	}
	if (!pcmROMInUse && pcmROMImage != NULL) {
		if (pcmROMImage->isFileUserProvided()) delete pcmROMImage->getFile();
		MT32Emu::ROMImage::freeROMImage(pcmROMImage);
		pcmROMImage = NULL;
	}
}

bool Master::handleROMSLoadFailed(QString usedSynthProfileName) {
	if (usedSynthProfileName.isEmpty() || usedSynthProfileName == synthProfileName) {
		bool recoveryAttempted = false;
		emit romsLoadFailed(recoveryAttempted);
		return recoveryAttempted;
	}
	return false;
}

QString Master::getDefaultROMSearchPath() {
#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)
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
#else
	return "./roms/";
#endif
}

void Master::loadSynthProfile(SynthProfile &synthProfile, QString name) {
	if (name.isEmpty()) name = synthProfileName;
	settings->beginGroup("Profiles/" + name);
	QString romPath = settings->value("romDir", "").toString();
	synthProfile.romDir.setPath(romPath.isEmpty() ? getDefaultROMSearchPath(): romPath);
	synthProfile.controlROMFileName = settings->value("controlROM", "MT32_CONTROL.ROM").toString();
	synthProfile.controlROMFileName2 = settings->value("controlROM2", "").toString();
	synthProfile.pcmROMFileName = settings->value("pcmROM", "MT32_PCM.ROM").toString();
	synthProfile.pcmROMFileName2 = settings->value("pcmROM2", "").toString();
	synthProfile.emuDACInputMode = (MT32Emu::DACInputMode)settings->value("emuDACInputMode", MT32Emu::DACInputMode_NICE).toInt();
	synthProfile.midiDelayMode = (MT32Emu::MIDIDelayMode)settings->value("midiDelayMode", MT32Emu::MIDIDelayMode_DELAY_SHORT_MESSAGES_ONLY).toInt();
	synthProfile.analogOutputMode = (MT32Emu::AnalogOutputMode)settings->value("analogOutputMode", MT32Emu::AnalogOutputMode_ACCURATE).toInt();
	synthProfile.rendererType = (MT32Emu::RendererType)settings->value("rendererType", MT32Emu::RendererType_BIT16S).toInt();
	synthProfile.partialCount = settings->value("partialCount", MT32Emu::DEFAULT_MAX_PARTIALS).toInt();
	synthProfile.reverbCompatibilityMode = (ReverbCompatibilityMode)settings->value("reverbCompatibilityMode", ReverbCompatibilityMode_DEFAULT).toInt();
	synthProfile.reverbEnabled = settings->value("reverbEnabled", true).toBool();
	synthProfile.reverbOverridden = settings->value("reverbOverridden", false).toBool();
	synthProfile.reverbMode = settings->value("reverbMode", 0).toInt();
	synthProfile.reverbTime = settings->value("reverbTime", 5).toInt();
	synthProfile.reverbLevel = settings->value("reverbLevel", 3).toInt();
#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)
	synthProfile.outputGain = settings->value("outputGain", 1.0).toFloat();
	synthProfile.reverbOutputGain = settings->value("reverbOutputGain", 1.0).toFloat();
#else
	synthProfile.outputGain = (float)settings->value("outputGain", 1.0).toDouble();
	synthProfile.reverbOutputGain = (float)settings->value("reverbOutputGain", 1.0).toDouble();
#endif
	synthProfile.reversedStereoEnabled = settings->value("reversedStereoEnabled", false).toBool();
	synthProfile.engageChannel1OnOpen = settings->value("engageChannel1OnOpen", false).toBool();
	synthProfile.niceAmpRamp = settings->value("niceAmpRamp", true).toBool();
	synthProfile.nicePanning = settings->value("nicePanning", false).toBool();
	synthProfile.nicePartialMixing = settings->value("nicePartialMixing", false).toBool();
	synthProfile.displayCompatibilityMode = DisplayCompatibilityMode(settings->value("displayCompatibilityMode", DisplayCompatibilityMode_DEFAULT).toInt());
	settings->endGroup();
}

void Master::storeSynthProfile(const SynthProfile &synthProfile, QString name) const {
	if (name.isEmpty()) name = synthProfileName;
	settings->beginGroup("Profiles/" + name);
	settings->setValue("romDir", synthProfile.romDir.absolutePath());
	settings->setValue("controlROM", synthProfile.controlROMFileName);
	settings->setValue("controlROM2", synthProfile.controlROMFileName2);
	settings->setValue("pcmROM", synthProfile.pcmROMFileName);
	settings->setValue("pcmROM2", synthProfile.pcmROMFileName2);
	settings->setValue("emuDACInputMode", synthProfile.emuDACInputMode);
	settings->setValue("midiDelayMode", synthProfile.midiDelayMode);
	settings->setValue("analogOutputMode", synthProfile.analogOutputMode);
	settings->setValue("rendererType", synthProfile.rendererType);
	settings->setValue("partialCount", synthProfile.partialCount);
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
	settings->setValue("niceAmpRamp", synthProfile.niceAmpRamp);
	settings->setValue("nicePanning", synthProfile.nicePanning);
	settings->setValue("nicePartialMixing", synthProfile.nicePartialMixing);
	settings->setValue("displayCompatibilityMode", synthProfile.displayCompatibilityMode);
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
	if (settings->value("Master/startPinnedSynthRoute", false).toBool()) {
		setPinned(startSynthRoute());
	}
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
			emit synthRouteAdded(synthRoute, audioDevice, true);
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
		qDebug() << "Exiting due to maximum number of sessions finished";
		emit maxSessionsFinished();
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
#ifdef WITH_JACK_MIDI_DRIVER
	if (jackMidiDriver->canDeletePort(midiSession)) return true;
#endif
	return midiDriver->canDeletePort(midiSession);
}

bool Master::canReconnectMidiPort(MidiSession *midiSession) {
	return midiDriver->canReconnectPort(midiSession);
}

void Master::configureMidiPropertiesDialog(MidiPropertiesDialog &mpd) {
	MidiDriver::PortNamingPolicy portNamingPolicy = midiDriver->getPortNamingPolicy();
	mpd.setMidiPortListEnabled(portNamingPolicy != MidiDriver::PortNamingPolicy_UNIQUE);
	mpd.setMidiPortNameEditorEnabled(portNamingPolicy != MidiDriver::PortNamingPolicy_RESTRICTED);
}

void Master::createMidiPort(MidiPropertiesDialog &mpd, SynthRoute *synthRoute) {
	QStringList knownPortNames;
	QString portNameHint = midiDriver->getNewPortNameHint(knownPortNames);
	mpd.setMidiList(knownPortNames);
	mpd.setMidiPortName(portNameHint);
	if (mpd.exec() != QDialog::Accepted) return;
	QString portName = mpd.getMidiPortName();
	if (portName.isEmpty()) return;
	createMidiPort(mpd.getCurrentMidiPortIndex(), portName, synthRoute);
}

void Master::createMidiPort(int portIx, const QString &portName, SynthRoute *synthRoute) {
	if (synthRoute == NULL) synthRoute = startSynthRoute();
	MidiSession *midiSession = new MidiSession(this, midiDriver, portName, synthRoute);
	if (midiDriver->createPort(portIx, portName, midiSession)) {
		synthRoute->addMidiSession(midiSession);
	} else {
		deleteMidiSession(midiSession);
	}
}

void Master::deleteMidiPort(MidiSession *midiSession) {
#ifdef WITH_JACK_MIDI_DRIVER
	if (jackMidiDriver->canDeletePort(midiSession)) {
		jackMidiDriver->deletePort(midiSession);
		deleteMidiSession(midiSession);
		return;
	}
#endif
	midiDriver->deletePort(midiSession);
	deleteMidiSession(midiSession);
}

void Master::reconnectMidiPort(MidiPropertiesDialog &mpd, MidiSession *midiSession) {
	QStringList knownPortNames;
	QString portNameHint = midiDriver->getNewPortNameHint(knownPortNames);
	if (portNameHint.isEmpty()) portNameHint = midiSession->getName();
	mpd.setMidiList(knownPortNames, knownPortNames.indexOf(portNameHint));
	mpd.setMidiPortName(portNameHint);
	if (mpd.exec() != QDialog::Accepted) return;
	QString portName = mpd.getMidiPortName();
	if (portName.isEmpty() || portName == midiSession->getName()) return;
	midiDriver->reconnectPort(mpd.getCurrentMidiPortIndex(), portName, midiSession);
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

QStringList Master::parseMidiListFromUrls(const QList<QUrl> urls) {
	QStringList fileNames;
	for (int i = 0; i < urls.size(); i++) {
		QUrl url = urls.at(i);
		if (url.scheme() == "file") {
			fileNames += parseMidiListFromPathName(url.toLocalFile());
		}
	}
	return fileNames;
}

QStringList Master::parseMidiListFromPathName(const QString pathName) {
	QStringList fileNames;
	QDir dir = QDir(pathName);
	if (dir.exists()) {
		if (!dir.isReadable()) return fileNames;
		QStringList syxFileNames = dir.entryList(QStringList() << "*.syx");
		QStringList midiFileNames = dir.entryList(QStringList() << "*.mid" << "*.smf");
		foreach (QString midiFileName, syxFileNames + midiFileNames) {
			fileNames += dir.absoluteFilePath(midiFileName);
		}
		return fileNames;
	}
	if (pathName.endsWith(".pls", Qt::CaseInsensitive)) {
		QFile listFile(pathName);
		if (!listFile.open(QIODevice::ReadOnly)) {
			qDebug() << "Couldn't open file" << pathName + ", ignored";
			return fileNames;
		}
		QTextStream listStream(&listFile);
		while (!listStream.atEnd()) {
			QString s = listStream.readLine();
			if (!s.isEmpty()) {
				fileNames += s;
			}
		}
	} else {
		fileNames += pathName;
	}
	return fileNames;
}

#ifdef WITH_JACK_MIDI_DRIVER
bool Master::createJACKMidiPort(bool exclusive) {
	return static_cast<JACKMidiDriver *>(jackMidiDriver)->createJACKPort(exclusive);
}

void Master::deleteJACKMidiPort(MidiSession *midiSession) {
	emit jackMidiPortDeleted(midiSession);
}

MidiSession *Master::createExclusiveJACKMidiPort(QString portName) {
	getAudioDevices();
	if (!audioDevices.isEmpty()) {
		const AudioDevice *jackAudioDevice = findAudioDevice("jackaudio", "Default");
		if (jackAudioDevice->driver.id == "jackaudio") {
			SynthRoute *synthRoute = new SynthRoute(this);
			synthRoute->setAudioDevice(jackAudioDevice);
			synthRoutes.append(synthRoute);
			emit synthRouteAdded(synthRoute, jackAudioDevice, false);
			MidiSession *midiSession = new MidiSession(this, jackMidiDriver, portName, synthRoute);
			synthRoute->enableExclusiveMidiMode(midiSession);
			if (synthRoute->open(JACKAudioDefaultDevice::startAudioStream)) {
				// This must be done asynchronously
				connect(synthRoute, SIGNAL(exclusiveMidiSessionRemoved(MidiSession *)), jackMidiDriver, SLOT(onJACKMidiPortDeleted(MidiSession *)), Qt::QueuedConnection);
				return midiSession;
			}
			deleteMidiSession(midiSession);
		}
	}
	return NULL;
}
#endif
