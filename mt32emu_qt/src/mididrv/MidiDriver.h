#ifndef MIDI_DRIVER_H
#define MIDI_DRIVER_H

#include <QObject>

#include "../Master.h"

class SynthRoute;
class MidiPropertiesDialog;

class MidiDriver : public QObject {
	Q_OBJECT
protected:
	Master *master;
	QString name;
	QList<MidiSession *>midiSessions;

	static void waitForProcessingThread(QThread &thread, MasterClockNanos timeout);

	MidiSession *createMidiSession(QString sessionName);
	void deleteMidiSession(MidiSession *midiSession);
	void showBalloon(const QString &title, const QString &text);

public:
	MidiDriver(Master *useMaster);
	Master *getMaster();
	virtual ~MidiDriver();
	virtual QString getName() const;
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual bool canCreatePort();
	virtual bool canDeletePort(MidiSession *midiSession);
	virtual bool canSetPortProperties(MidiSession *midiSession);
	virtual bool createPort(MidiPropertiesDialog *mpd, MidiSession *midiSession);
	virtual void deletePort(MidiSession *) {}
	virtual bool setPortProperties(MidiPropertiesDialog *mpd, MidiSession *midiSession);
	virtual QString getNewPortName(MidiPropertiesDialog *mpd);

signals:
	void midiSessionInitiated(MidiSession **returnVal, MidiDriver *midiDriver, QString name);
	void midiSessionDeleted(MidiSession *midiSession);
	void balloonMessageAppeared(const QString &title, const QString &text);
};

#endif
