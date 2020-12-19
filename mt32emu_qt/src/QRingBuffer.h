#ifndef QRING_BUFFER_H
#define QRING_BUFFER_H

#include <QtGlobal>
#include <QAtomicInt>

namespace Utility {

/**
 * A semi-lock-free byte buffer that facilitates communication between two threads (producer and consumer).
 * Depending on the Qt implementation, it may well appear "wait-free" on certain architectures.
 */
class QRingBuffer {
public:
	explicit QRingBuffer(const quint32 byteSize);
	~QRingBuffer();

	// Accessible from the producer thread.
	void *writePointer(quint32 &bytesFree, bool &freeSpaceContiguous) const;
	void advanceWritePointer(quint32 bytesWritten);

	// Accessible from the consumer thread.
	void *readPointer(quint32 &bytesUsed) const;
	void advanceReadPointer(quint32 bytesRead);

private:
	uchar * const buffer;
	const quint32 bufferSize;
	QAtomicInt readPosition;
	QAtomicInt writePosition;
};

}

#endif
