#ifndef MIDI_DRIVER_H
#define MIDI_DRIVER_H

#include <QObject>

class SynthManager;

class MidiDriver : public QObject {
	Q_OBJECT
protected:
	QString name;

	void setName(const QString &name);
public:
	MidiDriver();
	virtual ~MidiDriver();
	virtual QString getName() const;
signals:
	void nameChanged(const QString &name);
};

#endif
