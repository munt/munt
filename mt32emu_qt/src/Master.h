#ifndef MASTER_H
#define MASTER_H

#include <QObject>

#include "SynthRoute.h"

class MidiDriver;
class MidiSession;

class Master : public QObject {
	Q_OBJECT
private:
	static Master *INSTANCE;

	QList<SynthRoute *> synthRoutes;
	QList<AudioDevice *> audioDevices;
	QDir romDir;

	Master();

public:
	MidiSession *createMidiSession(MidiDriver *midiDriver, QString name);
	void deleteMidiSession(MidiSession *midiSession);
	void addAudioDevice(AudioDevice *audioDevice);
	void removeAudioDevice(AudioDevice *audioDevice);
	// May only be called from the application thread
	const QList<AudioDevice *> getAudioDevices() const;
	void setROMDir(QDir romDir);
	QDir getROMDir();

	static Master *getInstance();

	// These two methods are only intended to be called at application startup/shutdown
	static void init();
	static void deinit();

private slots:
	void reallyCreateMidiSession(MidiSession **returnVal, MidiDriver *midiDriver, QString name);
	void reallyDeleteMidiSession(MidiSession *midiSession);
	void reallyAddAudioDevice(AudioDevice *audioDevice);
	void reallyRemoveAudioDevice(AudioDevice *audioDevice);

signals:
	void synthRouteAdded(SynthRoute *route);
	void synthRouteRemoved(SynthRoute *route);
	void audioDeviceAdded(AudioDevice *audioDevice);
	void audioDeviceRemoved(AudioDevice *audioDevice);
};

#endif
