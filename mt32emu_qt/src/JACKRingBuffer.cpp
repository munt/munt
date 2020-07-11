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

#include "JACKRingBuffer.h"

JACKRingBuffer::JACKRingBuffer(size_t byteSize) :
	ringBuffer(jack_ringbuffer_create(byteSize))
{}

JACKRingBuffer::~JACKRingBuffer() {
	jack_ringbuffer_free(ringBuffer);
}

void *JACKRingBuffer::readPointer(size_t &bytesAvailable) const {
	jack_ringbuffer_data_t data[2];
	jack_ringbuffer_get_read_vector(ringBuffer, data);
	bytesAvailable = data[0].len;
	return data[0].buf;
}

void JACKRingBuffer::advanceReadPointer(size_t bytesRead) {
	jack_ringbuffer_read_advance(ringBuffer, bytesRead);
}

void *JACKRingBuffer::writePointer(size_t &bytesAvailable) const {
	jack_ringbuffer_data_t data[2];
	jack_ringbuffer_get_write_vector(ringBuffer, data);
	bytesAvailable = data[0].len;
	return data[0].buf;
}

void JACKRingBuffer::advanceWritePointer(size_t bytesWritten) {
	jack_ringbuffer_write_advance(ringBuffer, bytesWritten);
}
