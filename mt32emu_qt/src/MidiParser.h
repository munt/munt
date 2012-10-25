#ifndef MIDI_PARSER_H
#define MIDI_PARSER_H

#include <QtCore>

#include "MidiEvent.h"

class MidiParser {
private:
	static const int DEFAULT_TEMPO = 60000000 / 120;

	QFile file;
	MidiEventList midiEventList;

	unsigned int format;
	unsigned int numberOfTracks;
	int division;

	bool readFile(char *data, qint64 len);
	bool parseHeader();
	bool parseTrack(MidiEventList &midiEventList);
	quint32 parseVarLenInt(uchar * &data);
	void mergeMidiEventLists(QVector<MidiEventList> &tracks);
	bool parseSysex();
	bool doParse();

public:
	bool parse(QString fileName);
	bool parse(QStringList fileNameList);
	int getDivision();
	const MidiEventList &getMIDIEvents();
	SynthTimestamp getMidiTick(uint tempo = DEFAULT_TEMPO);
	void addAllNotesOff();
};

#endif
