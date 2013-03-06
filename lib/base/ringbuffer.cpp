#include "ringbuffer.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

RingBuffer::RingBuffer(const size_t size) 
{
	const size_t m = size/256 + 1; // size must be a multiple of 256
	m_ringBuffer.size = m*256;
	m_ringBuffer.ptr = (char*)malloc(m_ringBuffer.size);
	m_ringBuffer.w = 0;
	m_ringBuffer.r = 0;
}

RingBuffer::~RingBuffer() 
{
	 free(m_ringBuffer.ptr);
}

size_t RingBuffer::availableToWrite() const
{
  // Note: The largest possible result is m_ringBuffer.size - 1 because
  // we adopt the convention that m_ringBuffer.r == m_ringBuffer.w means that the
  // buffer is empty.
	if (m_ringBuffer.ptr) {
		return (m_ringBuffer.size + m_ringBuffer.r - m_ringBuffer.w - 1) % m_ringBuffer.size;
	}
	return 0;
}

size_t RingBuffer::availableToRead() const
{
	if (m_ringBuffer.ptr) {
		return (m_ringBuffer.size + m_ringBuffer.w - m_ringBuffer.r) % m_ringBuffer.size;
	} else {
		return 0;
	}
}

size_t RingBuffer::write(const char *src, const size_t len) 
{
        const size_t toWrite = MIN(len, availableToWrite());
	if (toWrite > 0) {
		const size_t w = m_ringBuffer.w;
		if (w + toWrite <= m_ringBuffer.size) {
			memcpy(m_ringBuffer.ptr + w, src, toWrite);
		} else {
			const size_t d = m_ringBuffer.size - w;
			memcpy(m_ringBuffer.ptr + w, src, d);
			memcpy(m_ringBuffer.ptr, src + d, toWrite - d);
		}
#if defined(__sh__)
		m_ringBuffer.w = (w + toWrite) % m_ringBuffer.size;
#else
		__sync_synchronize();  // memory barrier
		__sync_val_compare_and_swap(&(m_ringBuffer.w), m_ringBuffer.w, (w + toWrite) % m_ringBuffer.size);
#endif
	}
	return toWrite; 
}

size_t RingBuffer::read(char *dest, const size_t len)
{
        const ssize_t toRead = MIN(len, availableToRead());
	if (toRead > 0) {
#if !defined(__sh__)
		__sync_synchronize();  // memory barrier
#endif
		const size_t r = m_ringBuffer.r ;
		if (r + toRead <= m_ringBuffer.size) {
			memcpy(dest, m_ringBuffer.ptr + r, toRead);
		} else {
			const size_t d = m_ringBuffer.size - r;
			memcpy(dest, m_ringBuffer.ptr + r, d);
			memcpy(dest + d, m_ringBuffer.ptr, toRead - d);
		}
#if defined(__sh__)
		m_ringBuffer.r = (r + toRead) % m_ringBuffer.size;
#else
		__sync_val_compare_and_swap(&(m_ringBuffer.r), m_ringBuffer.r, (r + toRead) % m_ringBuffer.size);
#endif
	}
        return toRead;
}

void RingBuffer::skip(const size_t len)
{
	const size_t toSkip = MIN(len, availableToRead());
	m_ringBuffer.r = (m_ringBuffer.r + toSkip) % m_ringBuffer.size;
}

size_t RingBuffer::availableToWritePtr()
{
	size_t toWrite = availableToWrite();
        if (toWrite > 0) {
        	toWrite = MIN(toWrite, (m_ringBuffer.size - m_ringBuffer.w));
	}
	return toWrite;
}

void RingBuffer::ptrWriteCommit(const size_t toCommit)
{
	m_ringBuffer.w = (m_ringBuffer.w + toCommit) % m_ringBuffer.size;
}
