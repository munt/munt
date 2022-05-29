#ifndef MIDI_PARSER_H
#define MIDI_PARSER_H

#include <QtCore>

#include "QMidiEvent.h"

class MidiParser : public MidiStreamSource {
public:
	bool parse(const QString fileName);
	const QString getStreamName() const;
	const QMidiEventList &getMIDIEvents() const;
	MasterClockNanos getMidiTick(uint tempo = DEFAULT_TEMPO) const;
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
