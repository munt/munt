#ifndef AUDIO_FILE_WRITER_H
#define AUDIO_FILE_WRITER_H

#include <QtCore>

#include "MasterClock.h"
#include "MidiParser.h"
#include "QSynth.h"

class AudioFileWriter : public QThread {
public:
	AudioFileWriter();
	~AudioFileWriter();
	void convertMIDIFile(QString useOutFileName, QString useMIDIFileName, unsigned int bufferSize = 65536);
	void startRealtimeProcessing(QSynth *useSynth, unsigned int useSampleRate, QString useOutFileName, unsigned int bufferSize, MasterClockNanos latency);
	void stop();

protected:
	void run();

private:
	QSynth *synth;
	unsigned int sampleRate;
	unsigned int bufferSize;
	MasterClockNanos latency;
	QString outFileName;
	qint16 *buffer;
	QString midiFileName;
	MidiParser *parser;
	bool realtimeMode;
	volatile bool stopProcessing;

	void setTempo(uint newTempo);
};

#endif
