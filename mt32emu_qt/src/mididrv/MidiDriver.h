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

public:
	MidiDriver(Master *useMaster);
	Master *getMaster();
	virtual ~MidiDriver();
	virtual QString getName() const;
	virtual void start() = 0;
	virtual void stop() = 0;

signals:
	void nameChanged(const QString &name);
};

#endif
