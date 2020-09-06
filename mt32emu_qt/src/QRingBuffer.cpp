/* Copyright (C) 2011-2020 Jerome Fisher, Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "QRingBuffer.h"

using namespace Utility;

// There's no such thing like atomic load or store in the old Qt4 API.
// Although, the implementation suggests that the available "non-atomic"
// operations involve aligned volatile access which should be atomic.
// To enforce memory ordering, we have to invoke a real atomic operation though.

static inline quint32 loadRelaxed(const QAtomicInt &atomicInt) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	return atomicInt;
#elif QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	return atomicInt.load();
#else
	return atomicInt.loadRelaxed();
#endif
}

static inline quint32 loadAcquire(const QAtomicInt &atomicInt) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	QAtomicInt helper(atomicInt);
	return helper.fetchAndStoreAcquire(0);
#else
	return atomicInt.loadAcquire();
#endif
}

static inline void storeRelaxed(QAtomicInt &atomicInt, quint32 value) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	atomicInt = value;
#elif QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	atomicInt.store(value);
#else
	atomicInt.storeRelaxed(value);
#endif
}

static inline void storeRelease(QAtomicInt &atomicInt, quint32 value) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	atomicInt.fetchAndStoreRelease(value);
#else
	atomicInt.storeRelease(value);
#endif
}

QRingBuffer::QRingBuffer(const quint32 byteSize) :
	buffer(new uchar[byteSize]), bufferSize(byteSize)
{}

QRingBuffer::~QRingBuffer() {
	delete[] buffer;
}

void *QRingBuffer::writePointer(quint32 &bytesFree, bool &freeSpaceContiguous) const {
	// Aquire barrier ensures that data is never written ahead of the indices, when the buffer
	// space might still be in use. In practice however, this is barely possible, because all
	// subsequent data writes depend on values of the indices, but shouldn't hurt anyway.
	quint32 myReadPosition = loadAcquire(readPosition);
	quint32 myWritePosition = loadRelaxed(writePosition);
	bool wrapped = myWritePosition < myReadPosition;
	bytesFree = (wrapped ? myReadPosition : bufferSize) - myWritePosition;
	freeSpaceContiguous = wrapped || myReadPosition == 0;
	// As a simple overflow protection, reserve the last byte and do not let the buffer become full.
	if (freeSpaceContiguous) bytesFree--;
	return buffer + myWritePosition;
}

void QRingBuffer::advanceWritePointer(quint32 bytesWritten) {
	quint32 myWritePosition = loadRelaxed(writePosition);
	myWritePosition += bytesWritten;
	if (bufferSize <= myWritePosition) {
		myWritePosition -= bufferSize;
	}
	// Release barrier ensures that data is fully written prior to updating the index.
	storeRelease(writePosition, myWritePosition);
}

void *QRingBuffer::readPointer(quint32 &bytesReady) const {
	// Aquire barrier ensures that data is never read ahead of the indices, when the buffer
	// space might not contain valid data yet. In practice however, this is barely possible,
	// because all subsequent data reads depend on values of the indices, but shouldn't hurt anyway.
	quint32 myReadPosition = loadRelaxed(readPosition);
	quint32 myWritePosition = loadAcquire(writePosition);
	bytesReady = (myWritePosition < myReadPosition ? bufferSize : myWritePosition) - myReadPosition;
	return buffer + myReadPosition;
}

void QRingBuffer::advanceReadPointer(quint32 bytesRead) {
	quint32 myReadPosition = loadRelaxed(readPosition);
	myReadPosition += bytesRead;
	if (bufferSize <= myReadPosition) {
		myReadPosition -= bufferSize;
	}
	// Release barrier ensures that data is completely read prior to updating the index.
	storeRelease(readPosition, myReadPosition);
}
