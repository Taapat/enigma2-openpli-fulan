#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

typedef struct __attribute__ ((aligned (4))){
    size_t size;
    volatile size_t w;
    volatile size_t r;
    char *ptr;
} ring_buffer_t;

class RingBuffer
{

public:
	explicit RingBuffer(const size_t size);
                ~RingBuffer();

	void reset() { m_ringBuffer.w=0; m_ringBuffer.r=0;}
	size_t availableToWrite() const;
	/*space to write to the buffer without the need to wrap around*/
	size_t availableToWritePtr();
	/*update the write position after an external direct write in to the buffer */
	void    ptrWriteCommit(const size_t len);
        size_t availableToRead() const;
        char*   ptr(){return (m_ringBuffer.ptr + m_ringBuffer.w);}
        size_t write(const char *src, const size_t len);
        size_t read(char *dest, const size_t len);
	void    skip(const size_t len);

private:
        ring_buffer_t m_ringBuffer;

	RingBuffer();
        RingBuffer(const RingBuffer&);
        const RingBuffer& operator=(const RingBuffer&);
};

#endif
