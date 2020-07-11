#ifndef JACK_RING_BUFFER_H
#define JACK_RING_BUFFER_H

#include <jack/ringbuffer.h>

class JACKRingBuffer {
public:
	JACKRingBuffer(size_t byteSize);
	~JACKRingBuffer();

	void *readPointer(size_t &bytesAvailable) const;
	void advanceReadPointer(size_t bytesRead);

	void *writePointer(size_t &bytesAvailable) const;
	void advanceWritePointer(size_t bytesWritten);

private:
	jack_ringbuffer_t * const ringBuffer;
};

#endif
