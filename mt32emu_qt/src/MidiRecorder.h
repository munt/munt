#ifndef MIDI_RECORDER_H
#define MIDI_RECORDER_H

#include <QtCore>

#include "MasterClock.h"

class DataChunk;
class MidiTrackRecorder;

class MidiRecorder : public QObject {
	Q_OBJECT
public:
	MidiRecorder();
	~MidiRecorder();

	void reset();
	void startRecording();
	bool stopRecording();
	bool isEmpty() const;
	bool isRecording() const;
	bool hasPendingData() const;

	// Methods below are only invoked from the main thread.
	MidiTrackRecorder *addTrack();
	bool saveSMF(QString fileName, MasterClockNanos midiTick);

private:
	QAtomicInt status;

	// Fields below are only accessed from the main thread.
	MasterClockNanos startNanos, endNanos;
	QList<MidiTrackRecorder *> midiTrackRecorders;
	QTimer allocationTimer;

	bool writeHeader(QFile &file, const int numberOfTracks, uint division);
	bool writeTrack(QFile &file, MidiTrackRecorder *midiTrackRecorder, const MasterClockNanos midiTick);
	bool writeFile(QFile &file, const char *data, qint64 len);
	void writeMessageTimestamp(uchar * &data, quint32 &eventTicks, const MasterClockNanos timestamp, const MasterClockNanos midiTick);
	void writeVarLenInt(uchar * &data, quint32 value);

private slots:
	void handleAllocationTimer();
};

// A semi-lock-free MIDI stream buffer capable to grow as it gets filled up.
// Memory is allocated upfront by chunks of predefined size in the main thread.
// This property makes it suitable for using directly from a realtime thread.
class MidiTrackRecorder {
friend class MidiRecorder;

public:
	MidiTrackRecorder(MidiRecorder &midiRecorder);
	~MidiTrackRecorder();

	// Methods below are only invoked from a single MIDI driver thread.
	bool recordShortMessage(quint32 shortMessageData, MasterClockNanos midiNanos);
	bool recordSysex(const uchar *sysexData, quint32 sysexDataLength, MasterClockNanos midiNanos);

private:
	MidiRecorder &midiRecorder;
	QMutex trackMutex;
	DataChunk * const firstChunk;
	QAtomicPointer<DataChunk> currentChunk;

	// This field is only accessed from the MIDI driver thread.
	quint32 writePosition;

	uchar *requireBufferSpace(quint32 byteLength);
};

#endif
