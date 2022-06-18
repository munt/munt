#ifndef MIDI_DRIVER_H
#define MIDI_DRIVER_H

#include <QtCore>

#include "../MasterClock.h"

class Master;
class MidiSession;
class SynthRoute;

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
	enum PortNamingPolicy {
		PortNamingPolicy_ARBITRARY,
		PortNamingPolicy_UNIQUE,
		PortNamingPolicy_RESTRICTED
	};

	MidiDriver(Master *useMaster);
	Master *getMaster();
	virtual ~MidiDriver();
	virtual QString getName() const;
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual bool canCreatePort();
	virtual bool canDeletePort(MidiSession *midiSession);
	virtual bool canReconnectPort(MidiSession *midiSession);
	virtual PortNamingPolicy getPortNamingPolicy();
	virtual bool createPort(int portIx, const QString &portName, MidiSession *midiSession);
	virtual void deletePort(MidiSession *) {}
	virtual void reconnectPort(int /* newPortIx */, const QString & /* newPortName */, MidiSession *) {}
	virtual QString getNewPortNameHint(QStringList &knownPortNames);

signals:
	void midiSessionInitiated(MidiSession **returnVal, MidiDriver *midiDriver, QString name);
	void midiSessionDeleted(MidiSession *midiSession);
	void balloonMessageAppeared(const QString &title, const QString &text);
};

#endif
