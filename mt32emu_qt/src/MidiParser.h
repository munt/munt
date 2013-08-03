#ifndef MIDI_PARSER_H
#define MIDI_PARSER_H

#include <QtCore>

#include "QMidiEvent.h"

class MidiParser {
public:
	static const quint32 DEFAULT_BPM = 120;
	static const quint32 MICROSECONDS_PER_MINUTE = 60000000;
	static const int DEFAULT_TEMPO = MICROSECONDS_PER_MINUTE / DEFAULT_BPM;

	bool parse(const QString fileName);
	const QMidiEventList &getMIDIEvents();
	SynthTimestamp getMidiTick(uint tempo = DEFAULT_TEMPO);
	void addChannelsReset();

private:
	QFile file;
	QMidiEventList midiEventList;

	unsigned int format;
	unsigned int numberOfTracks;
	int division;

	static quint32 parseVarLenInt(const uchar * &data);

	bool readFile(char *data, qint64 len);
	bool parseHeader();
	bool parseTrack(QMidiEventList &midiEventList);
	void mergeMidiEventLists(QVector<QMidiEventList> &tracks);
	bool parseSysex();
	bool doParse();
};

#endif
