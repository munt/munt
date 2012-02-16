#ifndef MIDI_DRIVER_H
#define MIDI_DRIVER_H

#include <QObject>

#include "../Master.h"

class SynthRoute;

class MidiDriver : public QObject {
	Q_OBJECT
protected:
	Master *master;
	QString name;
	QList<MidiSession *>midiSessions;

	void setName(const QString &name);
	MidiSession *createMidiSession(QString sessionName);
	void deleteMidiSession(MidiSession *midiSession);
	void showBalloon(const QString &title, const QString &text);

public:
	MidiDriver(Master *useMaster, Qt::ConnectionType masterConnectionType = Qt::DirectConnection);
	Master *getMaster();
	virtual ~MidiDriver();
	virtual QString getName() const;
	virtual void start() = 0;
	virtual void stop() = 0;

signals:
	void nameChanged(const QString &name);
	void midiSessionInitiated(MidiSession **returnVal, MidiDriver *midiDriver, QString name);
	void midiSessionDeleted(MidiSession *midiSession);
	void balloonMessageAppeared(const QString &title, const QString &text);
};

#endif
