#ifndef MIDI_PARSER_H
#define MIDI_PARSER_H

#include <QtCore>

#include "MidiEvent.h"

class MidiParser {
private:
	QFile file;
	QList<MidiEvent> midiEventList;

	unsigned int format;
	unsigned int numberOfTracks;
	int division;

	bool readFile(char *data, qint64 len);
	bool parseHeader();
	bool parseTrack();
	quint32 parseVarLenInt(uchar * &data);

public:
	MidiParser(QString fileName);
	~MidiParser();
	bool parse();
	int getDivision();
	QList<MidiEvent> getMIDIEventList();
};

#endif
