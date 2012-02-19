#ifndef MIDI_SESSION_H
#define MIDI_SESSION_H

#include <QObject>

#include "SynthRoute.h"

class MidiDriver;

class MidiSession : public QObject {
	Q_OBJECT
	friend class Master;
private:
	MidiDriver *midiDriver;
	QString name;
	SynthRoute *synthRoute;

	MidiSession(QObject *parent, MidiDriver *useMidiDriver, QString useName, SynthRoute *useSynthRoute);

public:
	QString getName();
	void setName(const QString &newName);
	SynthRoute *getSynthRoute();
};

#endif
