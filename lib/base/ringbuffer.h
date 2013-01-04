#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

typedef struct {
    ssize_t size;
    volatile ssize_t w;
    volatile ssize_t r;
    char *ptr;
} ring_buffer_t;

class RingBuffer
{

public:
	explicit RingBuffer(const ssize_t size);
                ~RingBuffer();

	ssize_t availableToWrite() const;
        ssize_t availableToRead() const;
        ssize_t write(const char *src, const ssize_t len);
        ssize_t read(char *dest, const ssize_t len);

private:
        ring_buffer_t m_ringBuffer;

	RingBuffer();
        RingBuffer(const RingBuffer&);
        const RingBuffer& operator=(const RingBuffer&);
}

#endif
