#ifndef QMIDI_BUFFER_H
#define QMIDI_BUFFER_H

#include <QtGlobal>

#include "QRingBuffer.h"

/**
 * A semi-lock-free queue intended to collect MIDI events coming from a single MIDI source before rendering audio output.
 * The MIDI events are expected to be in chronological order, timestamped using the count of frames rendered by QSynth.
 */
class QMidiBuffer {
public:
	QMidiBuffer();

	// Accessible from the MIDI source thread.
	bool pushShortMessage(quint64 timestamp, quint32 data);
	bool pushSysexMessage(quint64 timestamp, quint32 dataLength, const uchar *data);
	void flush();

	// Accessible from the rendering thread.
	bool retrieveEvents();
	quint64 getEventTimestamp() const;
	quint32 getEventData(const uchar *&sysexData) const;
	bool nextEvent();
	void discardEvents();
	void popEvents();

private:
	Utility::QRingBuffer ringBuffer;

	// Accessed from the MIDI source thread.
	void *writePointer;
	quint32 bytesWritten;
	quint32 bytesToWrite;
	bool freeSpaceContiguous;

	// Accessed from the rendering thread.
	void *readPointer;
	quint32 bytesRead;
	quint32 bytesToRead;

	bool requestSpace(quint32 length);
};

#endif
