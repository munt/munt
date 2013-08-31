#ifndef AUDIO_FILE_WRITER_H
#define AUDIO_FILE_WRITER_H

#include <QtCore>

#include "MasterClock.h"
#include "MidiParser.h"
#include "QSynth.h"

class AudioFileWriter : public QThread {
	Q_OBJECT

public:
	explicit AudioFileWriter();
	~AudioFileWriter();
	bool convertMIDIFiles(QString useOutFileName, QStringList useMIDIFileNameList, QString synthProfileName, quint32 bufferSize = 65536);
	void startRealtimeProcessing(QSynth *useSynth, quint32 useSampleRate, QString useOutFileName, quint32 bufferSize);
	void stop();

protected:
	void run();

private:
	QSynth *synth;
	unsigned int sampleRate;
	unsigned int bufferSize;
	QString outFileName;
	qint16 *buffer;
	MidiParser *parsers;
	uint parsersCount;
	bool realtimeMode;
	volatile bool stopProcessing;


signals:
	void parsingFailed(const QString &, const QString &);
	void midiEventProcessed(int midiEventsProcessed, int midiEventsTotal);
	void conversionFinished();
};

#endif
