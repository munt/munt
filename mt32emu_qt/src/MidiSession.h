#ifndef MIDI_SESSION_H
#define MIDI_SESSION_H

#include <QObject>

#include "SynthRoute.h"

class MidiDriver;
class QMidiStreamParser;

class MidiSession : public QObject {
	Q_OBJECT
	friend class Master;
private:
	MidiDriver *midiDriver;
	QString name;
	SynthRoute *synthRoute;
	QMidiStreamParser *qMidiStreamParser;

	MidiSession(QObject *parent, MidiDriver *useMidiDriver, QString useName, SynthRoute *useSynthRoute);
	~MidiSession();

public:
	QString getName();
	void setName(const QString &newName);
	SynthRoute *getSynthRoute();
	QMidiStreamParser *getQMidiStreamParser();
};

class QMidiStreamParser : public MT32Emu::MidiStreamParser {
public:
	QMidiStreamParser(SynthRoute &useSynthRoute);
	void setTimestamp(MasterClockNanos newTimestamp);

protected:
	void handleShortMessage(const MT32Emu::Bit32u message);
	void handleSysex(const MT32Emu::Bit8u stream[], const MT32Emu::Bit32u length);
	void handleSystemRealtimeMessage(const MT32Emu::Bit8u realtime);
	void printDebug(const char *debugMessage);

private:
	SynthRoute &synthRoute;
	MasterClockNanos timestamp;
};

#endif
