#ifndef MASTER_H
#define MASTER_H

#include <QObject>

#include "SynthRoute.h"

class AudioDriver;
class MidiDriver;
class MidiSession;
class QSystemTrayIcon;
class MidiPropertiesDialog;
class QDropEvent;

class Master : public QObject {
	Q_OBJECT
private:
	QList<SynthRoute *> synthRoutes;
	QList<AudioDriver *> audioDrivers;
	QList<const AudioDevice *> audioDevices;
	MidiDriver *midiDriver;
	SynthRoute *pinnedSynthRoute;
	const QSynth *audioFileWriterSynth;

	QSettings *settings;
	QString synthProfileName;
	QSystemTrayIcon *trayIcon;
	QString defaultAudioDriverId;
	QString defaultAudioDeviceName;
	qint64 lastAudioDeviceScan;

	bool stopping;

	void init();
	~Master();

	void initAudioDrivers();
	void initMidiDrivers();
	const AudioDevice *findAudioDevice(QString driverId, QString name) const;
	const QString getROMPathName(const QDir &romDir, QString romFileName) const;
	void makeROMImages(SynthProfile &synthProfile);
	SynthRoute *startSynthRoute();

public:
	static Master *getInstance();
	static void isSupportedDropEvent(QDropEvent *e);

	// May only be called from the application thread
	const QList<const AudioDevice *> getAudioDevices();
	void setDefaultAudioDevice(QString driverId, QString name);
	void setTrayIcon(QSystemTrayIcon *trayIcon);
	QString getDefaultSynthProfileName();
	bool setDefaultSynthProfileName(QString name);
	const QStringList enumSynthProfiles() const;
	void loadSynthProfile(SynthProfile &synthProfile, QString name);
	void storeSynthProfile(const SynthProfile &synthProfile, QString name) const;
	void freeROMImages(const MT32Emu::ROMImage* &controlROMImage, const MT32Emu::ROMImage* &pcmROMImage);
	QSystemTrayIcon *getTrayIcon() const;
	QSettings *getSettings() const;
	bool isPinned(const SynthRoute *synthRoute) const;
	void setPinned(SynthRoute *synthRoute);
	void startPinnedSynthRoute();
	void startMidiProcessing();
	void processCommandLine(QStringList args);
	bool canCreateMidiPort();
	bool canDeleteMidiPort(MidiSession *midiSession);
	bool canSetMidiPortProperties(MidiSession *midiSession);
	void createMidiPort(MidiPropertiesDialog *mpd, SynthRoute *synthRoute = NULL);
	void deleteMidiPort(MidiSession *midiSession);
	void setMidiPortProperties(MidiPropertiesDialog *mpd, MidiSession *midiSession);
	QString getDefaultROMSearchPath();
	void setAudioFileWriterSynth(const QSynth *);

private slots:
	void createMidiSession(MidiSession **returnVal, MidiDriver *midiDriver, QString name);
	void deleteMidiSession(MidiSession *midiSession);
	void showBalloon(const QString &title, const QString &text);
	void updateMainWindowTitleContribution(const QString &titleContribution);
	void aboutToQuit();

signals:
	void synthRouteAdded(SynthRoute *route, const AudioDevice *audioDevice);
	void synthRouteRemoved(SynthRoute *route);
	void synthRoutePinned();
	void romsNotSet();
	void playMidiFiles(const QStringList &);
	void convertMidiFiles(const QStringList &);
	void mainWindowTitleUpdated(const QString &);
};

#endif
