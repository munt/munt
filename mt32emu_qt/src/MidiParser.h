#ifndef MIDI_PARSER_H
#define MIDI_PARSER_H

#include <QtCore>

#include "MidiEvent.h"

class MidiParser {
public:
	static const quint32 DEFAULT_BPM = 120;
	static const quint32 MICROSECONDS_PER_MINUTE = 60000000;
	static const int DEFAULT_TEMPO = MICROSECONDS_PER_MINUTE / DEFAULT_BPM;

	bool parse(const QString fileName);
	const MidiEventList &getMIDIEvents();
	SynthTimestamp getMidiTick(uint tempo = DEFAULT_TEMPO);
	void addAllNotesOff();

private:
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
};

#endif
