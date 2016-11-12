#ifndef MIDI_RECORDER_H
#define MIDI_RECORDER_H

#include <QtCore>

#include "MasterClock.h"
#include "QMidiEvent.h"

class MidiRecorder {
public:
	MidiRecorder();
	void startRecording();
	void recordShortMessage(quint32 msg, MasterClockNanos midiNanos);
	void recordSysex(const uchar *sysexData, quint32 sysexLen, MasterClockNanos midiNanos);
	void stopRecording();
	bool isRecording();
	bool saveSMF(QString fileName, MasterClockNanos midiTick);

private:
	QMidiEventList midiEventList;
	QFile file;
	MasterClockNanos startNanos, endNanos;

	bool writeHeader(uint division);
	bool writeTrack(MasterClockNanos midiTick);
	bool writeFile(const char *data, qint64 len);
	void writeVarLenInt(uchar * &data, quint32 value);
};

#endif
