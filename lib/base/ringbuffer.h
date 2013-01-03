#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

typedef struct ring_buffer {
    int size;
    char *ptr;
    volatile int w;
    volatile int r;
} ring_buffer;

class RingBuffer
{

public:
	explicit RingBuffer(ssize_t size);
                ~RingBuffer();

int rb_available_to_write(ring_buffer *buffer);
int rb_available_to_read(ring_buffer *buffer);
int rb_write_to_buffer(ring_buffer *buffer, const char *src, int len);
int rb_read_from_buffer(ring_buffer *buffer, char *dest, int len);

private:
	RingBuffer();
        RingBuffer(const RingBuffer&);
        const RingBuffer& operator=(const RingBuffer&);
}

#endif
