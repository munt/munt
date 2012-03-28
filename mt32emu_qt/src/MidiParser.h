#ifndef MIDI_PARSER_H
#define MIDI_PARSER_H

#include <QtCore>

#include "MidiEvent.h"

class MidiParser {
private:
	static const int DEFAULT_TEMPO = 60000000 / 120;

	QFile file;
	QVector<MidiEvent> midiEventList;

	unsigned int format;
	unsigned int numberOfTracks;
	int division;

	bool readFile(char *data, qint64 len);
	bool parseHeader();
	bool parseTrack(QVector<MidiEvent> &midiEventList);
	quint32 parseVarLenInt(uchar * &data);
	void mergeMidiEventLists(QVector< QVector<MidiEvent> > &tracks);
	bool parseSysex();

public:
	bool parse(QString fileName);
	int getDivision();
	QVector<MidiEvent> getMIDIEvents();
	SynthTimestamp getMidiTick(uint tempo = DEFAULT_TEMPO);
	void addAllNotesOff();
};

#endif
