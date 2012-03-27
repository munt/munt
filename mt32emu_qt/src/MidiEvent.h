#ifndef MIDI_EVENT_H
#define MIDI_EVENT_H

#include <QtGlobal>

#include <mt32emu/mt32emu.h>

enum MidiEventType {
	SHORT_MESSAGE,
	SYSEX,
	SET_TEMPO
};

typedef qint64 SynthTimestamp;

class MidiEvent {
private:
	SynthTimestamp timestamp;
	MidiEventType type;
	MT32Emu::Bit32u msg;
	MT32Emu::Bit32u sysexLen;
	unsigned char *sysexData;

public:
	MidiEvent();
	MidiEvent(const MidiEvent &copyOf);
	~MidiEvent();

	SynthTimestamp getTimestamp() const;
	MidiEventType getType() const;
	MT32Emu::Bit32u getShortMessage() const;
	MT32Emu::Bit32u getSysexLen() const;
	unsigned char *getSysexData() const;

	void setTimestamp(SynthTimestamp newTimestamp);
	void assignShortMessage(SynthTimestamp newTimestamp, MT32Emu::Bit32u newMsg);
	void assignSysex(SynthTimestamp newTimestamp, unsigned char *newSysexData, MT32Emu::Bit32u newSysexLen);
	void assignSetTempoMessage(SynthTimestamp newTimestamp, MT32Emu::Bit32u newTempo);
};

#endif
