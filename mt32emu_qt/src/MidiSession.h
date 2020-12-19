#ifndef MIDI_SESSION_H
#define MIDI_SESSION_H

#include <QObject>

#include "SynthRoute.h"

class MidiDriver;
class MidiTrackRecorder;
class QMidiStreamParser;
class QMidiBuffer;

class MidiSession : public QObject {
	Q_OBJECT
	friend class Master;
private:
	MidiDriver *midiDriver;
	QString name;
	SynthRoute *synthRoute;
	QMidiStreamParser *qMidiStreamParser;
	QMidiBuffer *qMidiBuffer;
	MidiTrackRecorder *midiTrackRecorder;

	MidiSession(QObject *parent, MidiDriver *useMidiDriver, QString useName, SynthRoute *useSynthRoute);
	~MidiSession();

public:
	QString getName();
	void setName(const QString &newName);
	SynthRoute *getSynthRoute() const;
	QMidiStreamParser *getQMidiStreamParser();
	QMidiBuffer *getQMidiBuffer();
	MidiTrackRecorder *getMidiTrackRecorder();
	MidiTrackRecorder *setMidiTrackRecorder(MidiTrackRecorder *midiTrackRecorder);
};

class QMidiStreamParser : public MT32Emu::MidiStreamParser {
public:
	QMidiStreamParser(MidiSession &midiSession);
	void setTimestamp(MasterClockNanos newTimestamp);

protected:
	void handleShortMessage(const MT32Emu::Bit32u message);
	void handleSysex(const MT32Emu::Bit8u stream[], const MT32Emu::Bit32u length);
	void handleSystemRealtimeMessage(const MT32Emu::Bit8u realtime);
	void printDebug(const char *debugMessage);

private:
	MidiSession &midiSession;
	MasterClockNanos timestamp;
};

#endif
