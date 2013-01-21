#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

typedef struct __attribute__ ((aligned (4))){
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

	void reset() { m_ringBuffer.w=0; m_ringBuffer.r=0;}
	ssize_t availableToWrite() const;
	/*space to write to the buffer without the need to wrap around*/
	ssize_t availableToWritePtr();
	/*update the write position after an external direct write in to the buffer */
	void    ptrWriteCommit(const ssize_t len);
        ssize_t availableToRead() const;
        char*   ptr(){return (m_ringBuffer.ptr + m_ringBuffer.w);}
        ssize_t write(const char *src, const ssize_t len);
        ssize_t read(char *dest, const ssize_t len);
	void    skip(const ssize_t len);

private:
        ring_buffer_t m_ringBuffer;

	RingBuffer();
        RingBuffer(const RingBuffer&);
        const RingBuffer& operator=(const RingBuffer&);
};

#endif
