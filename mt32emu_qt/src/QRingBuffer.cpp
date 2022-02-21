/* Copyright (C) 2011-2022 Jerome Fisher, Sergey V. Mikayev
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
#include "QAtomicHelper.h"

using namespace Utility;

QRingBuffer::QRingBuffer(const quint32 byteSize) :
	buffer(new uchar[byteSize]), bufferSize(byteSize)
{}

QRingBuffer::~QRingBuffer() {
	delete[] buffer;
}

void *QRingBuffer::writePointer(quint32 &bytesFree, bool &freeSpaceContiguous) const {
	// Acquire barrier ensures that data is never written ahead of the indices, when the buffer
	// space might still be in use. In practice however, this is barely possible, because all
	// subsequent data writes depend on values of the indices, but shouldn't hurt anyway.
	quint32 myReadPosition = QAtomicHelper::loadAcquire(readPosition);
	quint32 myWritePosition = QAtomicHelper::loadRelaxed(writePosition);
	bool wrapped = myWritePosition < myReadPosition;
	bytesFree = (wrapped ? myReadPosition : bufferSize) - myWritePosition;
	freeSpaceContiguous = wrapped || myReadPosition == 0;
	// As a simple overflow protection, reserve the last byte and do not let the buffer become full.
	if (freeSpaceContiguous) bytesFree--;
	return buffer + myWritePosition;
}

void QRingBuffer::advanceWritePointer(quint32 bytesWritten) {
	quint32 myWritePosition = QAtomicHelper::loadRelaxed(writePosition);
	myWritePosition += bytesWritten;
	if (bufferSize <= myWritePosition) {
		myWritePosition -= bufferSize;
	}
	// Release barrier ensures that data is fully written prior to updating the index.
	QAtomicHelper::storeRelease(writePosition, myWritePosition);
}

void *QRingBuffer::readPointer(quint32 &bytesReady) const {
	// Acquire barrier ensures that data is never read ahead of the indices, when the buffer
	// space might not contain valid data yet. In practice however, this is barely possible,
	// because all subsequent data reads depend on values of the indices, but shouldn't hurt anyway.
	quint32 myReadPosition = QAtomicHelper::loadRelaxed(readPosition);
	quint32 myWritePosition = QAtomicHelper::loadAcquire(writePosition);
	bytesReady = (myWritePosition < myReadPosition ? bufferSize : myWritePosition) - myReadPosition;
	return buffer + myReadPosition;
}

void QRingBuffer::advanceReadPointer(quint32 bytesRead) {
	quint32 myReadPosition = QAtomicHelper::loadRelaxed(readPosition);
	myReadPosition += bytesRead;
	if (bufferSize <= myReadPosition) {
		myReadPosition -= bufferSize;
	}
	// Release barrier ensures that data is completely read prior to updating the index.
	QAtomicHelper::storeRelease(readPosition, myReadPosition);
}
