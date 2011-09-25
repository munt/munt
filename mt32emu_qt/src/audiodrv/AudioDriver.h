#ifndef AUDIO_DRIVER_H
#define AUDIO_DRIVER_H

#include <QObject>
#include <QtGlobal>
#include <QList>
#include <QString>
#include <QMetaType>

class AudioDriver;
class QSynth;

class AudioStream {
public:
	//virtual void suspend() = 0;
	//virtual void unsuspend() = 0;
	virtual ~AudioStream() {};
};

class AudioDevice {
public:
	const AudioDriver *driver;
	// id must be unique within the driver and as permanent as possible -
	// it will be stored and retrieved from settings.
	const QString id;
	const QString name;
	AudioDevice(AudioDriver *driver, QString id, QString name);
	virtual ~AudioDevice() {};
	virtual AudioStream *startAudioStream(QSynth *synth, unsigned int sampleRate) const = 0;
};

Q_DECLARE_METATYPE(AudioDevice*);

class AudioDriver {
public:
	// id must be unique within the application and permanent -
	// it will be stored and retrieved from settings. Preferably lowercase.
	const QString id;
	// name is the English human-readable name to be used in the GUI.
	const QString name;
	AudioDriver(QString useID, QString useName);
	virtual ~AudioDriver() {};
	virtual QList<AudioDevice *> getDeviceList() = 0;
};

#endif
