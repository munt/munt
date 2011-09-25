#ifndef MASTER_H
#define MASTER_H

#include <QObject>

#include "SynthRoute.h"

class AudioDriver;
class MidiDriver;
class MidiSession;

class Master : public QObject {
	Q_OBJECT
private:
	static Master *INSTANCE;

	QList<SynthRoute *> synthRoutes;
	QList<AudioDriver *> audioDrivers;
	QList<AudioDevice *> audioDevices;
	MidiDriver *midiDriver;

	QSettings *settings;
	QDir romDir;

	Master();
	void initAudioDrivers();
	void initMidiDrivers();
	const AudioDevice *findAudioDevice(QString driverId, QString name) const;

public:
	MidiSession *createMidiSession(MidiDriver *midiDriver, QString name);
	void deleteMidiSession(MidiSession *midiSession);
	// May only be called from the application thread
	const QList<AudioDevice *> getAudioDevices();
	void setROMDir(QDir romDir);
	QDir getROMDir();
	QSettings *getSettings();

	static Master *getInstance();

	// These two methods are only intended to be called at application startup/shutdown
	static void init();
	static void deinit();

private slots:
	void reallyCreateMidiSession(MidiSession **returnVal, MidiDriver *midiDriver, QString name);
	void reallyDeleteMidiSession(MidiSession *midiSession);

signals:
	void synthRouteAdded(SynthRoute *route, const AudioDevice *audioDevice);
	void synthRouteRemoved(SynthRoute *route);
	void audioDeviceAdded(AudioDevice *audioDevice);
	void audioDeviceRemoved(AudioDevice *audioDevice);
};

#endif
