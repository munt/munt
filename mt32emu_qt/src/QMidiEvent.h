#ifndef QMIDI_EVENT_H
#define QMIDI_EVENT_H

#include <QtGlobal>
#include <QVector>

#include <mt32emu/mt32emu.h>

#include "MasterClock.h"

enum MidiEventType {
	SHORT_MESSAGE,
	SYSEX,
	SET_TEMPO,
	SYNC
};

typedef MasterClockNanos SynthTimestamp;

class QMidiEvent {
private:
	SynthTimestamp timestamp;
	MidiEventType type;
	union {
		MT32Emu::Bit32u msg;
		MT32Emu::Bit32u sysexLen;
	};
	uchar *sysexData;

public:
	QMidiEvent();
	QMidiEvent(const QMidiEvent &copyOf);
	~QMidiEvent();

	SynthTimestamp getTimestamp() const;
	MidiEventType getType() const;
	MT32Emu::Bit32u getShortMessage() const;
	MT32Emu::Bit32u getSysexLen() const;
	uchar *getSysexData() const;

	void setTimestamp(SynthTimestamp newTimestamp);
	void assignShortMessage(SynthTimestamp newTimestamp, MT32Emu::Bit32u newMsg);
	void assignSysex(SynthTimestamp newTimestamp, uchar const * const newSysexData, MT32Emu::Bit32u newSysexLen);
	void assignSetTempoMessage(SynthTimestamp newTimestamp, MT32Emu::Bit32u newTempo);
	void assignSyncMessage(SynthTimestamp newTimestamp);
};

class QMidiEventList : public QVector<QMidiEvent> {
public:
	QMidiEvent &newMidiEvent();
};

class MidiStreamSource {
public:
	static const quint32 DEFAULT_BPM = 120;
	static const quint32 MICROSECONDS_PER_MINUTE = 60000000;
	static const uint DEFAULT_TEMPO = MICROSECONDS_PER_MINUTE / DEFAULT_BPM;

	virtual ~MidiStreamSource() {}
	virtual const QString getStreamName() const = 0;
	virtual const QMidiEventList &getMIDIEvents() const = 0;
	virtual MasterClockNanos getMidiTick(uint tempo = DEFAULT_TEMPO) const = 0;
};

#endif
