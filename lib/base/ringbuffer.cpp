#include "ringbuffer.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

RingBuffer::RingBuffer(const ssize_t size) 
{
	const ssize_t m = size/256 + 1; // size must be a multiple of 256
	m_ringBuffer.size = m*256;
	m_ringBuffer.ptr = new char(m_ringBuffer.size);
	m_ringBuffer.w = 0;
	m_ringBuffer.r = 0;
}

RingBuffer::~RingBuffer() 
{
	delete m_ringBuffer.ptr;
}

ssize_t RingBuffer::availableToWrite() const
{
  // Note: The largest possible result is m_ringBuffer.size - 1 because
  // we adopt the convention that m_ringBuffer.r == m_ringBuffer.w means that the
  // buffer is empty.
	if (m_ringBuffer.ptr) {
		return (m_ringBuffer.size + m_ringBuffer.r - m_ringBuffer.w - 1) % m_ringBuffer.size;
	}
	return 0;
}

ssize_t RingBuffer::availableToRead() const
{
	if (m_ringBuffer.ptr) {
		return (m_ringBuffer.size + m_ringBuffer.w - m_ringBuffer.r) % m_ringBuffer.size;
	} else {
		return 0;
	}
}

ssize_t RingBuffer::write(const char *src, const ssize_t len) 
{
        const ssize_t toWrite = MIN(len, availableToWrite());
	if (toWrite > 0) {
		const ssize_t w = m_ringBuffer.w;
		if (w + toWrite <= m_ringBuffer.size) {
			memcpy(m_ringBuffer.ptr + w, src, toWrite);
		} else {
			const ssize_t d = m_ringBuffer.size - w;
			memcpy(m_ringBuffer.ptr + w, src, d);
			memcpy(m_ringBuffer.ptr, src + d, toWrite - d);
		}
		__sync_synchronize();  // memory barrier
		__sync_val_compare_and_swap(&(m_ringBuffer.w), m_ringBuffer.w, (w + toWrite) % m_ringBuffer.size);
	}
	return toWrite; 
}

ssize_t RingBuffer::read(char *dest, const ssize_t len)
{
        const ssize_t toRead = MIN(len, availableToRead());
	if (toRead > 0) {
		__sync_synchronize();  // memory barrier
		const ssize_t r =m_ringBuffer.r ;
		if (r + toRead <= ) {
			memcpy(dest, m_ringBuffer.ptr + r, toRead);
		} else {
			const ssize_t d = m_ringBuffer.size - r;
			memcpy(dest, m_ringBuffer.ptr + r, d);
			memcpy(dest + d, m_ringBuffer.ptr, toRead - d);
		}
		__sync_val_compare_and_swap(&(m_ringBuffer.r), m_ringBuffer.r, (r + toRead) % m_ringBuffer.size);
	}
        return toRead;
}
