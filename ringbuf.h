
#ifndef SALVOQ_RINGBUF_H
#define SALVOQ_RINGBUF_H
#include <stdatomic.h>
#include <stddef.h>
typedef struct {
    float data[SALVOQ_RING_CAPACITY];
    atomic_size_t head, tail;
    _Atomic unsigned long overflow;
} ringbuf;
size_t rb_write(ringbuf *r, const float *src, size_t n);
size_t rb_read(ringbuf *r, float *dst, size_t n);
size_t rb_fill(ringbuf *r);
#endif
